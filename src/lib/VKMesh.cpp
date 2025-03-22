#include "VKMesh.hpp"

///////////////////////////////////////////////////////////////////////////////
// Vulkan mesh
///////////////////////////////////////////////////////////////////////////////

void recordDrawVulkanMesh(vk::CommandBuffer &commandBuffer, VulkanMesh &mesh) {
    
    vk::Buffer vertexBuffers[] = {mesh.vertices.buffer};
    vk::DeviceSize offsets[] = {0};
    commandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
    commandBuffer.bindIndexBuffer(mesh.indices.buffer, 0, vk::IndexType::eUint32);
    
    commandBuffer.drawIndexed(static_cast<unsigned int>(mesh.indexCnt), 1, 0, 0, 0);
}    


void cleanupVulkanMesh(VulkanInitData &vkInitData, VulkanMesh &mesh) {
    cleanupVulkanBuffer(vkInitData.device, mesh.vertices);
    cleanupVulkanBuffer(vkInitData.device, mesh.indices);
}
