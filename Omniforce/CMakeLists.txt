# CMakeList.txt : CMake project for Omniforce Engine, include source and define
# project specific logic here.
#

include("../utils.cmake")

# Add source to this project's executable.
set(ENGINE_TARGET OmniforceEngine CACHE INTERNAL "")
set(Vulkan_SDK $ENV{VULKAN_SDK})
set(PATH_ENV $ENV{PATH})

foreach(dir IN LISTS PATH_ENV)
    if(dir MATCHES "LLVM")
        get_filename_component(LLVM_ROOT "${dir}" DIRECTORY)
        message(STATUS "Path to LLVM: ${LLVM_ROOT}")
        break()
    endif()
endforeach()

if(NOT LLVM_ROOT)
    message(FATAL_ERROR "Cannot find LLVM")
endif()

file(GLOB_RECURSE ENGINE_SOURCES
    "Source/*.h"
    "Source/*.cpp"
    "Source/*.hpp"
)

file(GLOB_RECURSE ENGINE_HEADERS
    "Source/*.h"
)

file(GLOB_RECURSE ENGINE_THIRDPARTY_SOURCES
   "ThirdParty/ImGuizmo/ImGuizmo.h"
   "ThirdParty/ImGuizmo/ImGuizmo.cpp"
   "ThirdParty/taskflow/taskflow/*.h"
   "ThirdParty/taskflow/taskflow/*.cpp"
   "ThirdParty/taskflow/taskflow/*.hpp"
   "ThirdParty/tinyfiledialogs/tinyfiledialogs.c"
   "ThirdParty/tinyfiledialogs/more_dialogs/tinyfd_moredialogs.c"
)

file (GLOB_RECURSE ENGINE_FILES
    ${ENGINE_SOURCES}
    ${ENGINE_THIRDPARTY_SOURCES}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ENGINE_FILES})

add_library(${ENGINE_TARGET} ${ENGINE_FILES})

target_include_directories(${ENGINE_TARGET} 
    PUBLIC 
        "Source/"
    PUBLIC
        "ThirdParty/imgui"
        "ThirdParty/ImGuizmo"
        "ThirdParty/taskflow"
        "ThirdParty/glm"
        "ThirdParty/spdlog/include"
        "ThirdParty/EnTT/src"
        "ThirdParty/nlohmann-json/single_include"
        "ThirdParty/robin-hood-hashing/src/include"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/Shaders"
        "ThirdParty/tinyfiledialogs"
        "ThirdParty/miniaudio"
        "ThirdParty/slang/include"
        "${Vulkan_SDK}\\Include" 
        "${Vulkan_SDK}\\Source" 
        "${LLVM_ROOT}\\include"
        "ThirdParty/stb"
        "ThirdParty/Jolt"
        "ThirdParty/mono/include/mono-2.0"
        "ThirdParty/bc7enc_rdo/include"
        "ThirdParty/nv-libdeflate"
        "ThirdParty/meshoptimizer/src"
        "ThirdParty/fastgltf/include"
        "ThirdParty/NVAftermath/include"
        "ThirdParty/miniball/cpp/main"
        "ThirdParty/libMETIS/include"
        "ThirdParty/GLFW/include"
        "ThirdParty/VulkanMemoryAllocator/include"
        "ThirdParty/volk"
)
target_link_directories(${ENGINE_TARGET} PUBLIC 
    "${Vulkan_SDK}\\Lib" 
    "${Vulkan_SDK}\\Bin" 
    "ThirdParty/mono/lib" 
    "ThirdParty/NVAftermath/lib/x64"
    "ThirdParty/slang"
    "${LLVM_ROOT}\\lib"
)
target_compile_definitions(${ENGINE_TARGET} 
PUBLIC 
    OMNIFORCE_STATIC 
    GLM_FORCE_DEPTH_ZERO_TO_ONE 
    GLM_FORCE_SSE2 
    GLM_ENABLE_EXPERIMENTAL
    VK_NO_PROTOTYPES 
    IMGUI_IMPL_VULKAN_NO_PROTOTYPES
    NOMINMAX
    "$<$<CONFIG:Debug>:OMNIFORCE_DEBUG>"
    "$<$<CONFIG:Debug>:VMA_LEAK_LOG_FORMAT>"
    "$<$<CONFIG:RelWithDebInfo>:OMNIFORCE_RELWITHDEBINFO>"
    "$<$<CONFIG:Release>:OMNIFORCE_RELEASE>"
    IDXTYPEWIDTH=64
    REALTYPEWIDTH=64
    WIN32_LEAN_AND_MEAN
    __OMNI_COMPILE_SHADER_FOR_CXX
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
        slang.lib
        libclang.lib
)

target_compile_options(
    ${ENGINE_TARGET}
    PUBLIC
        /MP
        /W1
)

target_precompile_headers(${ENGINE_TARGET} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/Source/Foundation/Common.h>")

set_property(TARGET ${ENGINE_TARGET} PROPERTY CXX_STANDARD 23)

omni_set_project_ide_folder(${ENGINE_TARGET} ${CMAKE_CURRENT_SOURCE_DIR})

INSTALL(FILES "${Vulkan_SDK}\\Bin\\shaderc_shared.dll" DESTINATION "${CMAKE_INSTALL_PREFIX}")

project(CreateCoreShadersSymlink)

# Define symbolic link and target folder paths
set(SHADERS_FOLDER_SYMLINK_NAME "${CMAKE_SOURCE_DIR}/Editor/Resources/Shaders")
set(SHADERS_FOLDER_TARGET_NAME "${CMAKE_CURRENT_SOURCE_DIR}/Shaders")

if(WIN32)
    # Convert paths to Windows-native format (uses backslashes)
    file(TO_NATIVE_PATH "${SHADERS_FOLDER_SYMLINK_NAME}" NATIVE_SHADERS_FOLDER_SYMLINK_NAME)
    file(TO_NATIVE_PATH "${SHADERS_FOLDER_TARGET_NAME}" NATIVE_SHADERS_FOLDER_TARGET_NAME)

    # Check if the symbolic link already exists
    if(NOT EXISTS "${NATIVE_SHADERS_FOLDER_SYMLINK_NAME}")
        add_custom_command(
            OUTPUT "${NATIVE_SHADERS_FOLDER_SYMLINK_NAME}"
            COMMAND cmake -E echo "Creating symbolic link: ${NATIVE_SHADERS_FOLDER_SYMLINK_NAME} -> ${NATIVE_SHADERS_FOLDER_TARGET_NAME}"
            COMMAND cmd /c mklink /J "${NATIVE_SHADERS_FOLDER_SYMLINK_NAME}" "${NATIVE_SHADERS_FOLDER_TARGET_NAME}"
            COMMENT "Creating symbolic link: ${NATIVE_SHADERS_FOLDER_SYMLINK_NAME} -> ${NATIVE_SHADERS_FOLDER_TARGET_NAME}"
        )

        # Create a custom target for the symbolic link creation
        add_custom_target(CreateCoreShadersSymlinkTarget ALL DEPENDS "${NATIVE_SHADERS_FOLDER_SYMLINK_NAME}")
    else()
        message(STATUS "Symbolic link already exists: ${NATIVE_SHADERS_FOLDER_SYMLINK_NAME}")
    endif()
else()
    message(FATAL_ERROR "This script is intended for Windows only.")
endif()

set(PARSE_TARGETS_FILE "${CMAKE_BINARY_DIR}/MetaToolGen/ParseTargets.txt")

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/MetaToolGen/DummyOut")
file(TOUCH ${PARSE_TARGETS_FILE})

add_custom_target(
    ClearParseTargets ALL
    COMMAND ${CMAKE_COMMAND} -E rm -f ${PARSE_TARGETS_FILE}
    COMMAND ${CMAKE_COMMAND} -E touch ${PARSE_TARGETS_FILE}
)

set(GEN_OUT_FILES) 

foreach(SOURCE_FILE ${ENGINE_HEADERS})
    get_filename_component(FILE_NAME_WITH_EXTENSION ${SOURCE_FILE} NAME)
    set(GEN_FILE "${CMAKE_BINARY_DIR}/MetaToolGen/DummyOut/${FILE_NAME_WITH_EXTENSION}.gen")

    if(NOT GEN_FILE IN_LIST GEN_OUT_FILES)
        list(APPEND GEN_OUT_FILES ${GEN_FILE})
        add_custom_command(
            OUTPUT ${GEN_FILE}
            COMMAND ${CMAKE_COMMAND} -E echo ${SOURCE_FILE} >> ${PARSE_TARGETS_FILE}
            COMMAND ${CMAKE_COMMAND} -E touch ${GEN_FILE}
            COMMENT ""
            DEPENDS ${SOURCE_FILE}
            VERBATIM
        )
    endif()
endforeach()

add_custom_target(
    MetaToolRunner ALL
    COMMAND ${METATOOL_TARGET}
    DEPENDS ${GEN_OUT_FILES} ${METATOOL_TARGET} ClearParseTargets
)

set_target_properties(ClearParseTargets PROPERTIES FOLDER "MetaToolActions")
set_target_properties(MetaToolRunner PROPERTIES FOLDER "MetaToolActions")

# setup slangdconfig.json
# Read the file content
file(READ "${CMAKE_SOURCE_DIR}/Omniforce/Shaders/slangdconfig.json" SLANG_CONFIG_CONTENTS)

# Replace "World" with "CMake"
string(REPLACE "CMAKE_GENERATED_SHADER_PATH" "${CMAKE_SOURCE_DIR}/Omniforce/Shaders/" SLANG_CONFIG_CONTENTS "${SLANG_CONFIG_CONTENTS}")

# Write the modified content to a new file
file(WRITE "${CMAKE_SOURCE_DIR}/Omniforce/Shaders/slangdconfig.json" "${SLANG_CONFIG_CONTENTS}")

add_dependencies(${ENGINE_TARGET} MetaToolRunner)