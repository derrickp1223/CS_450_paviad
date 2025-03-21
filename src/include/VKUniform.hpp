#pragma once
#include <vector>
#include "VKSetup.hpp"
#include "VKUtility.hpp"
#include "VKBuffer.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtc/type_ptr.hpp"

using namespace std;

struct UBOData {
    vector<VulkanBuffer> bufferData;    
    vector<void*> mapped;
};

UBOData createVulkanUniformBufferData(vk::Device &device,
                                vk::PhysicalDevice &physicalDevice,
                                size_t bufferSize, 
                                int maxFramesInFlights=2);
void cleanupVulkanUniformBufferData(vk::Device &device, UBOData &uboData);