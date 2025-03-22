#include "VKBuffer.hpp"

///////////////////////////////////////////////////////////////////////////////
// MEMORY QUERIES
///////////////////////////////////////////////////////////////////////////////

unsigned int findMemoryType(unsigned int typeFilter, 
                            vk::MemoryPropertyFlags properties, 
                            vk::PhysicalDevice physicalDevice) {

    // Get the memory properites from the physical device                        
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    // Loop through the properties to find a match in terms of the type and the properties
    for (unsigned int i = 0; i < memProperties.memoryTypeCount; i++) {
        unsigned int currentTypeBit = (1 << i);
        bool matchType = typeFilter & currentTypeBit;
        bool propEqual = ((memProperties.memoryTypes[i].propertyFlags & properties) == properties);

        if (matchType && propEqual) {
            return i;
        }
    }

    // If we are here, throw an exception
    throw runtime_error("findMemoryType: Failed to find suitable memory type!");
}

///////////////////////////////////////////////////////////////////////////////
// BUFFER MANAGEMENT
///////////////////////////////////////////////////////////////////////////////

VulkanBuffer createVulkanBuffer(vk::PhysicalDevice &physicalDevice,
                                vk::Device &device,
                                vk::DeviceSize size,
                                vk::BufferUsageFlags usage,
                                vk::MemoryPropertyFlags properties) {

    // Set up struct
    VulkanBuffer data;

    // Create buffer (memory not allocated YET)
    data.buffer = device.createBuffer(  vk::BufferCreateInfo(vk::BufferCreateFlags(), size, usage, 
                                        vk::SharingMode::eExclusive));
          
    // Get memory requirements
    vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(data.buffer);

    // Set up allocation info
    vk::MemoryAllocateInfo allocInfo(
        memRequirements.size, 
        findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice));
        
    // Actually allocate memory
    data.memory = device.allocateMemory(allocInfo);

    // Bind the memory
    device.bindBufferMemory(data.buffer, data.memory, 0);

    // Return data
    return data;
}

void copyDataToVulkanBuffer(vk::Device &device, vk::DeviceMemory memory, 
                            size_t bufferSize, void *hostData) {
                                
    void* data = device.mapMemory(memory, 0, bufferSize);
    memcpy(data, hostData, (size_t) bufferSize);
    device.unmapMemory(memory);
}

void copyBufferToVulkanBuffer(  vk::Device &device, vk::CommandPool &commandPool,
                                vk::Queue &graphicsQueue,
                                VulkanBuffer &src, VulkanBuffer &dst, vk::DeviceSize size) {
    // Create and start a command buffer
    vk::CommandBuffer oneTimeBuffer = createAndStartOneTimeVulkanCommandBuffer(device, commandPool);
    
    // Copy buffer over
    vk::BufferCopy copyRegion{};
    copyRegion.size = size;
    oneTimeBuffer.copyBuffer(src.buffer, dst.buffer, 1, &copyRegion);

    // End recording, submit, and cleanup buffer
    stopAndCleanupOneTimeVulkanCommandBuffer(device, commandPool, oneTimeBuffer, graphicsQueue);      
}

void copyDataToVulkanBufferViaStaging(  vk::PhysicalDevice &physicalDevice,
                                        vk::Device &device, 
                                        vk::CommandPool &commandPool,
                                        vk::Queue &graphicsQueue,
                                        VulkanBuffer &dst, 
                                        vk::DeviceSize size,
                                        void *data) {

    // Create staging buffer on CPU side and copy data to it
    VulkanBuffer stageBuffer = createVulkanBuffer(physicalDevice, device, size,
                                    vk::BufferUsageFlagBits::eTransferSrc,
                                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    copyDataToVulkanBuffer(device, stageBuffer.memory, size, data);      

    // Copy data to ACTUAL buffer using queue
    copyBufferToVulkanBuffer(device, commandPool, graphicsQueue, stageBuffer, dst, size);

    // Cleanup staging buffer    
    cleanupVulkanBuffer(device, stageBuffer);
}

void cleanupVulkanBuffer(vk::Device &device, VulkanBuffer &data) {
    device.destroyBuffer(data.buffer);
    device.freeMemory(data.memory);
}
