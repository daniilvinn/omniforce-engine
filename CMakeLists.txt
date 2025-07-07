# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.23)

project ("Omniforce Engine")

if(MSVC)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# Enable compile_commands.json for Ninja generator (for clangd/Cursor)
if(CMAKE_GENERATOR STREQUAL "Ninja")
	set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/Install")

find_package(OpenMP REQUIRED)

# Include sub-projects.
add_subdirectory ("Omniforce/ThirdParty")
set(METATOOL_TARGET MetaTool CACHE INTERNAL "") # HACK

add_subdirectory (Omniforce)
add_subdirectory (Sandbox)
add_subdirectory (Editor)

# Only include ScriptEngine for MSBuild generator (C# projects not compatible with Ninja)
if(NOT CMAKE_GENERATOR STREQUAL "Ninja")
	add_subdirectory (ScriptEngine)
endif()

add_subdirectory ("Tools/MetaTool")

if(MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${EDITOR_TARGET})

	get_filename_component(MSVC_RUNTIME_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

	set(MSVC_RUNTIME_DLLS_RELEASE
        "${MSVC_RUNTIME_DIR}/vcruntime140.dll"
        "${MSVC_RUNTIME_DIR}/msvcp140.dll"
    )

	install(FILES ${MSVC_RUNTIME_DLLS_RELEASE} DESTINATION "${CMAKE_INSTALL_PREFIX}")
endif()