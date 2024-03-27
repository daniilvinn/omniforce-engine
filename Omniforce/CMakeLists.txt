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
        "vendor/taskflow"
    PRIVATE 
        "${Vulkan_SDK}\\Include" 
        "${Vulkan_SDK}\\Source" 
        ${Stb_INCLUDE_DIR}
        "vendor/Jolt"
        "vendor/mono/include/mono-2.0"
        "vendor/bc7enc_rdo/include"
        "vendor/nv-libdeflate"
        "vendor/meshoptimizer/src"
        "vendor/fastgltf/include"
        "vendor/NVAftermath/include"
)
target_link_directories(${ENGINE_TARGET} INTERFACE "${Vulkan_SDK}\\Lib" PUBLIC "vendor/mono/lib" "vendor/NVAftermath/lib/x64")
target_compile_definitions(${ENGINE_TARGET} PUBLIC 
    OMNIFORCE_STATIC 
    GLM_FORCE_DEPTH_ZERO_TO_ONE 
    GLM_FORCE_SSE2 
    VK_NO_PROTOTYPES 
    IMGUI_IMPL_VULKAN_NO_PROTOTYPES
    NOMINMAX
    "$<$<CONFIG:Debug>:OMNIFORCE_DEBUG>"
    "$<$<CONFIG:RelWithDebInfo>:OMNIFORCE_RELWITHDEBINFO>"
    "$<$<CONFIG:Release>:OMNIFORCE_RELEASE>"
)

target_link_libraries(
    ${ENGINE_TARGET} 
    PUBLIC 
        spdlog::spdlog 
        spdlog::spdlog_header_only
        glm::glm
        EnTT::EnTT
	    nlohmann_json::nlohmann_json
        ${OMNI_IMGUI_TARGET}
        volk_headers
)

target_link_libraries(
    ${ENGINE_TARGET}
    PRIVATE 
        glfw
        debug shaderc_sharedd.lib optimized shaderc_shared.lib
        debug shaderc_utild.lib optimized shaderc_util.lib
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