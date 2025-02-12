#include <iostream>
#include <string>
#include <vulkan/vulkan.hpp>
#include "VkBootstrap.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "VKUtility.hpp"
using namespace std;

int main(int argc, char **argv) {

    if(argc >= 2) {
        string filename = string(argv[1]);
        cout << "FILENAME: " << filename << endl;
    }

    cout << "BEGIN THE EXERCISE!" << endl;

    vkb::InstanceBuilder instBuilder;
    auto instRes = instBuilder.request_validation_layers()
                            .use_default_debug_messenger()
                            .build();
    vkb::Instance vkbInstance = instRes.value();
    vk::Instance instance = vk::Instance { vkbInstance.instance };

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "BEHOLD", 
                                            nullptr, nullptr);

    VkSurfaceKHR vkSurface = nullptr;
    auto surfRes = glfwCreateWindowSurface(vkbInstance.instance,
                                            window, nullptr,
                                            &vkSurface);
    vk::SurfaceKHR surface = vk::SurfaceKHR { vkSurface };

    vkb::PhysicalDeviceSelector phySelect { vkbInstance };
    auto phyRet = phySelect.set_surface(vkSurface)
                            .set_minimum_version(1,1)
                            .select();
    vkb::PhysicalDevice vkbPhyDevice = phyRet.value();
    vk::PhysicalDevice phyDevice
         = vk::PhysicalDevice { vkbPhyDevice.physical_device };

    cout << "DEVICE: " << phyDevice.getProperties().deviceName << endl;

    vkb::DeviceBuilder devBuilder { vkbPhyDevice };
    auto devRet = devBuilder.build();
    vkb::Device vkbDevice = devRet.value();
    vk::Device device = vk::Device { vkbDevice.device };

    auto graphQRet = vkbDevice.get_queue(vkb::QueueType::graphics);
    auto presQRet = vkbDevice.get_queue(vkb::QueueType::present);

    vk::Queue graphQueue = vk::Queue { graphQRet.value() };
    vk::Queue presentQueue = vk::Queue { presQRet.value() };

    auto graphIndex 
        = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    auto presentIndex
        = vkbDevice.get_queue_index(vkb::QueueType::present).value();

    cout << "Graphics queue: " << graphIndex << endl;
    cout << "Present queue: " << presentIndex << endl;

    vkb::SwapchainBuilder swapBuilder { vkbDevice };
    VkSurfaceFormatKHR surfaceFormat;
    surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
    surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    auto swapRet = swapBuilder
        .set_desired_format(surfaceFormat)
        .build();
    vkb::Swapchain vkbSwapchain = swapRet.value();

    vk::SwapchainKHR swapchain
        = vk::SwapchainKHR { vkbSwapchain.swapchain };
    vk::Format swapformat { vkbSwapchain.image_format };
    vk::Extent2D swapextent { vkbSwapchain.extent };
    vector<VkImageView> oldViews
        = vkbSwapchain.get_image_views().value();
    vector<vk::ImageView> swapviews;
    for(int i = 0; i < oldViews.size(); i++) {
        swapviews.push_back(
            vk::ImageView { oldViews.at(i)});
    }

    vector<vk::AttachmentDescription> attachDesc;
    attachDesc.push_back(vk::AttachmentDescription(
        {},
        swapformat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR
    ));

    vk::AttachmentReference colorAttachRef(
        0, vk::ImageLayout::eColorAttachmentOptimal
    );

    vk::SubpassDescription subpassDesc(
        vk::SubpassDescriptionFlags(),
        vk::PipelineBindPoint::eGraphics,
        {}, colorAttachRef, {}, {}
    );

    vk::RenderPass pass
        = device.createRenderPass(
            vk::RenderPassCreateInfo(
                {}, attachDesc, subpassDesc
            )
        );

    auto vertSrc = readBinaryFile(
        "build/compiledshaders/ProfExercises03/shader.vert.spv");
    auto fragSrc = readBinaryFile(
        "build/compiledshaders/ProfExercises03/shader.frag.spv");
        
    // CREATION TODO

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // RENDER TODO
    }

    // CLEANUP TODO
    device.destroyRenderPass(pass);
    for(int i = 0; i < swapviews.size(); i++) {
        device.destroyImageView(swapviews.at(i));
    }
    swapviews.clear();
    device.destroySwapchainKHR(swapchain);
    device.destroy();
    instance.destroySurfaceKHR(surface);
    glfwDestroyWindow(window);
    glfwTerminate();
    vkb::destroy_instance(vkbInstance);

    return 0;
}