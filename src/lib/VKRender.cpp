#include "VKRender.hpp"

///////////////////////////////////////////////////////////////////////////////
// Constructors and Destructor
///////////////////////////////////////////////////////////////////////////////

VulkanRenderEngine::VulkanRenderEngine(VulkanInitData &vkInitData) : vkInitData(vkInitData) {}

bool VulkanRenderEngine::initialize(VulkanInitRenderParams *params) {

    if(!initialized) {
    
        // Create depth image    
        this->depthImage = createVulkanDepthImage(  vkInitData, 
                                                    vkInitData.swapchain.extent.width, 
                                                    vkInitData.swapchain.extent.height);

        // Create render pass
        this->renderPass = createVulkanRenderPass(this->depthImage);

        // Create pipeline
        this->pipelineData = createVulkanPipelineData(  this->renderPass,
                                                        params->vertSPVFilename, 
                                                        params->fragSPVFilename);

        // Create frame buffers
        this->framebuffers = createVulkanFramebuffers(this->renderPass, this->depthImage);

        // Grab device and graphics queue index
        vk::Device device = vkInitData.device; 
        unsigned int graphicsQueueIndex = vkInitData.graphicsQueue.index; 

        // Create command pool
        this->commandPool = createVulkanCommandPool(device, graphicsQueueIndex);     

        // For each possible frame in flight
        for(unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {   
            // Start with struct
            VulkanFrameData frameData;

            // Create command buffers            
            frameData.commandBuffer = createVulkanCommandBuffer(device, this->commandPool);

            // Create sync objects
            frameData.imageAvailableSemaphore = createVulkanSemaphore(device);
            frameData.renderFinishedSemaphore = createVulkanSemaphore(device);
            frameData.inFlightFence = createVulkanFence(device);

            // Add to list
            this->allFrameData.push_back(frameData);
        }

        // We are now initialized
        initialized = true;
    }
    else {
        cout << "WARNING: Render engine already initialized." << endl;
    }

    return initialized;
}

VulkanRenderEngine::~VulkanRenderEngine() {
    if(initialized) {
        for(unsigned int i = 0; i < this->allFrameData.size(); i++) {
            cleanupVulkanFence(vkInitData.device, this->allFrameData.at(i).inFlightFence);
            cleanupVulkanSemaphore(vkInitData.device, this->allFrameData.at(i).renderFinishedSemaphore);
            cleanupVulkanSemaphore(vkInitData.device, this->allFrameData.at(i).imageAvailableSemaphore);
        }
        
        cleanupVulkanCommandPool(vkInitData.device, this->commandPool);

        cleanupVulkanFramebuffers(this->framebuffers);
        cleanupVulkanPipelineData(this->pipelineData);    
        cleanupVulkanRenderPass(this->renderPass);
        cleanupVulkanImage(vkInitData, this->depthImage);
    }
    else {
        cout << "WARNING: Render engine NOT initialized." << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Getters
///////////////////////////////////////////////////////////////////////////////

vk::CommandPool& VulkanRenderEngine::getCommandPool() {
    return this->commandPool;
}

///////////////////////////////////////////////////////////////////////////////
// Swap chain recreation
///////////////////////////////////////////////////////////////////////////////

void VulkanRenderEngine::recreateSwapChain() {    
    // Wait until device is idle
    vkInitData.device.waitIdle();

    // Cleanup framebuffers
    cleanupVulkanFramebuffers(this->framebuffers);

    // Cleanup swapchain data
    cleanupVulkanSwapchain(vkInitData);

    // Cleanup depth image
    cleanupVulkanImage(vkInitData, this->depthImage);   
    
    // (Re)create swap chain and image views
    createVulkanSwapchain(vkInitData);

    // (Re)create depth image
    this->depthImage = createVulkanDepthImage(  vkInitData, 
                                                vkInitData.swapchain.extent.width, 
                                                vkInitData.swapchain.extent.height);

    // (Re)create frame buffers
    this->framebuffers = createVulkanFramebuffers(this->renderPass, this->depthImage);
}

///////////////////////////////////////////////////////////////////////////////
// Vulkan render pass
///////////////////////////////////////////////////////////////////////////////

vk::RenderPass VulkanRenderEngine::createVulkanRenderPass(VulkanImage &depthImage) {

    // Create attachment for color and depth
    vector<vk::AttachmentDescription> attachmentDescriptions;

    // Color first
    attachmentDescriptions.push_back(vk::AttachmentDescription(
        {},
        vkInitData.swapchain.format,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,       // Clear buffer to constant value on load
        vk::AttachmentStoreOp::eStore,      // Store values (so we can see what we render :)
        vk::AttachmentLoadOp::eDontCare,    // Don't care about stencil buffer
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,        // Initially undefined before presentation
        vk::ImageLayout::ePresentSrcKHR     // Present appropriate to surface
    ));
    
    // Depth attachment
    attachmentDescriptions.push_back(vk::AttachmentDescription(
        {},
        depthImage.format,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,       // Clear buffer to constant value on load
        vk::AttachmentStoreOp::eDontCare,   // We don't need to see this later
        vk::AttachmentLoadOp::eDontCare,    // Don't care about stencil buffer
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,        // Initially undefined before presentation
        vk::ImageLayout::eDepthStencilAttachmentOptimal     // Present as depth buffer
    ));

    // Which images to attach to this subpass
    // Refers to: layout(location = 0) out vec4 outColor
    vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference depthAttachmentRef(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
   
    // Describe the actual subpass
    vk::SubpassDescription subpassDescription(        
        vk::SubpassDescriptionFlags(),
        vk::PipelineBindPoint::eGraphics,
        {},
        colorAttachmentRef,
        {},
        &depthAttachmentRef
    );  

    // Make the ACTUAL render pass
    return vkInitData.device.createRenderPass(vk::RenderPassCreateInfo(
        {},
        attachmentDescriptions,
        subpassDescription
    ));      
}

void VulkanRenderEngine::cleanupVulkanRenderPass(vk::RenderPass &pass) {    
    vkInitData.device.destroyRenderPass(pass);
}

///////////////////////////////////////////////////////////////////////////////
// Vulkan attributes and uniform data layout
///////////////////////////////////////////////////////////////////////////////

AttributeDescData VulkanRenderEngine::getAttributeDescData() {
    // Create struct
    AttributeDescData attribDescData;

    // Get vertex binding description
    attribDescData.bindDesc = vk::VertexInputBindingDescription(0, sizeof(SimpleVertex), vk::VertexInputRate::eVertex);

    // Get attribute descriptions
    attribDescData.attribDesc.clear();

    // POSITION
    attribDescData.attribDesc.push_back(vk::VertexInputAttributeDescription(
        0, // location
        0, // binding
        vk::Format::eR32G32B32Sfloat,  // format
        offsetof(SimpleVertex, pos) // offset
    ));

    // COLOR
    attribDescData.attribDesc.push_back(vk::VertexInputAttributeDescription(
        1, // location
        0, // binding
        vk::Format::eR32G32B32Sfloat,  // format
        offsetof(SimpleVertex, color) // offset
    ));

    // Return struct
    return attribDescData;
}

vector<vk::DescriptorSetLayout> VulkanRenderEngine::getDescriptorSetLayouts() {
    return {};
}

vector<vk::PushConstantRange> VulkanRenderEngine::getPushConstantRanges() {
    return {};
}

///////////////////////////////////////////////////////////////////////////////
// Vulkan pipeline
///////////////////////////////////////////////////////////////////////////////

VulkanPipelineData VulkanRenderEngine::createVulkanPipelineData(      
    vk::RenderPass &renderPass, 
    string vertSPVFilename, 
    string fragSPVFilename) {
       
    // Set up data
    VulkanPipelineData data;

    // Load up BYTECODE shader files
    auto vertShaderCode = readBinaryFile(vertSPVFilename);
    auto fragShaderCode = readBinaryFile(fragSPVFilename);

    // Compiling/linking to GPU machine code doesn't happen until graphics pipeline created.
    // Once the pipeline is created, we will be able to destroy these modules safely.
    vk::ShaderModule vertShaderModule = createVulkanShaderModule(vkInitData.device, vertShaderCode);
    vk::ShaderModule fragShaderModule = createVulkanShaderModule(vkInitData.device, fragShaderCode);

    // Assign VERTEX SHADER to appropriate stage
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo(
        {}, vk::ShaderStageFlagBits::eVertex, vertShaderModule, "main");  
   
    // Assign FRAGMENT SHADER to appropriate stage
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo(
        {}, vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main");  

    // Combine them
    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // Get the attribute description data
    AttributeDescData attribDescData = getAttributeDescData(); 
    
    // Set up how attributes are arranged
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
        {}, attribDescData.bindDesc, attribDescData.attribDesc);
        
    // Render a regular triangle list
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly({}, vk::PrimitiveTopology::eTriangleList, false);
    
    // Set viewport (startx, starty, width, height, mindepth, maxdepth)
    vk::Viewport viewport(0, 0, 
                            (float)vkInitData.swapchain.extent.width, 
                            (float)vkInitData.swapchain.extent.height, 
                            0.0f, 1.0f);

    // Set scissors (if we wanted to only draw part of the screen)
    // Using full screen here
    vk::Rect2D scissor({0,0}, vkInitData.swapchain.extent);
          
    // Identify properties that you want to be able to change dynamically
    // WITHOUT recreating the whole pipeline
    vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor        
    };

    vk::PipelineDynamicStateCreateInfo dynamicState({}, dynamicStates);  

    vk::PipelineViewportStateCreateInfo viewportState({}, viewport, scissor);
    
    // Set RASTERIZER stage values
    // Defaults to:
    // - discard fragments outside near/far planes
    // - polygon fill mode    

    vk::PipelineRasterizationStateCreateInfo rasterizer {};
    // Change the following:
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eNone; // eBack;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise; 
                
    // Set up BLENDING
    // Basically no blending here
    // Per frame buffer...
    vk::PipelineColorBlendAttachmentState colorBlendAttachment {};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    
    // Set up depth testing
    vk::PipelineDepthStencilStateCreateInfo depthStencil(
        {},
        true,                   // Enable depth testing
        true,                   // Enable depth writing
        vk::CompareOp::eLess,   // Lower depth = closer = keep
        false,                  // Not putting bounds on depth test
        false, {}, {}          // Not using stencil test
    );

    // Global blend settings
    vk::PipelineColorBlendStateCreateInfo colorBlending({}, false, vk::LogicOp::eCopy, colorBlendAttachment);
        
    // Get the pipeline creation info
    data.descriptorSetLayouts = getDescriptorSetLayouts();
    vector<vk::PushConstantRange> pushConstantRanges = getPushConstantRanges();

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
        {},                       
        data.descriptorSetLayouts,     
        pushConstantRanges     
    );

    // Create the pipeline layout
    data.pipelineLayout = vkInitData.device.createPipelineLayout(pipelineLayoutInfo);

    // Not doing multisample AA
    vk::PipelineMultisampleStateCreateInfo multisample({}, vk::SampleCountFlagBits::e1);

    // Create pipeline cache
    data.cache = vkInitData.device.createPipelineCache( vk::PipelineCacheCreateInfo());

    // CREATE ACTUAL PIPELINE
    vk::GraphicsPipelineCreateInfo pipelineInfo(vk::PipelineCreateFlags(),
                                                shaderStages,
                                                &vertexInputInfo,
                                                &inputAssembly,
                                                0,
                                                &viewportState,
                                                &rasterizer,
                                                &multisample,
                                                &depthStencil,
                                                &colorBlending,
                                                &dynamicState,
                                                data.pipelineLayout,
                                                renderPass);    
    
    auto ret = vkInitData.device.createGraphicsPipeline(data.cache, pipelineInfo);

    if (ret.result != vk::Result::eSuccess) {
        throw runtime_error("Failed to create graphics pipeline!");
    }

    // Set pipeline
    data.graphicsPipeline = ret.value;

    // Cleanup modules
    vkInitData.device.destroyShaderModule(fragShaderModule);
    vkInitData.device.destroyShaderModule(vertShaderModule);
    
    // Return data
    return data;
}

void VulkanRenderEngine::cleanupVulkanPipelineData(VulkanPipelineData &pipelineData) {
    for(unsigned int i = 0; i < pipelineData.descriptorSetLayouts.size(); i++) {
        vkInitData.device.destroyDescriptorSetLayout(pipelineData.descriptorSetLayouts.at(i));
    }

    vkInitData.device.destroyPipelineCache(pipelineData.cache);
    vkInitData.device.destroyPipelineLayout(pipelineData.pipelineLayout);
    vkInitData.device.destroyPipeline(pipelineData.graphicsPipeline);
}

///////////////////////////////////////////////////////////////////////////////
// Vulkan framebuffers
///////////////////////////////////////////////////////////////////////////////

vector<vk::Framebuffer> VulkanRenderEngine::createVulkanFramebuffers(
                                                    vk::RenderPass &renderPass, 
                                                    VulkanImage &depthImage) {    

    // Create vector and allocate space
    vector<vk::Framebuffer> framebuffers;    
    framebuffers.resize(vkInitData.swapchain.views.size());

    // For each image view...
    for (size_t i = 0; i < vkInitData.swapchain.views.size(); i++) {
        vector<vk::ImageView> attachments = {   vkInitData.swapchain.views.at(i), 
                                                depthImage.view };
        framebuffers[i] = vkInitData.device.createFramebuffer(vk::FramebufferCreateInfo({}, 
                                                    renderPass, attachments, 
                                                    vkInitData.swapchain.extent.width, 
                                                    vkInitData.swapchain.extent.height, 1));
    }

    // Return list of framebuffers
    return framebuffers;
}

void VulkanRenderEngine::cleanupVulkanFramebuffers(vector<vk::Framebuffer> &framebuffers) {
    for (auto framebuffer : framebuffers) {
        vkInitData.device.destroyFramebuffer(framebuffer);
    }
    framebuffers.clear();        
}

///////////////////////////////////////////////////////////////////////////////
// Vulkan command buffer and rendering
///////////////////////////////////////////////////////////////////////////////

void VulkanRenderEngine::recordCommandBuffer(   void *userData,
                                                vk::CommandBuffer &commandBuffer,
                                                unsigned int frameIndex) {


    
    // Void data is assumed to be vector of meshes only
    vector<VulkanMesh> *allMeshes = static_cast<vector<VulkanMesh>*>(userData);

    // Begin commands
    commandBuffer.begin(vk::CommandBufferBeginInfo());

    // Get the extents of the buffers (since we'll use it a few times)
    vk::Extent2D extent = vkInitData.swapchain.extent;

    // Begin render pass
    array<vk::ClearValue, 2> clearValues {};
    clearValues[0].color = vk::ClearColorValue(1.0f, 1.0f, 0.7f, 1.0f);
    clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0.0f);
    
    commandBuffer.beginRenderPass(vk::RenderPassBeginInfo(
        this->renderPass, 
        this->framebuffers[frameIndex], 
        { {0,0}, extent },
        clearValues),
        vk::SubpassContents::eInline);
    
    // Bind pipeline
    commandBuffer.bindPipeline(
        vk::PipelineBindPoint::eGraphics, 
        this->pipelineData.graphicsPipeline);

    // Set up viewport and scissors
    vk::Viewport viewports[] = {{0, 0, (float)extent.width, (float)extent.height, 0.0f, 1.0f}};
    commandBuffer.setViewport(0, viewports);
    
    vk::Rect2D scissors[] = {{{0,0}, extent}};
    commandBuffer.setScissor(0, scissors);
    
    // Record our buffer (ONLY drawing first mesh)
    recordDrawVulkanMesh(commandBuffer, allMeshes->at(0));
    
    // Stop render pass
    commandBuffer.endRenderPass();
    
    // End command buffer
    commandBuffer.end();
}

void VulkanRenderEngine::notifyFrameResize() {
    frameBufferResized.store(true);
}

void VulkanRenderEngine::drawFrame(void *userData) {

    // Is the current size 0 x 0 (minimized?)
    int windowWidth = 0, windowHeight = 0;
    glfwGetFramebufferSize(vkInitData.window, &windowWidth, &windowHeight);
    if(windowWidth == 0 || windowHeight == 0) {
        return;
    }

    // Have we resized recently?
    if(frameBufferResized.load()) {        
        recreateSwapChain();
        frameBufferResized.store(false);
        return;
    }

    // Wait for this image to finish
    auto waitRes = vkInitData.device.waitForFences(1, &this->allFrameData[currentImage].inFlightFence, true, UINT64_MAX);
    if(waitRes != vk::Result::eSuccess) {
        throw runtime_error("drawFrame: Timeout while waiting for image fence!");
    }

    // Acquire a frame index from the swap chain
    auto result = vkInitData.device.acquireNextImageKHR(vkInitData.swapchain.chain, 
                                                        UINT64_MAX, 
                                                        this->allFrameData[currentImage].imageAvailableSemaphore, 
                                                        nullptr);   

    // Reset the fence since we're about to submit work
    auto resetRes = vkInitData.device.resetFences(1, &this->allFrameData[currentImage].inFlightFence);
    if(resetRes != vk::Result::eSuccess) {
        throw runtime_error("drawFrame: Failed to reset image fence!");
    }
    
    // Get actual image index for framebuffer purposes
    unsigned int frameIndex = result.value;
    
    // Record a command buffer which draws the scene onto that image
    this->allFrameData[currentImage].commandBuffer.reset();        
    recordCommandBuffer(userData, this->allFrameData[currentImage].commandBuffer, frameIndex);

    // Submit the recorded command buffer
    vk::Semaphore waitSemaphores[] = {this->allFrameData[currentImage].imageAvailableSemaphore};
    vk::Semaphore signalSemaphores[] = {this->allFrameData[currentImage].renderFinishedSemaphore};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

    vk::SubmitInfo submitInfo(
        waitSemaphores,
        waitStages,
        this->allFrameData[currentImage].commandBuffer,
        signalSemaphores);
                
    vkInitData.graphicsQueue.queue.submit(submitInfo, this->allFrameData[currentImage].inFlightFence);
        
    // Present the swap chain image
    vk::SwapchainKHR swapChains[] = {vkInitData.swapchain.chain};
    uint32_t imageIndices[] = {frameIndex};
    vk::PresentInfoKHR presentInfo(signalSemaphores, swapChains, imageIndices);
    
    try {
        auto presentRes = vkInitData.presentQueue.queue.presentKHR(presentInfo);
    }
    catch(const vk::OutOfDateKHRError& e) {
        // Recreate swap chain
        recreateSwapChain();
    }
    
    // Increment current frame for in-flight work
    currentImage = (currentImage + 1) % MAX_FRAMES_IN_FLIGHT;   
}

