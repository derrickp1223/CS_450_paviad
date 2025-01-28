#include "VKSetup.hpp"

///////////////////////////////////////////////////////////////////////////////
// GLFW (for Vulkan)
///////////////////////////////////////////////////////////////////////////////

GLFWwindow* createGLFWWindow(   string windowName, 
                                int windowWidth, int windowHeight,
                                bool isWindowResizable) {
        // Initialize GLFW as usual
        glfwInit();

        // Do NOT create an OpenGL context
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        // Disable resizing for now
        glfwWindowHint(GLFW_RESIZABLE, isWindowResizable);

        // Create window
        GLFWwindow *window = glfwCreateWindow(windowWidth, windowHeight, windowName.c_str(), nullptr, nullptr);

        // Return window
        return window;
}

void cleanupGLFWWindow(GLFWwindow *window) {
    glfwDestroyWindow(window);
    glfwTerminate();
}

///////////////////////////////////////////////////////////////////////////////
// Vulkan Boiletplate Setup (using vk-bootstrap and VulkanHPP)
///////////////////////////////////////////////////////////////////////////////

bool initVulkanBootstrap(string appName, GLFWwindow *window, VulkanInitData &vkInitData) {

    // Store window for reference
    vkInitData.window = window;

    ///////////////////////////////////////////////////////////////////////////
    // INSTANCE
    ///////////////////////////////////////////////////////////////////////////

    // Create vk-bootstrap instance
    vkb::InstanceBuilder builder;
    // Build the Vulkan instance
    auto instRet = builder.set_app_name(appName.c_str())
                        .set_engine_name("Forge Engine")
                        .request_validation_layers()
                        .use_default_debug_messenger()
                        .build();

    // Did we succeed?
    if(!instRet) {
        cerr << "initVulkanBootstrap: Failed to create Vulkan instance." << endl;
        cerr << "Error: " << instRet.error().message() << endl;
        return false;
    }

    // Get the VKInstance
    vkb::Instance vkbInstance = instRet.value();
    vkInitData.bootInstance = vkbInstance;

    // Convert to vk::Instance
    vkInitData.instance = vk::Instance { vkbInstance.instance };
    
    ///////////////////////////////////////////////////////////////////////////
    // SURFACE
    ///////////////////////////////////////////////////////////////////////////

    // Create a window surface
    VkSurfaceKHR surface = nullptr;
    VkResult surfErr = glfwCreateWindowSurface(vkbInstance.instance, window, NULL, &surface);
    if(surfErr != VK_SUCCESS) {
        cerr << "initVulkanBootstrap: Failed to create window surface." << endl;
        cerr << "Error: " << surfErr << endl;
        return false;
    }

    // Convert to vk::SurfaceKHR
    vkInitData.surface = vk::SurfaceKHR { surface };
    
    ///////////////////////////////////////////////////////////////////////////
    // DEVICE
    ///////////////////////////////////////////////////////////////////////////

    // Set up desired features
    vk::PhysicalDeviceFeatures requiredDeviceFeatures {};
    requiredDeviceFeatures.samplerAnisotropy = true;

    // Select physical device
    vkb::PhysicalDeviceSelector selector { vkbInstance };
    auto physRet = selector.set_surface(surface)
                        .set_minimum_version(1,1) // require at least a Vulkan 1.1 device
                        //.require_dedicated_transfer_queue()
                        .set_required_features(requiredDeviceFeatures)
                        .select();

    // Check for device selection
    if(!physRet) {
        cerr << "initVulkanBootstrap: Failed to select Vulkan Physical Device." << endl;
        cerr << "Error: " << physRet.error().message() << endl;
        return false;
    }

    // Get physical device
    vkInitData.physicalDevice = vk::PhysicalDevice { physRet.value().physical_device };

    // Create a vkb::Device (which has a VkDevice inside it)
    vkb::DeviceBuilder deviceBuilder { physRet.value() };
    auto devRet = deviceBuilder.build();

    if(!devRet) {
        cerr << "initVulkanBootstrap: Failed to create Vulkan Device." << endl;
        cerr << "Error: " << devRet.error().message() << endl;
        return false;
    }

    vkb::Device vkbDevice = devRet.value();

    // Convert to a vk::Device
    vkInitData.device = vk::Device { vkbDevice.device };

    // Store reference to bootstrap device for later
    vkInitData.bootDevice = vkbDevice;
    
    ///////////////////////////////////////////////////////////////////////////
    // QUEUES
    ///////////////////////////////////////////////////////////////////////////

    // Get the graphics queue 
    auto graphicsQueueRet = vkbDevice.get_queue(vkb::QueueType::graphics);
    if(!graphicsQueueRet) {
        cerr << "initVulkanBootstrap: Failed to get graphics queue." << endl;
        cerr << "Error: " << graphicsQueueRet.error().message() << endl;
        return false;
    }
    
    vkInitData.graphicsQueue.queue = vk::Queue { graphicsQueueRet.value() };
    vkInitData.graphicsQueue.index = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    
    // Get present queue
    auto presentQueueRet = vkbDevice.get_queue(vkb::QueueType::present);
    if(!presentQueueRet) {
        cerr << "initVulkanBootstrap: Failed to get present queue." << endl;
        cerr << "Error: " << presentQueueRet.error().message() << endl;
        return false;
    }

    vkInitData.presentQueue.queue = vk::Queue { presentQueueRet.value() };
    vkInitData.presentQueue.index = vkbDevice.get_queue_index(vkb::QueueType::present).value();
    
    ///////////////////////////////////////////////////////////////////////////
    // SWAPCHAIN
    ///////////////////////////////////////////////////////////////////////////

    if(!createVulkanSwapchain(vkInitData)) {
        return false;
    }

    // Success!
    return true;
}

bool createVulkanSwapchain(VulkanInitData &vkInitData) {
    // Create swapchain
    vkb::SwapchainBuilder swapchainBuilder { vkInitData.bootDevice };

    // Make sure it stores values in linear space, BUT
    // does gamma correction during presentation
    VkSurfaceFormatKHR desiredFormat;
    desiredFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
    desiredFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    auto swapRet = swapchainBuilder.set_desired_format(desiredFormat).build();

    if(!swapRet) {
        cerr << "initVulkanBootstrap: Failed to create swapchain." << endl;
        cerr << "Error: " << swapRet.error().message() << endl;
        return false;
    }
    
    vkb::Swapchain vkSwapchain = swapRet.value();

    // Convert to our data structure so we use VulkanHpp consistently
    vkInitData.swapchain.chain = vk::SwapchainKHR { vkSwapchain.swapchain };
    vkInitData.swapchain.format = vk::Format(vkSwapchain.image_format);
    vkInitData.swapchain.extent = vk::Extent2D { vkSwapchain.extent };
    
    vector<VkImageView> vkViews = vkSwapchain.get_image_views().value();
    for(unsigned int i = 0; i < vkViews.size(); i++) {
        vkInitData.swapchain.views.push_back(vk::ImageView { vkViews.at(i) });
    }

    return true;
}

void cleanupVulkanSwapchain(VulkanInitData &vkInitData) {
    for(unsigned int i = 0; i < vkInitData.swapchain.views.size(); i++) {
        vkInitData.device.destroyImageView(vkInitData.swapchain.views.at(i));
    }
    vkInitData.swapchain.views.clear();    
    vkInitData.device.destroySwapchainKHR(vkInitData.swapchain.chain);
}

void cleanupVulkanBootstrap(VulkanInitData &vkInitData) {
    
    for(unsigned int i = 0; i < vkInitData.swapchain.views.size(); i++) {
        vkInitData.device.destroyImageView(vkInitData.swapchain.views.at(i));
    }
    vkInitData.swapchain.views.clear();    
    vkInitData.device.destroySwapchainKHR(vkInitData.swapchain.chain);

    vkInitData.device.destroy();
    vkInitData.instance.destroySurfaceKHR(vkInitData.surface);    
    
    vkb::destroy_instance(vkInitData.bootInstance);    
}
