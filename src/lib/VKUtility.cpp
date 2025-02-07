#include "VKUtility.hpp"

///////////////////////////////////////////////////////////////////////////////
// FILE I/O
///////////////////////////////////////////////////////////////////////////////

vector<char> readBinaryFile(const string& filename) {
    ifstream file(filename, ios::ate | ios::binary);

    if (!file.is_open()) {
        throw runtime_error("Failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

///////////////////////////////////////////////////////////////////////////////
// TIME
///////////////////////////////////////////////////////////////////////////////

chrono::steady_clock::time_point getTime() {
    return chrono::steady_clock::now();
}

float getElapsedSeconds(chrono::steady_clock::time_point start, 
                        chrono::steady_clock::time_point end) {
    return chrono::duration<float, std::chrono::seconds::period>(end - start).count();
}

///////////////////////////////////////////////////////////////////////////////
// MATRIX CONVERSION
///////////////////////////////////////////////////////////////////////////////

void aiMatToGLM4(aiMatrix4x4 &a, glm::mat4 &m) {
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            m[j][i] = a[i][j];
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// DEBUGGING PRINT
///////////////////////////////////////////////////////////////////////////////

void printTab(int cnt) {
    for(int i = 0; i < cnt; i++) {
        cout << "\t";
    }
}

void printNodeInfo(aiNode *node, glm::mat4 &nodeT, glm::mat4 &parentMat, glm::mat4 &currentMat, int level) {
    printTab(level);
    cout << "NAME: " << node->mName.C_Str() << endl;
    printTab(level);
    cout << "NUM MESHES: " << node->mNumMeshes << endl;
    printTab(level);
    cout << "NUM CHILDREN: " << node->mNumChildren << endl;
    printTab(level);
    cout << "Parent Model Matrix:" << glm::to_string(parentMat) << endl;
    printTab(level);
    cout << "Node Transforms:" << glm::to_string(nodeT) << endl;
    printTab(level);
    cout << "Current Model Matrix:" << glm::to_string(currentMat) << endl;
    cout << endl;
}

///////////////////////////////////////////////////////////////////////////////
// VULKAN SYNC OBJECTS
///////////////////////////////////////////////////////////////////////////////

vk::Semaphore createVulkanSemaphore(vk::Device &device) {
    return device.createSemaphore(vk::SemaphoreCreateInfo());
}

vk::Fence createVulkanFence(vk::Device &device) {
    return device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
}

void cleanupVulkanSemaphore(vk::Device &device, vk::Semaphore &s) {
    device.destroySemaphore(s);
}

void cleanupVulkanFence(vk::Device &device, vk::Fence &f) {
    device.destroyFence(f);
}  

///////////////////////////////////////////////////////////////////////////////
// Vulkan command pools and buffers
///////////////////////////////////////////////////////////////////////////////

vk::CommandPool createVulkanCommandPool(vk::Device &device, unsigned int queueIndex) {

    return device.createCommandPool(
        vk::CommandPoolCreateInfo(
            vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer), // Reset pool every frame
            queueIndex));   
}

vk::CommandBuffer createVulkanCommandBuffer(vk::Device &device, vk::CommandPool &commandPool) {
    return device.allocateCommandBuffers(
        vk::CommandBufferAllocateInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1)).front();    
}

void cleanupVulkanCommandPool(vk::Device &device, vk::CommandPool &commandPool) {
    device.destroyCommandPool(commandPool);
} 

vk::CommandBuffer createAndStartOneTimeVulkanCommandBuffer(vk::Device &device, vk::CommandPool &commandPool) {
    // Create a command buffer
    vk::CommandBuffer oneTimeBuffer = createVulkanCommandBuffer(device, commandPool);

    // Start recording    
    oneTimeBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    // Return buffer
    return oneTimeBuffer;
}

void stopAndCleanupOneTimeVulkanCommandBuffer(  vk::Device &device, vk::CommandPool &commandPool,
                                                vk::CommandBuffer &oneTimeBuffer,
                                                vk::Queue &graphicsQueue) {
    // End recording
    oneTimeBuffer.end();

    // Submit to queue
    vk::SubmitInfo submitInfo = vk::SubmitInfo().setCommandBuffers(oneTimeBuffer);                    
    graphicsQueue.submit(submitInfo);
    graphicsQueue.waitIdle();

    // Clean up command buffer
    device.freeCommandBuffers(commandPool, oneTimeBuffer);    
}

///////////////////////////////////////////////////////////////////////////////
// Vulkan shaders
///////////////////////////////////////////////////////////////////////////////

vk::ShaderModule createVulkanShaderModule(vk::Device &device, const vector<char>& code) {

    return device.createShaderModule(vk::ShaderModuleCreateInfo(
        vk::ShaderModuleCreateFlags(), code.size(), 
        reinterpret_cast<const uint32_t*>(code.data()) // Cast that pretends as if it were a uint32_t pointer
    ));
}
