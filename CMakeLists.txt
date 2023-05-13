# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.23)

project ("Omniforce Engine")

# Include sub-projects.
add_subdirectory ("Omniforce/vendor/imgui")
add_subdirectory (Omniforce)
add_subdirectory (Sandbox)
add_subdirectory (Editor)