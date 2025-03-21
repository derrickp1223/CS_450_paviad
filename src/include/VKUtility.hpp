#pragma once
#include <iostream>
#include <fstream>
#include <chrono>
#include <atomic>
#define VULKAN_HPP_NO_NODISCARD_WARNINGS
#include <vulkan/vulkan.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define GLM_FORCE_CTOR_INIT
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtc/type_ptr.hpp"

using namespace std;

vector<char> readBinaryFile(const string& filename);

chrono::steady_clock::time_point getTime();
float getElapsedSeconds(chrono::steady_clock::time_point start, chrono::steady_clock::time_point end);

void aiMatToGLM4(aiMatrix4x4 &a, glm::mat4 &m);

void printTab(int cnt);
void printNodeInfo(aiNode *node, glm::mat4 &nodeT, glm::mat4 &parentMat, glm::mat4 &currentMat, int level);

vk::Semaphore createVulkanSemaphore(vk::Device &device);
vk::Fence createVulkanFence(vk::Device &device);
void cleanupVulkanSemaphore(vk::Device &device, vk::Semaphore &s);
void cleanupVulkanFence(vk::Device &device, vk::Fence &f);

vk::CommandPool createVulkanCommandPool(vk::Device &device, unsigned int queueIndex);
vk::CommandBuffer createVulkanCommandBuffer(vk::Device &device, vk::CommandPool &commandPool);
void cleanupVulkanCommandPool(vk::Device &device, vk::CommandPool &pool);

vk::CommandBuffer createAndStartOneTimeVulkanCommandBuffer(vk::Device &device, vk::CommandPool &commandPool);
void stopAndCleanupOneTimeVulkanCommandBuffer(  vk::Device &device, 
                                                vk::CommandPool &commandPool,
                                                vk::CommandBuffer &oneTimeBuffer,
                                                vk::Queue &graphicsQueue);

vk::ShaderModule createVulkanShaderModule(vk::Device &device, const vector<char>& code);
