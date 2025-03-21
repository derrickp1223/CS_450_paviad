#pragma once
#include <iostream>
#include <string>
#include <vulkan/vulkan.hpp>
#include "VkBootstrap.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
using namespace std;

struct VulkanSwapChain {
    vk::SwapchainKHR chain;
    //vector<vk::Image> images;
    vector<vk::ImageView> views;
    vk::Extent2D extent;
    vk::Format format;
};

struct VulkanQueue {
    vk::Queue queue;
    unsigned int index;
};

struct VulkanInitData {
    vkb::Instance bootInstance; // Cleaned up explicitly
    vkb::Device bootDevice;     // Do NOT clean up explicitly
    GLFWwindow *window;         // Do NOT clean up explicitly

    vk::Instance instance;      // Do NOT clean up explicitly 
    vk::SurfaceKHR surface;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;    
    VulkanQueue graphicsQueue;
    VulkanQueue presentQueue;
    VulkanSwapChain swapchain;
};

GLFWwindow* createGLFWWindow(string windowName, int windowWidth, int windowHeight, bool isWindowResizable = true);
void cleanupGLFWWindow(GLFWwindow *window);
bool initVulkanBootstrap(string appName, GLFWwindow *window, VulkanInitData &vkInitData);
bool createVulkanSwapchain(VulkanInitData &vkInitData);
void cleanupVulkanSwapchain(VulkanInitData &vkInitData);
void cleanupVulkanBootstrap(VulkanInitData &vkInitData);
