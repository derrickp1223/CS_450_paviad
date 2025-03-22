#include "VKSetup.hpp"
#include "VKRender.hpp"
#include "VKImage.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "MeshData.hpp"

struct Vertex {
    glm::vec3 pos;
    glm::vec4 color;
};

struct SceneData {
    vector<VulkanMesh> allMeshes;
    const aiScene *scene = nullptr;
};

SceneData sceneData;

class Assign02RenderEngine : public VulkanRenderEngine {
    public:
        Assign02RenderEngine(VulkanInitData & vkInitData) :
        VulkanRenderEngine(vkInitData) {};

        virtual bool initialize(VulkanInitRenderParams *params) override {
            if(!VulkanRenderEngine::initialize(params)) { return false; }

            return true;
            };

        virtual ~Assign02RenderEngine() {};

        virtual void recordCommandBuffer( void *userData, vk::CommandBuffer &commandBuffer, 
                                          unsigned int frameIndex) override {
            SceneData *sceneData = static_cast<SceneData*>(userData);
            for(auto &mesh : sceneData->allMeshes) {
                recordDrawVulkanMesh(commandBuffer, mesh);
            }
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


int main(int argc, char **argv) {
    cout << "BEGIN FORGING!!!" << endl;
    
    // Set name
    string appName = "Assign02";
    string windowTitle = "Assign02: paviad";
    int windowWidth = 800;
    int windowHeight = 600;

    // Create GLFW window
    GLFWwindow* window = createGLFWWindow(windowTitle, windowWidth, windowHeight);

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
    VulkanRenderEngine *renderEngine = new Assign02RenderEngine(vkInitData);
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
   
    for (int i = 0; i < sceneData.scene->mNumMeshes; ++i) { // TODOOOO
        Mesh<Vertex> mesh;        
        extractMeshData(sceneData.scene->mMeshes[i], mesh);
        VulkanMesh vulkanMesh = createVulkanMesh(mesh);
        sceneData.allMeshes.push_back(vulkanMesh);
    }

    VulkanMesh vulkanMesh = createVulkanMesh(mesh);
    
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
        renderEngine->drawFrame(&allMeshes);  

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
    cleanupVulkanMesh(vkInitData, mesh);
    delete renderEngine;
    cleanupVulkanBootstrap(vkInitData);
    cleanupGLFWWindow(window);
    
    cout << "FORGING DONE!!!" << endl;
    return 0;
}