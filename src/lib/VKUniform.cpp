#include "VKUniform.hpp"

vk::DescriptorSetLayout createUniformDescriptorSetLayout(VulkanInitData &vkInitData) {
    // Create bindings
    vector<vk::DescriptorSetLayoutBinding> allBindings = {
        vk::DescriptorSetLayoutBinding(
            0,
            vk::DescriptorType::eUniformBuffer,
            1,
            vk::ShaderStageFlagBits::eVertex,
            nullptr
        ),
        vk::DescriptorSetLayoutBinding(
            1,
            vk::DescriptorType::eUniformBuffer,
            1,
            vk::ShaderStageFlagBits::eFragment,
            nullptr
        )
    };

    // Create layout
    vk::DescriptorSetLayout descriptorSetLayout
         = vkInitData.device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo({}, allBindings));

    // Return layout
    return descriptorSetLayout;    
}

void cleanupUniformDescriptorSetLayout(VulkanInitData &vkInitData, vk::DescriptorSetLayout &layout) {
    vkInitData.device.destroyDescriptorSetLayout(layout);
}

UBOData createVulkanUniformBufferData(vk::Device &device,
                                vk::PhysicalDevice &physicalDevice,
                                size_t bufferSize, 
                                int maxFramesInFlights) {

    // Create the struct and allocate space
    UBOData data;

    data.bufferData.resize(maxFramesInFlights);    
    data.mapped.resize(maxFramesInFlights);

    for (unsigned int i = 0; i < maxFramesInFlights; i++) {
        data.bufferData[i] = createVulkanBuffer(
                                physicalDevice,
                                device,
                                bufferSize,
                                vk::BufferUsageFlagBits::eUniformBuffer,
                                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        // Keep the memory mapped
        vkMapMemory(device, data.bufferData[i].memory, 0, bufferSize, 0, &data.mapped[i]);
    }

    return data;
}

void cleanupVulkanUniformBufferData(vk::Device &device, UBOData &uboData) {
    for(unsigned int i = 0; i < uboData.bufferData.size(); i++) {
        cleanupVulkanBuffer(device, uboData.bufferData[i]);
    }
    uboData.bufferData.clear();
    uboData.mapped.clear();  
}