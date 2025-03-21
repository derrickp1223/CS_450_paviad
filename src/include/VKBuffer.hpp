#pragma once
#include <iostream>
#include <fstream>
#include <vulkan/vulkan.hpp>
#include "VKUtility.hpp"

using namespace std;

struct VulkanBuffer {
    vk::Buffer buffer;
    vk::DeviceMemory memory;
};

unsigned int findMemoryType(unsigned int typeFilter, 
                            vk::MemoryPropertyFlags properties, 
                            vk::PhysicalDevice physicalDevice);

VulkanBuffer createVulkanBuffer(vk::PhysicalDevice &physicalDevice,
                                vk::Device &device,
                                vk::DeviceSize size,
                                vk::BufferUsageFlags usage,
                                vk::MemoryPropertyFlags properties);
void copyDataToVulkanBuffer(vk::Device &device, vk::DeviceMemory memory, 
                            size_t bufferSize, void *hostData);
void copyDataToVulkanBufferViaStaging(  vk::PhysicalDevice &physicalDevice,
                                        vk::Device &device, 
                                        vk::CommandPool &commandPool,
                                        vk::Queue &graphicsQueue,
                                        VulkanBuffer &dst, 
                                        vk::DeviceSize size,
                                        void *data);
void copyBufferToVulkanBuffer(  vk::Device &device, vk::CommandPool &commandPool,
                                vk::Queue &graphicsQueue,
                                VulkanBuffer &src, VulkanBuffer &dst, vk::DeviceSize size);
void cleanupVulkanBuffer(vk::Device &device, VulkanBuffer &data);


