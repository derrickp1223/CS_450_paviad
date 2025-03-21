#pragma once
#include "VKSetup.hpp"
#include "VKUtility.hpp"
#include "VKBuffer.hpp"

struct VulkanImage {
    vk::Image image;
    vk::DeviceMemory memory;
    vk::ImageView view;
    vk::Format format;
};

VulkanImage createVulkanImage( VulkanInitData &vkInitData, int width, int height, 
                                vk::Format format, vk::ImageUsageFlags usage,
                                vk::ImageAspectFlags aspectFlags);
                                
VulkanImage createVulkanDepthImage(VulkanInitData &vkInitData, int width, int height);

void transitionVulkanImageLayout(   VulkanInitData &vkInitData, 
                                    vk::CommandPool &commandPool,
                                    VulkanImage &vkImage, 
                                    vk::ImageLayout oldLayout,
                                    vk::ImageLayout newLayout);

void cleanupVulkanImage(VulkanInitData &vkInitData, VulkanImage &vkImage);

