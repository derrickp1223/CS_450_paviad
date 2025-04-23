#include <iostream>
#include <string>
#include <vulkan/vulkan.hpp>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "VkBootstrap.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "VKUtility.hpp"
#include "VKBuffer.hpp"
#include "VKUniform.hpp"
#include "VKImage.hpp"
#include "glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
using namespace std;

struct PointVertex {
    glm::vec3 pos = glm::vec3(0,0,0);
    glm::vec4 color = glm::vec4(1,1,1,1);
    glm::vec3 normal = glm::vec3(0,0,0);

    PointVertex() {};
    PointVertex(glm::vec3 p) { pos = p; };
    PointVertex(glm::vec3 p, glm::vec4 c) {
        pos = p;
        color = c;
    };
};

/*
vector<PointVertex> vertices = {
    {{-0.5f, -0.5f, 0.5f}},
    {{ 0.5f, -0.5f, 0.5f}},
    {{ 0.5f,  0.5f, 0.5f}},
    {{-0.5f,  0.5f, 0.5f}}
};

vector<unsigned int> indices = { 0, 2, 1, 2, 0, 3};
*/

struct PointLight {
    alignas(16) glm::vec4 pos;
    alignas(16) glm::vec4 vpos;
    alignas(16) glm::vec4 color;
};

struct UniformPush {
    alignas(16) glm::mat4 modelMat;
    alignas(16) glm::mat4 normMat;
};

struct UBOVertex {
    alignas(16) glm::mat4 viewMat;
    alignas(16) glm::mat4 projMat;
};

UBOVertex uboVertHost;

struct UBOFragment {
    PointLight light;
};

UBOFragment uboFragHost;

glm::mat4 modelMat(1.0f);
string transformString = "v";

glm::vec2 lastMousePos;
bool leftMouseDown = false;

static void mouse_button_callback(
    GLFWwindow *window,
    int button, int action, int mods
) {
    if(button == GLFW_MOUSE_BUTTON_LEFT) {
        if(action == GLFW_PRESS) {
            leftMouseDown = true;
        }
        else if(action == GLFW_RELEASE) {
            leftMouseDown = false;
        }
    }
}

static void mouse_motion_callback(
    GLFWwindow *window,
    double xpos, double ypos
) {
    cout << "MOUSE: " << xpos << "," << ypos << endl;
    glm::vec2 curMouse = glm::vec2(xpos, ypos);
    glm::vec2 diffMouse = curMouse - lastMousePos;

    int winWidth, winHeight;
    glfwGetFramebufferSize(window, &winWidth, &winHeight);
    if(winWidth != 0 && winHeight != 0) {
        diffMouse.x /= winWidth;
        diffMouse.y /= winHeight;

        float angle = diffMouse.x;
        angle *= 50.0f;

        if(leftMouseDown) {
            modelMat = glm::rotate(
                    glm::radians(angle),
                    glm::vec3(1,0,0))*modelMat;
        }
    }

    lastMousePos = curMouse;
}

void printRM(string name, glm::mat4 &M) {
    cout << name << ":" << endl;
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            cout << M[j][i] << "\t";
        }
        cout << endl;
    }
}


static void key_callback(GLFWwindow *window,
                        int key, int scancode,
                        int action, int mods) {
    if(action == GLFW_PRESS || action == GLFW_REPEAT) {
        if(key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, true);
        }
        else if(key == GLFW_KEY_SPACE) {
            modelMat = glm::mat4(1.0);
            transformString = "v";
        }
        else if(key == GLFW_KEY_Q) {
            modelMat = glm::rotate(
                glm::radians(5.0f),
                glm::vec3(0,0,1))*modelMat;
            transformString = "Rz(5)*" + transformString;
        }
        else if(key == GLFW_KEY_E) {
            modelMat = glm::rotate(
                glm::radians(-5.0f),
                glm::vec3(0,0,1))*modelMat;
            transformString = "Rz(-5)*" + transformString;
        }
        else if(key == GLFW_KEY_A) {
            modelMat = glm::rotate(
                glm::radians(5.0f),
                glm::vec3(0,1,0))*modelMat;
            transformString = "Ry(5)*" + transformString;
        }
        else if(key == GLFW_KEY_D) {
            modelMat = glm::rotate(
                glm::radians(-5.0f),
                glm::vec3(0,1,0))*modelMat;
            transformString = "Ry(-5)*" + transformString;
        }
        else if(key == GLFW_KEY_F) {
            modelMat = glm::scale(
                glm::vec3(0.8, 1, 1)
            )*modelMat;
            transformString = "S(0.8x)*" + transformString;
        }
        else if(key == GLFW_KEY_G) {
            modelMat = glm::scale(
                glm::vec3(1.25, 1, 1)
            )*modelMat;
            transformString = "S(1.25x)*" + transformString;
        }
        else if(key == GLFW_KEY_W) {
            modelMat = glm::translate(
                glm::vec3(0, 0.1, 0)
            )*modelMat;
            transformString = "T(y+0.1)*" + transformString;
        }
        else if(key == GLFW_KEY_S) {
            modelMat = glm::translate(
                glm::vec3(0, -0.1, 0)
            )*modelMat;
            transformString = "T(y-0.1)*" + transformString;
        }
        

        //cout << transformString << endl;
    }
}

void calculateTriangleNormal(
    vector<PointVertex> &vertices,
    int i0, int i1, int i2
) {
    glm::vec3 A = vertices.at(i0).pos;
    glm::vec3 B = vertices.at(i1).pos;
    glm::vec3 C = vertices.at(i2).pos;

    glm::vec3 v1 = B - A;
    glm::vec3 v2 = C - A;

    glm::vec3 N = glm::cross(v1, v2);
    N = glm::normalize(N);

    vertices.at(i0).normal += N;
    vertices.at(i1).normal += N;
    vertices.at(i2).normal += N;
}

void calculateAllNormals(
    vector<PointVertex> &vertices,
    vector<unsigned int> &indices
) {
    for(int i = 0; i < vertices.size(); i++) {
        vertices.at(i).normal = glm::vec3(0,0,0);
    }

    for(int i = 0; i < indices.size(); i+=3) {
        calculateTriangleNormal(vertices,
            indices.at(i),
            indices.at(i+1),
            indices.at(i+2));
    }

    for(int i = 0; i < vertices.size(); i++) {
        vertices.at(i).normal
        = glm::normalize(vertices.at(i).normal);
    }
}

void makeCylinder(
    vector<PointVertex> &vertices,
    vector<unsigned int> &indices,
    float length, float radius,
    int faceCnt) {

    vertices.clear();
    indices.clear();

    float angleInc
     = (2.0f*glm::pi<float>())/((float)faceCnt);

    for(int i = 0; i < faceCnt; i++) {
        PointVertex left, right;
        float x = length/2.0f;
        float angle = angleInc*i;
        float y = radius*glm::sin(angle);
        float z = radius*glm::cos(angle);

        left.pos = glm::vec3(-x, y, z);
        right.pos = glm::vec3(x, y, z);

        left.color = glm::vec4(1,0,0,1);
        right.color = glm::vec4(1,0,1,1);

        vertices.push_back(left);
        vertices.push_back(right);
    }

    int max_vert_cnt = faceCnt*2;

    for(int i = 0; i < faceCnt; i++) {
        int low_left = 2*i;
        int up_left = (2*(i+1))%max_vert_cnt;
        int low_right = low_left + 1;
        int up_right = up_left + 1;

        indices.push_back(low_left);
        indices.push_back(low_right);
        indices.push_back(up_left);

        indices.push_back(low_right);
        indices.push_back(up_right);
        indices.push_back(up_left);
    }

    calculateAllNormals(vertices, indices);
}

int main(int argc, char **argv) {

    if(argc >= 2) {
        string filename = string(argv[1]);
        cout << "FILENAME: " << filename << endl;
    }

    cout << "BEGIN THE EXERCISE!" << endl;

    uboFragHost.light.pos = glm::vec4(0,0.5,0.5,1.0);
    uboFragHost.light.color = glm::vec4(1,1,1,1);

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

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, mouse_motion_callback);
    glfwSetInputMode(window,
        GLFW_CURSOR,
        GLFW_CURSOR_DISABLED);

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
    vk::Format swapformat = vk::Format(vkbSwapchain.image_format);
    vk::Extent2D swapextent { vkbSwapchain.extent };
    vector<VkImageView> oldViews
        = vkbSwapchain.get_image_views().value();
    vector<vk::ImageView> swapviews;
    for(int i = 0; i < oldViews.size(); i++) {
        swapviews.push_back(
            vk::ImageView { oldViews.at(i)});
    }

    VulkanImage depthImage
     = createVulkanDepthImage(
        device, phyDevice,
        swapextent.width, swapextent.height);

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

    attachDesc.push_back(vk::AttachmentDescription(
        {},
        depthImage.format,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    ));

    vk::AttachmentReference colorAttachRef(
        0, 
        vk::ImageLayout::eColorAttachmentOptimal
    );

    vk::AttachmentReference depthAttachRef(
        1, 
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    );

    vk::SubpassDescription subpassDesc(
        vk::SubpassDescriptionFlags(),
        vk::PipelineBindPoint::eGraphics,
        {}, colorAttachRef, {}, &depthAttachRef
    );

    vk::RenderPass pass
        = device.createRenderPass(
            vk::RenderPassCreateInfo(
                {}, attachDesc, subpassDesc
            )
        );

    auto vertSrc = readBinaryFile(
        "build/compiledshaders/ProfExercises09/shader.vert.spv");
    auto fragSrc = readBinaryFile(
        "build/compiledshaders/ProfExercises09/shader.frag.spv");

    vk::ShaderModule vertShader = createVulkanShaderModule(device, vertSrc);
    vk::ShaderModule fragShader = createVulkanShaderModule(device, fragSrc);

    vk::PipelineShaderStageCreateInfo shaderStages[] = {
        vk::PipelineShaderStageCreateInfo(
            {}, vk::ShaderStageFlagBits::eVertex, vertShader, "main"
        ),
        vk::PipelineShaderStageCreateInfo(
            {}, vk::ShaderStageFlagBits::eFragment, fragShader, "main"
        )
    };

    vector<PointVertex> vertices;
    vector<unsigned int> indices;
    makeCylinder(vertices, indices, 1.0, 0.5, 10.0);

    vk::DeviceSize vertBufferSize = sizeof(vertices[0])*vertices.size();
    vk::DeviceSize indBufferSize = sizeof(indices[0])*indices.size();

    VulkanBuffer vkVertices
         = createVulkanBuffer(
                phyDevice, device,
                vertBufferSize, 
                vk::BufferUsageFlagBits::eVertexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible
                | vk::MemoryPropertyFlagBits::eHostCoherent);
    VulkanBuffer vkIndices
         = createVulkanBuffer(
                phyDevice, device,
                indBufferSize, 
                vk::BufferUsageFlagBits::eIndexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible
                | vk::MemoryPropertyFlagBits::eHostCoherent);
    
    copyDataToVulkanBuffer( device, 
                            vkVertices.memory, 
                            vertBufferSize,
                            vertices.data());
    copyDataToVulkanBuffer( device, 
                            vkIndices.memory, 
                            indBufferSize,
                            indices.data());

    vk::VertexInputBindingDescription vertBindDesc 
        = vk::VertexInputBindingDescription(
            0, sizeof(vertices[0]), vk::VertexInputRate::eVertex);

    vector<vk::VertexInputAttributeDescription> vertAttribDesc 
        = {
            vk::VertexInputAttributeDescription(
                0, 0, 
                vk::Format::eR32G32B32Sfloat,
                offsetof(PointVertex, pos)),

            vk::VertexInputAttributeDescription(
                1, 0, 
                vk::Format::eR32G32B32A32Sfloat,
                offsetof(PointVertex, color)),

            vk::VertexInputAttributeDescription(
                2, 0, 
                vk::Format::eR32G32B32Sfloat,
                offsetof(PointVertex, normal))
        };
    
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
        {}, vertBindDesc, vertAttribDesc
    );

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
        {}, vk::PrimitiveTopology::eTriangleList, false
    );

    vk::Viewport viewport(
        0, 0, (float)swapextent.width, (float)swapextent.height,
        0.0f, 1.0f
    );
    vk::Rect2D scissors({0,0}, swapextent);
    vk::PipelineViewportStateCreateInfo viewportInfo(
        {}, viewport, scissors
    );

    vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };
    vk::PipelineDynamicStateCreateInfo dynamicInfo(
        {}, dynamicStates
    );

    vk::PipelineRasterizationStateCreateInfo
        rasterizer {};
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode
     = vk::CullModeFlagBits::eNone;
    rasterizer.frontFace
     = vk::FrontFace::eCounterClockwise;

    vk::PipelineColorBlendAttachmentState colorBlend {};
    colorBlend.colorWriteMask = 
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB |
        vk::ColorComponentFlagBits::eA;
    vk::PipelineColorBlendStateCreateInfo colorBlending(
        {}, false, vk::LogicOp::eCopy, colorBlend
    );
    
    vk::PipelineDepthStencilStateCreateInfo depthStencil (
        {}, true, true, vk::CompareOp::eLess,
        false, false, {}, {}
    );

    vector<vk::PushConstantRange> pushRanges = {
        {vk::ShaderStageFlagBits::eVertex,
        0, sizeof(UniformPush)}
    };

    UBOData uboVertData = createVulkanUniformBufferData(
        device, phyDevice, sizeof(UBOVertex), 1
    ); 

    UBOData uboFragData = createVulkanUniformBufferData(
        device, phyDevice, sizeof(UBOFragment), 1
    );

    vector<vk::DescriptorSetLayoutBinding> allBinds = {
        vk::DescriptorSetLayoutBinding(
            0, vk::DescriptorType::eUniformBuffer,
            1, vk::ShaderStageFlagBits::eVertex
        ),
        vk::DescriptorSetLayoutBinding(
            1, vk::DescriptorType::eUniformBuffer,
            1, vk::ShaderStageFlagBits::eFragment
        )
    };

    vk::DescriptorSetLayout descSetLayout = 
        device.createDescriptorSetLayout(
            vk::DescriptorSetLayoutCreateInfo(
                {}, allBinds));
    vector<vk::DescriptorSetLayout> allDescSetLayouts = {
        descSetLayout
    };

    vector<vk::DescriptorPoolSize> poolSizes = {
        vk::DescriptorPoolSize(
            vk::DescriptorType::eUniformBuffer,
            2
        )
    };

    vk::DescriptorPool descPool 
    = device.createDescriptorPool(
        vk::DescriptorPoolCreateInfo()
            .setPoolSizes(poolSizes)
            .setMaxSets(1)
    );

    vector<vk::DescriptorSet> descSets
    = device.allocateDescriptorSets(
        vk::DescriptorSetAllocateInfo()
            .setDescriptorPool(descPool)
            .setDescriptorSetCount(1)
            .setSetLayouts(allDescSetLayouts)
    );

    vector<vk::WriteDescriptorSet> allWrites;

    vk::DescriptorBufferInfo descBufferInfo
    = vk::DescriptorBufferInfo()
        .setBuffer(uboVertData.bufferData[0].buffer)
        .setOffset(0)
        .setRange(sizeof(UBOVertex));

    vk::WriteDescriptorSet writeInfo
    = vk::WriteDescriptorSet()
        .setDstSet(descSets[0])
        .setDstBinding(0)
        .setDstArrayElement(0)
        .setDescriptorType(
            vk::DescriptorType::eUniformBuffer)
        .setDescriptorCount(1)
        .setBufferInfo(descBufferInfo);

    allWrites.push_back(writeInfo);

    vk::DescriptorBufferInfo descBufferFragInfo
    = vk::DescriptorBufferInfo()
        .setBuffer(uboFragData.bufferData[0].buffer)
        .setOffset(0)
        .setRange(sizeof(UBOFragment));

    vk::WriteDescriptorSet fragWriteInfo
    = vk::WriteDescriptorSet()
        .setDstSet(descSets[0])
        .setDstBinding(1)
        .setDstArrayElement(0)
        .setDescriptorType(
            vk::DescriptorType::eUniformBuffer)
        .setDescriptorCount(1)
        .setBufferInfo(descBufferFragInfo);

    allWrites.push_back(fragWriteInfo);

    device.updateDescriptorSets(allWrites, {});

    vk::PipelineLayoutCreateInfo layoutInfo(
        {}, allDescSetLayouts, pushRanges
    );
    vk::PipelineLayout pipelineLayout
    = device.createPipelineLayout(layoutInfo);

    vk::PipelineMultisampleStateCreateInfo multisample(
        {}, vk::SampleCountFlagBits::e1
    );

    vk::PipelineCache pipelineCache
    = device.createPipelineCache({});

    vk::GraphicsPipelineCreateInfo pipelineInfo(
        vk::PipelineCreateFlags(),
        shaderStages,
        &vertexInputInfo,
        &inputAssembly,
        0,
        &viewportInfo,
        &rasterizer,
        &multisample,
        &depthStencil,
        &colorBlending,
        &dynamicInfo,
        pipelineLayout,
        pass
    );
    auto pipeRet = device.createGraphicsPipeline(
        pipelineCache, pipelineInfo
    );
    vk::Pipeline graphicsPipeline = pipeRet.value;
    
    device.destroyShaderModule(vertShader);
    device.destroyShaderModule(fragShader);   

    vector<vk::Framebuffer> framebuffers;
    framebuffers.resize(swapviews.size());
    for(int i = 0; i < framebuffers.size(); i++) {
        vector<vk::ImageView> attach = {
            swapviews.at(i),
            depthImage.view
        };
        framebuffers[i] = device.createFramebuffer(
            vk::FramebufferCreateInfo(
                {},
                pass,
                attach,
                swapextent.width,
                swapextent.height,
                1
            )
        );
    }  

    vk::CommandPool commandPool 
     = device.createCommandPool(
        vk::CommandPoolCreateInfo(
        vk::CommandPoolCreateFlags(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer
        ),
        graphIndex
        )
     );

    vk::CommandBuffer commandBuffer
      = device.allocateCommandBuffers(
        vk::CommandBufferAllocateInfo(
            commandPool,
            vk::CommandBufferLevel::ePrimary,
            1
        )
    ).front();

    vk::Fence inFlightFence
    = device.createFence(
        vk::FenceCreateInfo(
            vk::FenceCreateFlagBits::eSignaled)
    );

    vk::Semaphore imageSem 
    = device.createSemaphore(vk::SemaphoreCreateInfo());
    vk::Semaphore renderSem 
    = device.createSemaphore(vk::SemaphoreCreateInfo());
    

     
    // CREATION TODO

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        device.waitForFences(1, &inFlightFence, 
                            true, UINT64_MAX);
        device.resetFences(1, &inFlightFence);

        auto result = device.acquireNextImageKHR(
            swapchain, UINT64_MAX, 
            imageSem, nullptr
        );

        unsigned int frameIndex = result.value;

        commandBuffer.reset();
        commandBuffer.begin(vk::CommandBufferBeginInfo());
        array<vk::ClearValue, 2> clearValues {};
        clearValues[0].color
         = vk::ClearColorValue(0.9f, 0.9f, 0.0f, 1.0f);
        clearValues[1].depthStencil
        = vk::ClearDepthStencilValue(1.0f, 0.0f);

        commandBuffer.beginRenderPass(
            vk::RenderPassBeginInfo(
                pass, framebuffers[frameIndex],
                { {0,0}, swapextent},
                clearValues
            ),
            vk::SubpassContents::eInline
        );

        commandBuffer.bindPipeline(
            vk::PipelineBindPoint::eGraphics,
            graphicsPipeline
        );

        vk::Viewport viewports[] =
        {
            {0,0,
            (float)swapextent.width,
            (float)swapextent.height,
            0.0f, 1.0f}
        };
        commandBuffer.setViewport(0, viewports);

        vk::Rect2D scissors[] =
        {
            {{0,0}, swapextent}
        };
        commandBuffer.setScissor(0, scissors);  

        UniformPush ub{};
        ub.modelMat = modelMat;
        
        uboVertHost.viewMat = glm::lookAt(
            glm::vec3(1,1,1),   // eye
            glm::vec3(0,0,0),   // look-at(center)
            glm::vec3(0,1,0));  // up

        ub.normMat = glm::mat4(
                        glm::transpose(
                            glm::inverse(
                                glm::mat3(
                                    uboVertHost.viewMat*ub.modelMat))));

        commandBuffer.pushConstants(
            pipelineLayout,
            vk::ShaderStageFlagBits::eVertex,
            0, sizeof(UniformPush),
            &ub
        );
            
        float fovY = glm::radians(90.0f);
        float aspectRatio = ((float)swapextent.width)/((float)swapextent.height);
        float near = 0.01f;
        float far = 1000.0f;
        uboVertHost.projMat = glm::perspective(
                                fovY,
                                aspectRatio, 
                                near, far);
        uboVertHost.projMat[1][1] *= -1.0f;

        uboFragHost.light.vpos = uboVertHost.viewMat*uboFragHost.light.pos;

        memcpy(uboVertData.mapped[0],
            &uboVertHost, sizeof(UBOVertex));
        memcpy(uboFragData.mapped[0],
            &uboFragHost, sizeof(UBOFragment));

        commandBuffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            pipelineLayout,
            0, descSets[0], {}
        );

        // RENDER
        vk::Buffer vertBuffers[] = {vkVertices.buffer};
        vk::DeviceSize offsets[] = {0};
        commandBuffer.bindVertexBuffers(
            0, vertBuffers, offsets);

        commandBuffer.bindIndexBuffer(
            vkIndices.buffer, 0,
            vk::IndexType::eUint32
        );

        commandBuffer.drawIndexed(
            indices.size(), 1, 0, 0, 0);
        
        commandBuffer.endRenderPass();
        commandBuffer.end();

        vk::Semaphore waitList[] = {imageSem};
        vk::Semaphore sigList[] = {renderSem};

        vk::PipelineStageFlags waitStages[]
        = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

        vk::SubmitInfo submitInfo(
            waitList, 
            waitStages,
            commandBuffer,
            sigList
        );

        graphQueue.submit(submitInfo, inFlightFence);

        vk::SwapchainKHR swapchains[] = {swapchain};
        uint32_t imageIndices[] = {frameIndex};
        vk::PresentInfoKHR presentInfo(
            sigList,
            swapchains,
            imageIndices
        );

        presentQueue.presentKHR(presentInfo);
    }

    // CLEANUP TODO
    cleanupVulkanImage(device, depthImage);

    for(int i = 0; i < allDescSetLayouts.size(); i++) {
        device.destroyDescriptorSetLayout(
            allDescSetLayouts.at(i));
    }
    allDescSetLayouts.clear();

    device.destroyDescriptorPool(descPool);
    cleanupVulkanUniformBufferData(device, uboFragData);
    cleanupVulkanUniformBufferData(device, uboVertData);
    device.destroySemaphore(imageSem);
    device.destroySemaphore(renderSem);
    device.destroyFence(inFlightFence);
    device.destroyCommandPool(commandPool);
    for(auto framebuffer: framebuffers) {
        device.destroyFramebuffer(framebuffer);
    }
    framebuffers.clear();

    device.destroyPipelineCache(pipelineCache);
    device.destroyPipelineLayout(pipelineLayout);
    device.destroyPipeline(graphicsPipeline);
    cleanupVulkanBuffer(device, vkIndices);
    cleanupVulkanBuffer(device, vkVertices);

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