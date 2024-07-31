### ImGui
set(OMNI_IMGUI_TARGET imgui CACHE INTERNAL "")

file(GLOB_RECURSE OMNI_IMGUI_FILES
	"imgui/imconfig.h"
	"imgui/imgui.h"
	"imgui/imgui.cpp"
	"imgui/imgui_demo.cpp"
	"imgui/imgui_draw.cpp"
	"imgui/imgui_internal.h"
	"imgui/imgui_tables.cpp"
	"imgui/imgui_widgets.cpp"
	"imgui/imgui_rectpack.h"
	"imgui/imgui_textedit.h"
	"imgui/imgui_truetype.h"

	"imgui/misc/cpp/imgui_stdlib.h"
	"imgui/misc/cpp/imgui_stdlib.cpp"

	"imgui/imgui_impl_glfw.cpp"
	"imgui/imgui_impl_glfw.h"

	"imgui/imgui_impl_vulkan.cpp"
	"imgui/imgui_impl_vulkan.h"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${OMNI_IMGUI_FILES})

add_library(${OMNI_IMGUI_TARGET} ${OMNI_IMGUI_FILES})

set(Vulkan_SDK $ENV{VULKAN_SDK})

target_include_directories(${OMNI_IMGUI_TARGET} PUBLIC ${IMGUI_INCLUDE_PATH}
	"${Vulkan_SDK}\\Include" 
	"${Vulkan_SDK}\\Source" 
	"imgui"
)

target_compile_definitions(${OMNI_IMGUI_TARGET} PRIVATE VK_NO_PROTOTYPES IMGUI_IMPL_VULKAN_NO_PROTOTYPES)

target_link_libraries(
	${OMNI_IMGUI_TARGET}
	PRIVATE 
		glfw
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${OMNI_IMGUI_TARGET} PROPERTY CXX_STANDARD 23)
endif()

### NVIDIA libdeflate
project(nv_libdeflate)

file(GLOB_RECURSE NV_LIBDEFLATE_FILES
	"nv-libdeflate/lib/*.*"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${NV_LIBDEFLATE_FILES})

add_library(nv_libdeflate ${NV_LIBDEFLATE_FILES})

target_include_directories(nv_libdeflate PRIVATE "nv-libdeflate")

### Jolt Physics
project(Jolt)

file(GLOB_RECURSE JOLT_FILES
	"Jolt/Jolt/*.h"
	"Jolt/Jolt/*.cpp"
	"Jolt/Jolt/*.hpp"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${JOLT_FILES})

add_library(Jolt ${JOLT_FILES})

target_include_directories(Jolt PRIVATE "Jolt")

target_compile_options(Jolt PRIVATE /MP)

set_property(TARGET Jolt PROPERTY CXX_STANDARD 23)

### meshoptimizer
add_subdirectory("meshoptimizer")

### bc7enc_rdo
add_subdirectory("bc7enc_rdo")

### Volk
add_subdirectory("volk")

### fastgltf
add_subdirectory("fastgltf")

### libMETIS
add_subdirectory("libMETIS")

### spdlog
add_subdirectory("spdlog")

### GLFW
add_subdirectory("GLFW")

### Disable warnings for METIS and GKlib
if(MSVC)
	target_compile_options(metis PRIVATE /W0)
	target_compile_options(GKlib PRIVATE /W0)
endif()