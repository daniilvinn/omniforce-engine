# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.23)

project ("Omniforce Engine")

find_package(OpenMP REQUIRED)

if(MSVC)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /W1")
endif()

# Include sub-projects.
add_subdirectory ("Omniforce/vendor/Jolt/Build")
add_subdirectory ("Omniforce/vendor/volk")
add_subdirectory ("Omniforce/vendor/imgui")
add_subdirectory ("Omniforce/vendor/bc7enc_rdo")
add_subdirectory ("Omniforce/vendor/nv-libdeflate")
add_subdirectory ("Omniforce/vendor/meshoptimizer")
add_subdirectory ("Omniforce/vendor/fastgltf")
add_subdirectory ("Omniforce/vendor/libMETIS")
add_subdirectory (Omniforce)
add_subdirectory (Sandbox)
add_subdirectory (Editor)
add_subdirectory (ScriptEngine)