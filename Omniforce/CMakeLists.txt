# CMakeList.txt : CMake project for Cursed Engine, include source and define
# project specific logic here.
#

include("../utils.cmake")

# Add source to this project's executable.
set(ENGINE_TARGET OmniforceEngine CACHE INTERNAL "")
set(Vulkan_SDK $ENV{VULKAN_SDK})

file (GLOB_RECURSE ENGINE_FILES
    "src/*.h"
    "src/*.cpp"
    "src/*.hpp"
    "vendor/ImGuizmo/ImGuizmo.h"
    "vendor/ImGuizmo/ImGuizmo.cpp"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ENGINE_FILES})

add_library(${ENGINE_TARGET} ${ENGINE_FILES})

find_package(spdlog CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_include_directories(${ENGINE_TARGET} 
    PUBLIC 
        "src/"
        "vendor/imgui"
        "vendor/ImGuizmo"
    PRIVATE 
        "${Vulkan_SDK}\\Include" 
        "${Vulkan_SDK}\\Source" 
        ${Stb_INCLUDE_DIR}
        "vendor/Jolt"
        "vendor/mono/include/mono-2.0"
        "vendor/Assimp/include"
)
target_link_directories(${ENGINE_TARGET} INTERFACE "${Vulkan_SDK}\\Lib" PUBLIC "vendor/mono/lib")
target_compile_definitions(${ENGINE_TARGET} PUBLIC OMNIFORCE_STATIC GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_SSE2)

target_link_libraries(
    ${ENGINE_TARGET} 
    PUBLIC 
        spdlog::spdlog 
        spdlog::spdlog_header_only
        glm::glm
        EnTT::EnTT
	    nlohmann_json::nlohmann_json
        ${OMNI_IMGUI_TARGET}
       
)

target_link_libraries(
    ${ENGINE_TARGET}
    PRIVATE 
        glfw
        vulkan-1.lib
        debug shaderc_sharedd.lib optimized shaderc_shared.lib
        debug shaderc_utild.lib optimized shaderc_util.lib
        Jolt
        libmono-static-sgen.lib
        MonoPosixHelper.lib
        Ws2_32.lib
        Winmm.lib
        Version.lib
        Bcrypt.lib
        assimp
)

set_property(TARGET ${ENGINE_TARGET} PROPERTY CXX_STANDARD 23)

omni_set_project_ide_folder(${ENGINE_TARGET} ${CMAKE_CURRENT_SOURCE_DIR})