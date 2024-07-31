# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.23)

project ("Omniforce Engine")

set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/installation")

find_package(OpenMP REQUIRED)

# Include sub-projects.
add_subdirectory ("Omniforce/vendor")

add_subdirectory (Omniforce)
add_subdirectory (Sandbox)
add_subdirectory (Editor)
add_subdirectory (ScriptEngine)

if(MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${EDITOR_TARGET})
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /W1 /MD<$<CONFIG:Debug>:d>")
endif()