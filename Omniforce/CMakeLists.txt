# CMakeList.txt : CMake project for Omniforce Engine, include source and define
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
    "vendor/taskflow/taskflow/*.h"
    "vendor/taskflow/taskflow/*.cpp"
    "vendor/taskflow/taskflow/*.hpp"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ENGINE_FILES})

add_library(${ENGINE_TARGET} ${ENGINE_FILES})

target_include_directories(${ENGINE_TARGET} 
    PUBLIC 
        "src/"
        "vendor/imgui"
        "vendor/ImGuizmo"
        "vendor/taskflow"
        "vendor/glm"
        "vendor/spdlog/include"
        "vendor/EnTT/src"
        "vendor/nlohmann-json/single_include"
        "vendor/robin-hood-hashing/src/include"
    PRIVATE 
        "${Vulkan_SDK}\\Include" 
        "${Vulkan_SDK}\\Source" 
        "vendor/stb"
        "vendor/Jolt"
        "vendor/mono/include/mono-2.0"
        "vendor/bc7enc_rdo/include"
        "vendor/nv-libdeflate"
        "vendor/meshoptimizer/src"
        "vendor/fastgltf/include"
        "vendor/NVAftermath/include"
        "vendor/miniball/cpp/main"
        "vendor/libMETIS/include"
        "vendor/GLFW/include"
        "vendor/VulkanMemoryAllocator/include"
)
target_link_directories(${ENGINE_TARGET} PUBLIC 
    "${Vulkan_SDK}\\Lib" 
    "${Vulkan_SDK}\\Bin" 
    "vendor/mono/lib" 
    "vendor/NVAftermath/lib/x64"
)
target_compile_definitions(${ENGINE_TARGET} PUBLIC 
    OMNIFORCE_STATIC 
    GLM_FORCE_DEPTH_ZERO_TO_ONE 
    GLM_FORCE_SSE2 
    GLM_ENABLE_EXPERIMENTAL
    VK_NO_PROTOTYPES 
    IMGUI_IMPL_VULKAN_NO_PROTOTYPES
    NOMINMAX
    "$<$<CONFIG:Debug>:OMNIFORCE_DEBUG>"
    "$<$<CONFIG:RelWithDebInfo>:OMNIFORCE_RELWITHDEBINFO>"
    "$<$<CONFIG:Release>:OMNIFORCE_RELEASE>"
    IDXTYPEWIDTH=64
    REALTYPEWIDTH=64
    WIN32_LEAN_AND_MEAN
)

target_link_libraries(
    ${ENGINE_TARGET} 
    PUBLIC
        ${OMNI_IMGUI_TARGET}
        volk_headers
        metis
        spdlog
)

target_link_libraries(
    ${ENGINE_TARGET}
    PRIVATE 
        debug shaderc_sharedd.lib optimized shaderc_shared.lib
        debug shaderc_utild.lib optimized shaderc_util.lib
        glfw
        Jolt
        libmono-static-sgen.lib
        MonoPosixHelper.lib
        Ws2_32.lib
        Winmm.lib
        Version.lib
        Bcrypt.lib
        bc7enc_rdo
        OpenMP::OpenMP_CXX
        nv_libdeflate
        meshoptimizer
        fastgltf
)

set_property(TARGET ${ENGINE_TARGET} PROPERTY CXX_STANDARD 23)

omni_set_project_ide_folder(${ENGINE_TARGET} ${CMAKE_CURRENT_SOURCE_DIR})