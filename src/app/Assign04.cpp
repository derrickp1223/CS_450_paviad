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
};

struct UPushVertex {
    glm::vec3 pos;
    glm::vec4 color;
    alignas(16) glm::mat4 modelMat;
};

struct UBOVertex {
    alignas(16) glm::mat4 viewMat;
    alignas(16) glm::mat4 projMat;
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

class Assign04RenderEngine : public VulkanRenderEngine {
    protected:
    UBOVertex hostUBOVert;
    UBOData deviceUBOVert;
    vk::DescriptorPool descriptorPool;
    vector<vk::DescriptorSet> descriptorSets;

    public:
        Assign04RenderEngine(VulkanInitData & vkInitData) :
        VulkanRenderEngine(vkInitData) {};

        virtual bool initialize(VulkanInitRenderParams *params) override {
            if(!VulkanRenderEngine::initialize(params)) { return false; }
            
            deviceUBOVert = createVulkanUniformBufferData(
                vkInitData.device, 
                vkInitData.physicalDevice,
                sizeof(UBOVertex),
                MAX_FRAMES_IN_FLIGHT
            );
            
            // Create descriptor pool
            vector<vk::DescriptorPoolSize> poolSizes;
            poolSizes.push_back(vk::DescriptorPoolSize(
                vk::DescriptorType::eUniformBuffer,
                MAX_FRAMES_IN_FLIGHT
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
                
                vkInitData.device.updateDescriptorSets(writes, {});
            }
            return true;
            };

        virtual ~Assign04RenderEngine() {
            vkInitData.device.destroyDescriptorPool(descriptorPool);
            cleanupVulkanUniformBufferData(vkInitData.device, deviceUBOVert);
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
            
            vk::DescriptorSetLayoutBinding uboBinding;
            uboBinding.binding = 0;
            uboBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
            uboBinding.descriptorCount = 1;
            uboBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
            uboBinding.pImmutableSamplers = nullptr;
            
            allBindings.push_back(uboBinding);
            
            vk::DescriptorSetLayout layout = vkInitData.device.createDescriptorSetLayout(
                vk::DescriptorSetLayoutCreateInfo({}, allBindings)
            );
            
            return vector<vk::DescriptorSetLayout>{layout};
        }
        
        virtual void updateUniformBuffers(SceneData *sceneData, vk::CommandBuffer &commandBuffer) {
            // Copy view and projection matrices from scene data
            hostUBOVert.viewMat = sceneData->viewMat;
            hostUBOVert.projMat = sceneData->projMat;
            
            // Invert Y for projection matrix
            hostUBOVert.projMat[1][1] *= -1;
            
            // Copy UBO host data to device
            memcpy(deviceUBOVert.mapped[this->currentImage], &hostUBOVert, sizeof(hostUBOVert));
            
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
            aiMatToGLM4(aiNodeT,nodeT); //TODO function defined in include/VKUtility.hpp/cpp

            // Compute current model matrix
            glm::mat4 modelMat = parentMat * nodeT;

            // Location of current node
            glm::vec3 pos = glm::vec(modelMat[3]);

            // Temporary model matrix
            glm::mat4 R = makeRotateZ(sceneData->rotAngle, pos);
            glm::mat4 tmpModel = R * modelMat;

            // Create instance of Upush and store tmpModel as model matrix
            UPushVertex pushVertex; 
            pushVertex.modelMat = tmpModel;

            // Push up UPushVertex data
            commandBuffer.pushConstants(
                this->pipelineData.pipelineLayout,
                vk::ShaderStageFlagBits::eVertex,
                0,
                sizeof(UPushVertex),
                &pushVertex // HERE TODO
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

        virtual void recordCommandBuffer( void *userData, vk::CommandBuffer &commandBuffer, 
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
            
            // Call render scene
            renderScene(commandBuffer, sceneData, sceneData->scene->mRootNode, glm::mat4(1.0f), 0);

            /* // Draw meshes
            for(auto &mesh : sceneData->allMeshes) {
                recordDrawVulkanMesh(commandBuffer, mesh);
            } */
            
            // Stop render pass
            commandBuffer.endRenderPass();
            
            // End command buffer
            commandBuffer.end();
        };

        void extractMeshData(aiMesh *mesh, Mesh<Vertex> &m) {
            m.vertices.clear();
            m.indices.clear();

            for(int i = 0; i < mesh->mNumVertices; ++i){
                Vertex vertex;

                aiVector3D aiPos = mesh->mVertices[i];
                vertex.pos = glm::vec3(aiPos.x, aiPos.y, aiPos.z);

                vertex.color = glm::vec4( 0.5f, 0.8f, 0.3f, 1.0f);

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
        }
    }
}


int main(int argc, char **argv) {
    cout << "BEGIN FORGING!!!" << endl;
    
    // Set name
    string appName = "Assign04";
    string windowTitle = "Assign04: paviad";
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

    // Assign 02
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
    VulkanRenderEngine *renderEngine = new Assign04RenderEngine(vkInitData);
    renderEngine->initialize(&params);

    /*
    Before your drawing loop:
        ▪ Make sure your VulkanRenderEngine is an instance of Assign02RenderEngine:
        • VulkanRenderEngine *renderEngine
        = new Assign02RenderEngine(vkInitData);
        ▪ For each mesh in the scene (mMeshes with mNumMeshes):
        • Create a Mesh<Vertex> object inside the loop
        • Call extractMeshData to get a Mesh from each
        sceneData.scene->mMeshes[i]
        • Call createVulkanMesh to get a VulkanMesh from that Mesh
        • Add the VulkanMesh to your vector of VulkanMesh's in your sceneData
        o In your drawing loop:
        ▪ Pass in the address of sceneData to drawFrame():
        • renderEngine->drawFrame(&sceneData);
        o After your drawing loop:
        ▪ Comment out previous cleanupVulkanMesh() call
        ▪ Loop through all of your VulkanMesh objects and call cleanupVulkanMesh()
        ▪ Clear out your list of VulkanMesh objects in your sceneData
    */
   for (int i = 0; i < sceneData.scene->mNumMeshes; ++i) {
    Mesh<Vertex> mesh;
    static_cast<Assign04RenderEngine*>(renderEngine)->
        extractMeshData(sceneData.scene->mMeshes[i], mesh);
    VulkanMesh vulkanMesh = 
        createVulkanMesh(vkInitData, renderEngine->getCommandPool(), mesh);
        sceneData.allMeshes.push_back(vulkanMesh);
}


    
    // Create very simple quad on host
    /*Mesh<SimpleVertex> hostMesh = {
        {
            {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
            {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}}
        },
        { 0, 2, 1, 2, 0, 3 }
    };
    
    // Create Vulkan mesh
    VulkanMesh mesh = createVulkanMesh(vkInitData, renderEngine->getCommandPool(), hostMesh); 
    vector<VulkanMesh> allMeshes {
        { mesh }
    };
    */

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
    
    // Cleanup  
    // cleanupVulkanMesh(vkInitData, mesh);
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