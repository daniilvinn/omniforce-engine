set(SANDBOX_TARGET Sandbox CACHE INTERNAL "")

file(GLOB_RECURSE SANDBOX_FILES
	"Source/*.h"
	"Source/*.cpp"
	"Source/*.hpp"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SANDBOX_FILES})

add_executable(${SANDBOX_TARGET} ${SANDBOX_FILES})

target_link_libraries(${SANDBOX_TARGET} PUBLIC OmniforceEngine ${OMNI_IMGUI_TARGET})
target_include_directories(${SANDBOX_TARGET} PUBLIC ${IMGUI_INCLUDE_PATH})

set_target_properties(${SANDBOX_TARGET} PROPERTIES
    CXX_STANDARD 23
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"   
)

if(WIN32)
set_target_properties(${SANDBOX_TARGET} PROPERTIES
	WIN32_EXECUTABLE $<$<CONFIG:Release>:1>
)
endif()

omni_set_project_ide_folder(${SANDBOX_TARGET} ${CMAKE_CURRENT_SOURCE_DIR})

project(CreateSandboxResourcesSymlink)

# Define symbolic link and target folder paths
set(SHADERS_FOLDER_SYMLINK_NAME "${CMAKE_CURRENT_SOURCE_DIR}/Resources")
set(SHADERS_FOLDER_TARGET_NAME "${CMAKE_SOURCE_DIR}/Editor/Resources")

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
        add_custom_target(CreateSandboxResourcesSymlink ALL DEPENDS "${NATIVE_SHADERS_FOLDER_SYMLINK_NAME}")
    else()
        message(STATUS "Symbolic link already exists: ${NATIVE_SHADERS_FOLDER_SYMLINK_NAME}")
    endif()
else()
    message(FATAL_ERROR "This script is intended for Windows only.")
endif()