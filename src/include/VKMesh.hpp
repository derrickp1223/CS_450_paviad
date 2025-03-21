#pragma once
#include <vector>
#include <cstddef>
#include "MeshData.hpp"
#include "VKBuffer.hpp"
#include "VKSetup.hpp"
#include "VKUtility.hpp"

///////////////////////////////////////////////////////////////////////////////
// Attribute layout/descriptions
// - Needed by shaders and pipeline
///////////////////////////////////////////////////////////////////////////////
struct AttributeDescData {
    vk::VertexInputBindingDescription bindDesc;
    vector<vk::VertexInputAttributeDescription> attribDesc;
};

///////////////////////////////////////////////////////////////////////////////
// Vulkan mesh data
///////////////////////////////////////////////////////////////////////////////

struct VulkanMesh {
    VulkanBuffer vertices;
    VulkanBuffer indices;
    int indexCnt = 0;
};

template<typename T>
VulkanMesh createVulkanMesh(VulkanInitData &vkInitData, 
                            vk::CommandPool &commandPool, 
                            Mesh<T> &hostMesh) {
    // Set up Vulkan mesh                            
    VulkanMesh mesh;

    // Create vertex buffer (note eTransferDst flag and eDeviceLocal)
    vk::DeviceSize vertBufferSize = sizeof(hostMesh.vertices[0]) * hostMesh.vertices.size();    
    mesh.vertices = createVulkanBuffer(
        vkInitData.physicalDevice, vkInitData.device, vertBufferSize,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, 
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    // Copy to buffer via staging buffer
    copyDataToVulkanBufferViaStaging(vkInitData.physicalDevice, vkInitData.device,
                                     commandPool, vkInitData.graphicsQueue.queue, 
                                     mesh.vertices, vertBufferSize, hostMesh.vertices.data());

    // Create index buffer
    vk::DeviceSize indexBufferSize = sizeof(hostMesh.indices[0]) * hostMesh.indices.size();
    mesh.indices = createVulkanBuffer(
        vkInitData.physicalDevice, vkInitData.device, indexBufferSize,
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, 
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    // Copy to buffer via staging buffer    
    copyDataToVulkanBufferViaStaging(vkInitData.physicalDevice, vkInitData.device,
                                     commandPool, vkInitData.graphicsQueue.queue, 
                                     mesh.indices, indexBufferSize, hostMesh.indices.data());

    // Set index count
    mesh.indexCnt = hostMesh.indices.size();

    // Return mesh
    return mesh;
}

void recordDrawVulkanMesh(vk::CommandBuffer &commandBuffer, VulkanMesh &mesh);
void cleanupVulkanMesh(VulkanInitData &vkInitData, VulkanMesh &mesh);

