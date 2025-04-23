#include "VKImage.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

VulkanImage createVulkanImage(  
    VulkanInitData &vkInitData, int width, int height, 
    vk::Format format, vk::ImageUsageFlags usage,
    vk::ImageAspectFlags aspectFlags) {
    
    return createVulkanImage(vkInitData.device,
        vkInitData.physicalDevice,
        width, height, format, usage,
        aspectFlags);
}

VulkanImage createVulkanImage(  
    vk::Device &device, 
    vk::PhysicalDevice &phyDevice,
    int width, int height, 
    vk::Format format, vk::ImageUsageFlags usage,
    vk::ImageAspectFlags aspectFlags) {

    // Create struct
    VulkanImage vkImage;

    // Store format for later
    vkImage.format = format;

    ///////////////////////////////////////////////////////////////////////////
    // IMAGE
    ///////////////////////////////////////////////////////////////////////////

    // Create Image object
    vk::ImageCreateInfo imageInfo(
        {},
        vk::ImageType::e2D,                 // 2D image
        format,                             // Data format
        vk::Extent3D(width, height, 1),     // Dimensions (note 1 texel in depth)
        1, 1, vk::SampleCountFlagBits::e1,  // No mipmap, not an array, 1 sample (multisampling)
        vk::ImageTiling::eOptimal,          // Layout memory efficiently (can't read texels easily ourselves)
        usage,                              // Usage flags
        vk::SharingMode::eExclusive         // Only used by one queue family
    );

    auto image = device.createImage(imageInfo);

    // Allocate memory for image
    vk::MemoryRequirements memRequirements = device.getImageMemoryRequirements(image);

    vk::MemoryAllocateInfo allocInfo(memRequirements.size,
                                     findMemoryType(memRequirements.memoryTypeBits, 
                                                    vk::MemoryPropertyFlagBits::eDeviceLocal, 
                                                    phyDevice));

    auto memory = device.allocateMemory(allocInfo);

    // Bind memory to image
    device.bindImageMemory(image, memory, 0);

    // Move into struct (rather than copy)
    vkImage.image = std::move(image);
    vkImage.memory = std::move(memory);

    ///////////////////////////////////////////////////////////////////////////
    // IMAGEVIEW
    ///////////////////////////////////////////////////////////////////////////

    vk::ImageViewCreateInfo viewInfo(
        {},        
        vkImage.image, 
        vk::ImageViewType::e2D, 
        format,
        {},             // Leave components (e.g., RGB) as-is
        { aspectFlags, 0, 1, 0, 1 } // Aspect that are visible (also mipmap level and array ranges)
    );

    vkImage.view = device.createImageView(viewInfo);

    // Return struct
    return vkImage;
}

VulkanImage createVulkanDepthImage(
    VulkanInitData &vkInitData, 
    int width, int height) {

    return createVulkanDepthImage(
        vkInitData.device,
        vkInitData.physicalDevice,
        width, height);
}    

VulkanImage createVulkanDepthImage(
    vk::Device &device,
    vk::PhysicalDevice &phyDevice,
    int width, int height) {

    // Start with image
    VulkanImage depthImage;

    // Set depth format to EXTREMELY common option
    vk::Format depthFormat = vk::Format::eD32Sfloat;

    // Create Vulkan image accordingly
    depthImage = createVulkanImage( device,
                                    phyDevice, 
                                    width, height, 
                                    depthFormat, 
                                    vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                    vk::ImageAspectFlagBits::eDepth);  

    // Return image struct
    return depthImage; 
}

void transitionVulkanImageLayout(   VulkanInitData &vkInitData, 
                                    vk::CommandPool &commandPool,
                                    VulkanImage &vkImage, 
                                    vk::ImageLayout oldLayout,
                                    vk::ImageLayout newLayout) {
    // Create and start a command buffer
    vk::CommandBuffer oneTimeBuffer = createAndStartOneTimeVulkanCommandBuffer(vkInitData.device, commandPool);

    // Create memory barrier to work with buffers
    vk::ImageMemoryBarrier barrier
        = vk::ImageMemoryBarrier()
            .setOldLayout(oldLayout)
            .setNewLayout(newLayout)
            .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setImage(vkImage.image)
            .setSubresourceRange(
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));


    // Determine correct barrier masks
    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined 
        && newLayout == vk::ImageLayout::eTransferDstOptimal) {    

        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;

    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal 
                && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {

        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;

    } 
    else {
        throw invalid_argument("transitionVulkanImageLayout: unsupported layout transition!");
    }
            
    oneTimeBuffer.pipelineBarrier(sourceStage, destinationStage,
                                    {}, {}, {}, barrier);                                         
    
    // End recording, submit, and cleanup buffer
    stopAndCleanupOneTimeVulkanCommandBuffer(vkInitData.device, commandPool, 
                                            oneTimeBuffer, vkInitData.graphicsQueue.queue);      
}

void cleanupVulkanImage(VulkanInitData &vkInitData, VulkanImage &vkImage) {
    cleanupVulkanImage(vkInitData.device, vkImage);
}

void cleanupVulkanImage(vk::Device &device, VulkanImage &vkImage) {
    device.destroyImageView(vkImage.view);
    device.freeMemory(vkImage.memory);
    device.destroyImage(vkImage.image);
}
