# CMakeList.txt : CMake project for Cursed Engine, include source and define
# project specific logic here.
#

# Add source to this project's executable.
set(ENGINE_TARGET OmniforceEngine CACHE INTERNAL "")
set(Vulkan_SDK $ENV{VULKAN_SDK})

file (GLOB_RECURSE ENGINE_FILES
    "src/*.h"
    "src/*.cpp"
    "src/*.hpp"
)

add_library(${ENGINE_TARGET} ${ENGINE_FILES})

target_include_directories(${ENGINE_TARGET} PUBLIC "src/" PRIVATE "${Vulkan_SDK}\\Include" "${Vulkan_SDK}\\Source")
target_link_directories(${ENGINE_TARGET} INTERFACE "${Vulkan_SDK}\\Lib")
target_compile_definitions(${ENGINE_TARGET} PUBLIC OMNIFORCE_STATIC)

find_package(spdlog CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(unofficial-spirv-reflect CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

target_link_libraries(
    ${ENGINE_TARGET} 
    PUBLIC 
        spdlog::spdlog 
        spdlog::spdlog_header_only
        glm::glm
)

target_link_libraries(
    ${ENGINE_TARGET}
    PRIVATE 
        glfw
        vulkan-1.lib
        unofficial::spirv-reflect::spirv-reflect
        debug shaderc_sharedd.lib optimized shaderc_shared.lib
        debug shaderc_utild.lib optimized shaderc_util.lib
)

set_property(TARGET ${ENGINE_TARGET} PROPERTY CXX_STANDARD 23)
