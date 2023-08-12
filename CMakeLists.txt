# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.23)

project ("Omniforce Engine")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /W3")

if(MSVC)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# Include sub-projects.
add_subdirectory ("Omniforce/vendor/Jolt/Build")
add_subdirectory ("Omniforce/vendor/imgui")
add_subdirectory (Omniforce)
add_subdirectory (Sandbox)
add_subdirectory (Editor)
add_subdirectory (ScriptEngine)
