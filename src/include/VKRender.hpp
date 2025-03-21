#pragma once
#include "VKSetup.hpp"
#include "VKImage.hpp"
#include "VKMesh.hpp"

///////////////////////////////////////////////////////////////////////////////
// Vulkan Render Structs
///////////////////////////////////////////////////////////////////////////////

struct VulkanInitRenderParams {
    string vertSPVFilename;
    string fragSPVFilename;
};

struct VulkanPipelineData {
    vk::PipelineCache cache;
    vk::PipelineLayout pipelineLayout; // Necessary for passing in uniform variables
    vk::Pipeline graphicsPipeline;
    vector<vk::DescriptorSetLayout> descriptorSetLayouts;
};

struct VulkanFrameData {
    vk::CommandBuffer commandBuffer;

    vk::Semaphore imageAvailableSemaphore;
    vk::Semaphore renderFinishedSemaphore;
    vk::Fence inFlightFence;
};

///////////////////////////////////////////////////////////////////////////////
// Vulkan Render Engine
///////////////////////////////////////////////////////////////////////////////

class VulkanRenderEngine {
    protected:    
        const int MAX_FRAMES_IN_FLIGHT = 2;

        bool initialized = false;

        VulkanInitData &vkInitData;     // Reference to init data; do NOT deallocate here!

        vk::RenderPass renderPass;
        VulkanPipelineData pipelineData;

        VulkanImage depthImage;
        vector<vk::Framebuffer> framebuffers;
        atomic<bool> frameBufferResized = false;

        vk::CommandPool commandPool;
        unsigned int currentImage = 0;
        vector<VulkanFrameData> allFrameData;

    public:        
        ///////////////////////////////////////////////////////////////////////////////
        // Constructors and Destructor
        ///////////////////////////////////////////////////////////////////////////////

        VulkanRenderEngine(VulkanInitData &vkInitData);
        virtual ~VulkanRenderEngine();

        virtual bool initialize(VulkanInitRenderParams *params);

        ///////////////////////////////////////////////////////////////////////////////
        // Per-frame drawing function
        ///////////////////////////////////////////////////////////////////////////////

        virtual void drawFrame(void *userData);

        ///////////////////////////////////////////////////////////////////////////////
        // Getters
        ///////////////////////////////////////////////////////////////////////////////

        vk::CommandPool& getCommandPool();
        
        ///////////////////////////////////////////////////////////////////////////////
        // Swap chain recreation
        ///////////////////////////////////////////////////////////////////////////////

        void recreateSwapChain();
        void notifyFrameResize();

    protected:
        ///////////////////////////////////////////////////////////////////////////////
        // Vulkan render pass
        ///////////////////////////////////////////////////////////////////////////////

        virtual vk::RenderPass createVulkanRenderPass(VulkanImage &depthImage);
        virtual void cleanupVulkanRenderPass(vk::RenderPass &pass);

        ///////////////////////////////////////////////////////////////////////////////
        // Vulkan attributes and uniform data layout
        ///////////////////////////////////////////////////////////////////////////////

        virtual AttributeDescData getAttributeDescData();
        virtual vector<vk::DescriptorSetLayout> getDescriptorSetLayouts();
        virtual vector<vk::PushConstantRange> getPushConstantRanges();

        ///////////////////////////////////////////////////////////////////////////////
        // Vulkan pipeline
        ///////////////////////////////////////////////////////////////////////////////

        virtual VulkanPipelineData createVulkanPipelineData(vk::RenderPass &renderPass, 
                                                        string vertSPVFilename, 
                                                        string fragSPVFilename);
        virtual void cleanupVulkanPipelineData(VulkanPipelineData &pipelineData); 

        ///////////////////////////////////////////////////////////////////////////////
        // Vulkan framebuffers
        ///////////////////////////////////////////////////////////////////////////////

        virtual vector<vk::Framebuffer> createVulkanFramebuffers(   vk::RenderPass &renderPass,
                                                                    VulkanImage &depthImage);
        virtual void cleanupVulkanFramebuffers(vector<vk::Framebuffer> &framebuffers);  
        
        ///////////////////////////////////////////////////////////////////////////////
        // Vulkan command buffer and rendering
        ///////////////////////////////////////////////////////////////////////////////
                
        virtual void recordCommandBuffer(   void *userData, 
                                            vk::CommandBuffer &commandBuffer, 
                                            unsigned int imageIndex);
};

