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


struct Vertex {
    glm::vec3 pos;
    glm::vec4 color;
};

struct UPushVertex {
    glm::vec3 pos;
    glm::vec4 color;
};

struct SceneData {
    vector<VulkanMesh> allMeshes;
    const aiScene *scene = nullptr;
    float rotAngle = 0.0f;
};

SceneData sceneData;

glm::mat4 makeRotateZ(float rotAngle, glm::vec3 offset) {
    float radAngle = glm::radians(rotAngle);

    glm::mat4 translate1 = glm::translate(-offset);
    glm::mat4 rotate = glm::rotate(radAngle, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat translate2 = glm::translate(offset);
    
    return translate2 * rotate * translate1;
}

class Assign03RenderEngine : public VulkanRenderEngine {
    public:
        Assign03RenderEngine(VulkanInitData & vkInitData) :
        VulkanRenderEngine(vkInitData) {};

        virtual bool initialize(VulkanInitRenderParams *params) override {
            if(!VulkanRenderEngine::initialize(params)) { return false; }

            return true;
            };

        virtual ~Assign03RenderEngine() {};

        virtual vector<vk::PushConstantRange> getPushConstantRanges() override {
            vector<vk::PushConstantRange> ranges;

            vk::PushConstantRange range;
            range.setStageFlags(vk::ShaderStageFlagBits::eVertex);
            range.setOffset(0);
            range.setSize(sizeof(UPushVertex));
            ranges.push_back(range);

            return ranges;
        }

        void renderScene(vk::CommandBuffer &commandBuffer, SceneData *sceneData, 
                         aiNode *node, glm::mat4 parentMat, int level) {
            // Get transformation for current node and convert                
            aiMatrix4x4 aiNodeT = node->mTransformation;
            glm::mat4 nodeT = aiMatToGLM4(aiNodeT); //TODO function defined in include/VKUtility.hpp/cpp

            // Compute current model matrix
            glm::mat4 modelMat = parentMat * nodeT;

            // Location of current node
            glm::vec3 pos = glm::vec(modelMat[3]);

            // Temporary model matrix
            glm::mat4 R = makeRotateZ(sceneData->rotAngle, pos);
            glm::mat4 tmpModel = R * modelMat;

            // Create instance of Upush and store tmpModel as model matrix
            UPushVertex tmpModel; //Does this work? TODO

            // Push up UPushVertex data
            commandBuffer.pushConstants(
                this->pipelineData.pipelineLayout,
                vk::ShaderStageFlagBits::eVertex,
                0,
                sizeof(UPushVertex),
                &tmpModel // HERE TODO
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
        }
    }
}


int main(int argc, char **argv) {
    cout << "BEGIN FORGING!!!" << endl;
    
    // Set name
    string appName = "Assign03";
    string windowTitle = "Assign03: paviad";
    int windowWidth = 800;
    int windowHeight = 600;

    // Create GLFW window
    GLFWwindow* window = createGLFWWindow(windowTitle, windowWidth, windowHeight);

    // Key callback
    glfwSetKeyCallback(window, keyCallback);

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
    VulkanRenderEngine *renderEngine = new Assign03RenderEngine(vkInitData);
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
    static_cast<Assign03RenderEngine*>(renderEngine)->
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