#include "VKSetup.hpp"
#include "VKRender.hpp"
#include "VKImage.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "MeshData.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/transform.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "VKUtility.hpp"
#include "VKUniform.hpp"


struct Vertex {
    glm::vec3 pos;
    glm::vec4 color;
    glm::vec3 normal;
};

struct UPushVertex {
    glm::vec3 pos;
    glm::vec4 color;
    alignas(16) glm::mat4 modelMat;
    alignas(16) glm::mat4 normMat;
};

struct UBOVertex {
    alignas(16) glm::mat4 viewMat;
    alignas(16) glm::mat4 projMat;
};

struct PointLight {
    alignas(16) glm::vec4 pos;
    alignas(16) glm::vec4 vpos;
    alignas(16) glm::vec4 color;
};

struct UBOFragment {
    PointLight light;
    alignas(4) float metallic;
    alignas(4) float roughness;
};

struct SceneData {
    vector<VulkanMesh> allMeshes;
    const aiScene *scene = nullptr;
    float rotAngle = 0.0f;

    glm::vec3 eye = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 lookAt = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec2 mousePos;
    glm::mat4 viewMat = glm::mat4(1.0f);
    glm::mat4 projMat = glm::mat4(1.0f);
    
    // New fields for Assign05
    PointLight light = {
        glm::vec4(0.5f, 0.5f, 0.5f, 1.0f), // World position
        glm::vec4(0.0f), // View position (will be updated)
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) // White color
    };
    
    float metallic = 0.0f;
    float roughness = 0.1f;
};

SceneData sceneData;

glm::mat4 makeRotateZ(float rotAngle, glm::vec3 offset) {
    float radAngle = glm::radians(rotAngle);

    glm::mat4 translate1 = glm::translate(-offset);
    glm::mat4 rotate = glm::rotate(radAngle, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat translate2 = glm::translate(offset);
    
    return translate2 * rotate * translate1;
}

glm::mat4 makeLocalRotate(glm::vec3 offset, glm::vec3 axis, float angle) {
    float radAngle = glm::radians(angle);
    
    glm::mat4 translate1 = glm::translate(-offset);
    glm::mat4 rotate = glm::rotate(radAngle, axis);
    glm::mat4 translate2 = glm::translate(offset);
    
    return translate2 * rotate * translate1;
}

static void mouse_position_callback(GLFWwindow* window, double xpos, double ypos) {
    // Calculate relative mouse motion
    glm::vec2 relMouse = glm::vec2(xpos, ypos) - sceneData.mousePos;
    
    // Get framebuffer size
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    
    if (width > 0 && height > 0) {
        // Scale mouse motion
        float relX = relMouse.x / static_cast<float>(width);
        float relY = relMouse.y / static_cast<float>(height);
        
        // Inverse mouse movement
        relX = -relX;
        relY = -relY;

        // Rotate camera based on mouse motion
        
        // X motion - rotate around global Y axis
        glm::mat4 rotY = makeLocalRotate(
            sceneData.eye,
            glm::vec3(0.0f, 1.0f, 0.0f),
            30.0f * relX
        );
        
        // Convert lookAt to vec4 for matrix multiplication
        glm::vec4 lookAtV = glm::vec4(sceneData.lookAt, 1.0f);
        lookAtV = rotY * lookAtV;
        sceneData.lookAt = glm::vec3(lookAtV);
        
        // Y motion - rotate around local X axis
        glm::vec3 cameraDir = glm::normalize(sceneData.lookAt - sceneData.eye);
        glm::vec3 localX = glm::normalize(glm::cross(cameraDir, glm::vec3(0.0f, 1.0f, 0.0f)));
        
        glm::mat4 rotX = makeLocalRotate(
            sceneData.eye,
            localX,
            30.0f * relY
        );
        
        lookAtV = glm::vec4(sceneData.lookAt, 1.0f);
        lookAtV = rotX * lookAtV;
        sceneData.lookAt = glm::vec3(lookAtV);
    }
    
    // Update mouse position
    sceneData.mousePos = glm::vec2(xpos, ypos);
}

class Assign05RenderEngine : public VulkanRenderEngine {
    protected:
    UBOVertex hostUBOVert;
    UBOData deviceUBOVert;
    UBOFragment hostUBOFrag;
    UBOData deviceUBOFrag;
    vk::DescriptorPool descriptorPool;
    vector<vk::DescriptorSet> descriptorSets;

    public:
        Assign05RenderEngine(VulkanInitData & vkInitData) :
        VulkanRenderEngine(vkInitData) {};

        virtual bool initialize(VulkanInitRenderParams *params) override {
            if(!VulkanRenderEngine::initialize(params)) { return false; }
            
            // Create UBO for vertex shader
            deviceUBOVert = createVulkanUniformBufferData(
                vkInitData.device, 
                vkInitData.physicalDevice,
                sizeof(UBOVertex),
                MAX_FRAMES_IN_FLIGHT
            );
            
            // Create UBO for fragment shader
            deviceUBOFrag = createVulkanUniformBufferData(
                vkInitData.device, 
                vkInitData.physicalDevice,
                sizeof(UBOFragment),
                MAX_FRAMES_IN_FLIGHT
            );
            
            // Create descriptor pool
            vector<vk::DescriptorPoolSize> poolSizes;
            poolSizes.push_back(vk::DescriptorPoolSize(
                vk::DescriptorType::eUniformBuffer,
                2 * MAX_FRAMES_IN_FLIGHT  // Changed to include both vertex and fragment UBOs
            ));
            
            vk::DescriptorPoolCreateInfo poolCreateInfo;
            poolCreateInfo.setPoolSizes(poolSizes);
            poolCreateInfo.setMaxSets(MAX_FRAMES_IN_FLIGHT);
            
            descriptorPool = vkInitData.device.createDescriptorPool(poolCreateInfo);
            
            // Create descriptor sets
            vector<vk::DescriptorSetLayout> localLayoutList;
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                localLayoutList.push_back(pipelineData.descriptorSetLayouts[0]);
            }
            
            vk::DescriptorSetAllocateInfo allocInfo;
            allocInfo.setDescriptorPool(descriptorPool);
            allocInfo.setDescriptorSetCount(MAX_FRAMES_IN_FLIGHT);
            allocInfo.setSetLayouts(localLayoutList);
            
            descriptorSets = vkInitData.device.allocateDescriptorSets(allocInfo);
            
            // Update descriptor sets
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vector<vk::WriteDescriptorSet> writes;
                
                // Vertex shader UBO descriptor
                vk::DescriptorBufferInfo bufferVertInfo;
                bufferVertInfo.setBuffer(deviceUBOVert.bufferData[i].buffer);
                bufferVertInfo.setOffset(0);
                bufferVertInfo.setRange(sizeof(UBOVertex));
                
                vk::WriteDescriptorSet descVertWrites;
                descVertWrites.setDstSet(descriptorSets[i]);
                descVertWrites.setDstBinding(0);
                descVertWrites.setDstArrayElement(0);
                descVertWrites.setDescriptorType(vk::DescriptorType::eUniformBuffer);
                descVertWrites.setDescriptorCount(1);
                descVertWrites.setBufferInfo(bufferVertInfo);
                
                writes.push_back(descVertWrites);
                
                // Fragment shader UBO descriptor
                vk::DescriptorBufferInfo bufferFragInfo;
                bufferFragInfo.setBuffer(deviceUBOFrag.bufferData[i].buffer);
                bufferFragInfo.setOffset(0);
                bufferFragInfo.setRange(sizeof(UBOFragment));
                
                vk::WriteDescriptorSet descFragWrites;
                descFragWrites.setDstSet(descriptorSets[i]);
                descFragWrites.setDstBinding(1);
                descFragWrites.setDstArrayElement(0);
                descFragWrites.setDescriptorType(vk::DescriptorType::eUniformBuffer);
                descFragWrites.setDescriptorCount(1);
                descFragWrites.setBufferInfo(bufferFragInfo);
                
                writes.push_back(descFragWrites);
                
                vkInitData.device.updateDescriptorSets(writes, {});
            }
            return true;
        };

        virtual ~Assign05RenderEngine() {
            vkInitData.device.destroyDescriptorPool(descriptorPool);
            cleanupVulkanUniformBufferData(vkInitData.device, deviceUBOVert);
            cleanupVulkanUniformBufferData(vkInitData.device, deviceUBOFrag);
        };

        virtual vector<vk::PushConstantRange> getPushConstantRanges() override {
            vector<vk::PushConstantRange> ranges;

            vk::PushConstantRange range;
            range.setStageFlags(vk::ShaderStageFlagBits::eVertex);
            range.setOffset(0);
            range.setSize(sizeof(UPushVertex));
            ranges.push_back(range);

            return ranges;
        }

        virtual vector<vk::DescriptorSetLayout> getDescriptorSetLayouts() override {
            vector<vk::DescriptorSetLayoutBinding> allBindings;
            
            // Vertex shader UBO binding
            vk::DescriptorSetLayoutBinding uboVertBinding;
            uboVertBinding.binding = 0;
            uboVertBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
            uboVertBinding.descriptorCount = 1;
            uboVertBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
            uboVertBinding.pImmutableSamplers = nullptr;
            
            // Fragment shader UBO binding
            vk::DescriptorSetLayoutBinding uboFragBinding;
            uboFragBinding.binding = 1;
            uboFragBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
            uboFragBinding.descriptorCount = 1;
            uboFragBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
            uboFragBinding.pImmutableSamplers = nullptr;
            
            allBindings.push_back(uboVertBinding);
            allBindings.push_back(uboFragBinding);
            
            vk::DescriptorSetLayout layout = vkInitData.device.createDescriptorSetLayout(
                vk::DescriptorSetLayoutCreateInfo({}, allBindings)
            );
            
            return vector<vk::DescriptorSetLayout>{layout};
        }
        
        virtual AttributeDescData getAttributeDescData() override {
            AttributeDescData attribDescData;
            
            // Set binding description
            attribDescData.bindDesc = vk::VertexInputBindingDescription(
                0, sizeof(Vertex), vk::VertexInputRate::eVertex
            );
            
            // Clear and add attribute descriptions
            attribDescData.attribDesc.clear();
            
            // Position attribute
            attribDescData.attribDesc.push_back(
                vk::VertexInputAttributeDescription(
                    0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)
                )
            );
            
            // Color attribute
            attribDescData.attribDesc.push_back(
                vk::VertexInputAttributeDescription(
                    1, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, color)
                )
            );
            
            // Normal attribute
            attribDescData.attribDesc.push_back(
                vk::VertexInputAttributeDescription(
                    2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)
                )
            );
            
            return attribDescData;
        }
        
        virtual void updateUniformBuffers(SceneData *sceneData, vk::CommandBuffer &commandBuffer) {
            // Update vertex UBO
            hostUBOVert.viewMat = sceneData->viewMat;
            hostUBOVert.projMat = sceneData->projMat;
            
            // Invert Y for projection matrix
            hostUBOVert.projMat[1][1] *= -1;
            
            // Copy UBO vertex host data to device
            memcpy(deviceUBOVert.mapped[this->currentImage], &hostUBOVert, sizeof(hostUBOVert));
            
            // Update fragment UBO
            hostUBOFrag.light = sceneData->light;
            hostUBOFrag.metallic = sceneData->metallic;
            hostUBOFrag.roughness = sceneData->roughness;
            
            // Copy UBO fragment host data to device
            memcpy(deviceUBOFrag.mapped[this->currentImage], &hostUBOFrag, sizeof(hostUBOFrag));
            
            // Bind descriptor sets
            commandBuffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                pipelineData.pipelineLayout,
                0,
                descriptorSets[currentImage],
                {}
            );
        }

        void renderScene(vk::CommandBuffer &commandBuffer, SceneData *sceneData, 
                         aiNode *node, glm::mat4 parentMat, int level) {
            // Get transformation for current node and convert                
            aiMatrix4x4 aiNodeT = node->mTransformation;
            glm::mat4 nodeT;
            aiMatToGLM4(aiNodeT, nodeT);

            // Compute current model matrix
            glm::mat4 modelMat = parentMat * nodeT;

            // Location of current node
            glm::vec3 pos = glm::vec3(modelMat[3]);

            // Temporary model matrix
            glm::mat4 R = makeRotateZ(sceneData->rotAngle, pos);
            glm::mat4 tmpModel = R * modelMat;
            
            // Calculate normal matrix
            glm::mat4 normalMat = glm::transpose(glm::inverse(glm::mat4(sceneData->viewMat * tmpModel)));

            // Create instance of UPushVertex and store matrices
            UPushVertex pushVertex; 
            pushVertex.modelMat = tmpModel;
            pushVertex.normMat = normalMat;

            // Push UPushVertex data
            commandBuffer.pushConstants(
                this->pipelineData.pipelineLayout,
                vk::ShaderStageFlagBits::eVertex,
                0,
                sizeof(UPushVertex),
                &pushVertex
            );

            // Draw Meshes
            for (int i = 0; i < node->mNumMeshes; i++) {
                int index = node->mMeshes[i];
                recordDrawVulkanMesh(commandBuffer, sceneData->allMeshes.at(index));
            }

            // Children
            for (int i = 0; i < node->mNumChildren; i++) {
                renderScene(commandBuffer, sceneData, node->mChildren[i], modelMat, level + 1);
            }
        }

        virtual void recordCommandBuffer(void *userData, vk::CommandBuffer &commandBuffer, 
            unsigned int frameIndex) override {
            SceneData *sceneData = static_cast<SceneData*>(userData);

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

            // Update uniform buffers before calling renderScene
            updateUniformBuffers(sceneData, commandBuffer);

            // Call render scene
            renderScene(commandBuffer, sceneData, sceneData->scene->mRootNode, glm::mat4(1.0f), 0);

            // Stop render pass
            commandBuffer.endRenderPass();

            // End command buffer
            commandBuffer.end();
        }

        void extractMeshData(aiMesh *mesh, Mesh<Vertex> &m) {
            m.vertices.clear();
            m.indices.clear();

            for(int i = 0; i < mesh->mNumVertices; ++i){
                Vertex vertex;

                // Position
                aiVector3D aiPos = mesh->mVertices[i];
                vertex.pos = glm::vec3(aiPos.x, aiPos.y, aiPos.z);

                // Set color to yellow
                vertex.color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
                
                // Set normal
                if (mesh->HasNormals()) {
                    aiVector3D aiNormal = mesh->mNormals[i];
                    vertex.normal = glm::vec3(aiNormal.x, aiNormal.y, aiNormal.z);
                } else {
                    vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);  // default up vector
                }

                m.vertices.push_back(vertex);
            }

            for(int i = 0; i < mesh->mNumFaces; ++i){
                aiFace face = mesh->mFaces[i];

                for(int j = 0; j < face.mNumIndices; ++j) {
                    m.indices.push_back(face.mIndices[j]);
                }
            }
        }
};

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                break;
            case GLFW_KEY_J:
                sceneData.rotAngle += 1.0f;
                break;
            case GLFW_KEY_K:
                sceneData.rotAngle -= 1.0f;
                break;
            case GLFW_KEY_W: {
                // Move forward
                glm::vec3 cameraDir = sceneData.lookAt - sceneData.eye;
                cameraDir = glm::normalize(cameraDir);
                float speed = 0.1f;
                
                sceneData.eye += cameraDir * speed;
                sceneData.lookAt += cameraDir * speed;
                break;
            }
            case GLFW_KEY_S: {
                // Move backward
                glm::vec3 cameraDir = sceneData.lookAt - sceneData.eye;
                cameraDir = glm::normalize(cameraDir);
                float speed = 0.1f;
                
                sceneData.eye -= cameraDir * speed;
                sceneData.lookAt -= cameraDir * speed;
                break;
            }
            case GLFW_KEY_D: {
                // Move right
                glm::vec3 cameraDir = sceneData.lookAt - sceneData.eye;
                glm::vec3 localX = glm::normalize(glm::cross(cameraDir, glm::vec3(0.0f, 1.0f, 0.0f)));
                float speed = 0.1f;
                
                sceneData.eye += localX * speed;
                sceneData.lookAt += localX * speed;
                break;
            }
            case GLFW_KEY_A: {
                // Move left
                glm::vec3 cameraDir = sceneData.lookAt - sceneData.eye;
                glm::vec3 localX = glm::normalize(glm::cross(cameraDir, glm::vec3(0.0f, 1.0f, 0.0f)));
                float speed = 0.1f;
                
                sceneData.eye -= localX * speed;
                sceneData.lookAt -= localX * speed;
                break;
            }
            // New key controls for Assign05
            case GLFW_KEY_1:
                sceneData.light.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); // White
                break;
            case GLFW_KEY_2:
                sceneData.light.color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
                break;
            case GLFW_KEY_3:
                sceneData.light.color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
                break;
            case GLFW_KEY_4:
                sceneData.light.color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f); // Blue
                break;
            case GLFW_KEY_V:
                sceneData.metallic -= 0.1f;
                if (sceneData.metallic < 0.0f) sceneData.metallic = 0.0f;
                break;
            case GLFW_KEY_B:
                sceneData.metallic += 0.1f;
                if (sceneData.metallic > 1.0f) sceneData.metallic = 1.0f;
                break;
            case GLFW_KEY_N:
                sceneData.roughness -= 0.1f;
                if (sceneData.roughness < 0.1f) sceneData.roughness = 0.1f;
                break;
            case GLFW_KEY_M:
                sceneData.roughness += 0.1f;
                if (sceneData.roughness > 0.7f) sceneData.roughness = 0.7f;
                break;
        }
    }
}


int main(int argc, char **argv) {
    cout << "BEGIN FORGING!!!" << endl;
    
    // Set name
    string appName = "Assign05";
    string windowTitle = "Assign05: paviad";
    int windowWidth = 800;
    int windowHeight = 600;

    // Create GLFW window
    GLFWwindow* window = createGLFWWindow(windowTitle, windowWidth, windowHeight);

    // Key callback
    glfwSetKeyCallback(window, keyCallback);

    // Get initial mouse position
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    sceneData.mousePos = glm::vec2(mx, my);
    
    // Set mouse callback
    glfwSetCursorPosCallback(window, mouse_position_callback);
    
    // Hide cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Setup up Vulkan via vk-bootstrap
    VulkanInitData vkInitData;
    initVulkanBootstrap(appName, window, vkInitData);

    // Load model
    string modelPath = "sampleModels/sphere.obj";

    if (argc >= 2) 
        modelPath = string(argv[1]);

    Assimp::Importer importer;
    sceneData.scene = importer.ReadFile(modelPath,
        aiProcess_Triangulate |
        aiProcess_FlipUVs     |
        aiProcess_GenNormals  |
        aiProcess_JoinIdenticalVertices);
        
    if (sceneData.scene == NULL) {
        cout << "Scene is null " << endl;
        return -1;
    }
    if (sceneData.scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        cout << "Error loading AI SCENE FLAGS" << endl;
        return -1;
    }
    if (sceneData.scene->mRootNode == NULL) {
        cout << "Scene root node is null " << endl;
    }

    // Setup basic forward rendering process
    string vertSPVFilename = "build/compiledshaders/" + appName + "/shader.vert.spv";                                                    
    string fragSPVFilename = "build/compiledshaders/" + appName + "/shader.frag.spv";
    
    // Create render engine
    VulkanInitRenderParams params = {
        vertSPVFilename, fragSPVFilename
    };    
    VulkanRenderEngine *renderEngine = new Assign05RenderEngine(vkInitData);
    renderEngine->initialize(&params);

    // Load all meshes from the scene
    for (int i = 0; i < sceneData.scene->mNumMeshes; ++i) {
        Mesh<Vertex> mesh;
        static_cast<Assign05RenderEngine*>(renderEngine)->
            extractMeshData(sceneData.scene->mMeshes[i], mesh);
        VulkanMesh vulkanMesh = 
            createVulkanMesh(vkInitData, renderEngine->getCommandPool(), mesh);
        sceneData.allMeshes.push_back(vulkanMesh);
    }

    float timeElapsed = 1.0f;
    int framesRendered = 0;
    auto startCountTime = getTime();
    float fpsCalcWindow = 5.0f;
                                       
    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        // Get start time        
        auto startTime = getTime();

        // Poll events for window
        glfwPollEvents();  

        // Update view matrix
        sceneData.viewMat = glm::lookAt(
            sceneData.eye,
            sceneData.lookAt,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        // Calculate aspect ratio and update projection matrix
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (width > 0 && height > 0) ? 
            static_cast<float>(width) / static_cast<float>(height) : 1.0f;
        
        sceneData.projMat = glm::perspective(
            glm::radians(90.0f),
            aspect,
            0.01f,
            50.0f
        );
        
        // Update light view position based on current view matrix
        sceneData.light.vpos = sceneData.viewMat * sceneData.light.pos;
        
        // Draw frame
        renderEngine->drawFrame(&sceneData);

        // Increment frame count
        framesRendered++;

        // Get end and elapsed
        auto endTime = getTime();
        float frameTime = getElapsedSeconds(startTime, endTime);
        
        float timeSoFar = getElapsedSeconds(startCountTime, getTime());

        if(timeSoFar >= fpsCalcWindow) {
            float fps = framesRendered / timeSoFar;
            cout << "FPS: " << fps << endl;

            startCountTime = getTime();
            framesRendered = 0;
        }        
    }
        
    // Make sure all queues on GPU are done
    vkInitData.device.waitIdle();
    
    // Cleanup meshes
    for (auto &mesh : sceneData.allMeshes) {
        cleanupVulkanMesh(vkInitData, mesh);
    }
    sceneData.allMeshes.clear();

    delete renderEngine;
    cleanupVulkanBootstrap(vkInitData);
    cleanupGLFWWindow(window);
    
    cout << "FORGING DONE!!!" << endl;
    return 0;
}