# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.23)

project ("Omniforce Engine")

find_package(OpenMP REQUIRED)

if(MSVC)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# Include sub-projects.
add_subdirectory ("Omniforce/vendor/Jolt/Build")
add_subdirectory ("Omniforce/vendor/imgui")
add_subdirectory ("Omniforce/vendor/Assimp")
add_subdirectory ("Omniforce/vendor/bc7enc_rdo")
add_subdirectory ("Omniforce/vendor/nv-libdeflate")
add_subdirectory (Omniforce)
add_subdirectory (Sandbox)
add_subdirectory (Editor)
add_subdirectory (ScriptEngine)