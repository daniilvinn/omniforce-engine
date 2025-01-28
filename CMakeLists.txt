# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.23)

project ("Omniforce Engine")

if(MSVC)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/Install")

find_package(OpenMP REQUIRED)

# Include sub-projects.
add_subdirectory ("Omniforce/ThirdParty")

add_subdirectory (Omniforce)
add_subdirectory (Sandbox)
add_subdirectory (Editor)
add_subdirectory (ScriptEngine)
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