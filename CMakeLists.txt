cmake_minimum_required(VERSION 3.24) 
project(CS_450_Assignments)
set(CMAKE_INSTALL_PREFIX .)

# Comment out this line if it creates problems
set(CMAKE_PREFIX_PATH "/Users/dp/VulkanSDK/1.4.304.0")

#####################################
# Require C++17
#####################################

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#####################################
# Find necessary libraries
#####################################

include(FetchContent)

#####################################
# Vulkan
#####################################

find_package(Vulkan REQUIRED)

#####################################
# GLFW
#####################################

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw
    GIT_TAG 3.4   
    GIT_SHALLOW 1
)
FetchContent_MakeAvailable(glfw)

#####################################
# GLM
#####################################

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG 1.0.1  
    GIT_SHALLOW 1
)

FetchContent_MakeAvailable(glm)

#####################################
# Assimp
#####################################

set(CMAKE_POLICY_DEFAULT_CMP0175 OLD)

FetchContent_Declare(
    Assimp
    GIT_REPOSITORY https://github.com/assimp/assimp
    GIT_TAG v5.4.3
    GIT_SHALLOW 1
)

set("ASSIMP_INSTALL_PDB" OFF CACHE BOOL "" FORCE)
set("BUILD_SHARED_LIBS" OFF CACHE BOOL "" FORCE)
set("ASSIMP_BUILD_TESTS" OFF CACHE BOOL "" FORCE)
set("ASSIMP_BUILD_SAMPLES" OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(Assimp)

#####################################
# VK-Bootstrap
#####################################

FetchContent_Declare(
    fetch_vk_bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
    GIT_TAG v1.3.302
    GIT_SHALLOW 1
)

set("VK_BOOTSTRAP_VULKAN_HEADER_DIR" ${Vulkan_INCLUDE_DIRS} CACHE FILEPATH "" FORCE)

FetchContent_MakeAvailable(fetch_vk_bootstrap)

#####################################
# Get general sources
#####################################

include_directories("src/include" Vulkan::Headers)

message(${Vulkan_INCLUDE_DIRS})

file(GLOB GENERAL_SOURCES    
    "src/lib/*.cpp"  
    "src/lib/*.hpp"  
)

#####################################
# SPIR-V COMPILATION
#####################################

# Adapted from: https://gist.github.com/evilactually/a0d191701cb48f157b05be7f74d79396
macro(COMPILE_VULKAN_SHADERS target)
    file(GLOB SHADER_SOURCES
        "vulkanshaders/${target}/*.vert"
        "vulkanshaders/${target}/*.frag"
    )

    foreach(GLSL ${SHADER_SOURCES})
        #message(${GLSL})
        cmake_path(GET GLSL FILENAME filename)        
        #message(${filename}) 
        set(SPIRV "${PROJECT_BINARY_DIR}/compiledshaders/${target}/${filename}.spv")
        #message(${SPIRV})
        #message("${PROJECT_BINARY_DIR}/compiledshaders/${target}/")
        #message(Vulkan::glslc)
        	  
        add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND cd
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${PROJECT_BINARY_DIR}/compiledshaders/${target}/"
            COMMAND Vulkan::glslc ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL})
        list(APPEND SPIRV_BINARY_FILES ${SPIRV})
    endforeach(GLSL)

    add_custom_target(
	    Shaders_${target}
	    DEPENDS ${SPIRV_BINARY_FILES}
	)

	add_dependencies(${target} Shaders_${target})

endmacro()

#####################################
# Create executables 
# and install targets
#####################################

set(ALL_LIBRARIES 
    Vulkan::Vulkan    
    glfw   
    glm::glm  
    assimp::assimp
    vk-bootstrap::vk-bootstrap)

macro(CREATE_VULKAN_EXECUTABLE target)
    add_executable(${target} ${GENERAL_SOURCES} "./src/app/${target}.cpp")
    target_link_libraries(${target} PRIVATE ${ALL_LIBRARIES})
    install(TARGETS ${target} RUNTIME DESTINATION bin/${target})
    COMPILE_VULKAN_SHADERS(${target})
    install(DIRECTORY ${PROJECT_BINARY_DIR}/compiledshaders/${target} DESTINATION bin/${target}/build/compiledshaders)
endmacro()

CREATE_VULKAN_EXECUTABLE(BasicVulkan)
CREATE_VULKAN_EXECUTABLE(Assign01)

