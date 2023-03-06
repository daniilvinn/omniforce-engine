# CMakeList.txt : CMake project for Cursed Engine, include source and define
# project specific logic here.
#

# Add source to this project's executable.
set(ENGINE_TARGET CursedEngine CACHE INTERNAL "")

file (GLOB_RECURSE ENGINE_FILES
    "src/*.h"
    "src/*.cpp"
    "src/*.hpp"
)

add_library(${ENGINE_TARGET} ${ENGINE_FILES})

target_include_directories(${ENGINE_TARGET} PUBLIC "src/")
target_compile_definitions(${ENGINE_TARGET} PUBLIC CURSED_STATIC)

target_precompile_headers(${ENGINE_TARGET} PRIVATE "src/cursed_pch.h")

find_package(spdlog CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${ENGINE_TARGET} PUBLIC spdlog::spdlog spdlog::spdlog_header_only PRIVATE glfw)

set_property(TARGET ${ENGINE_TARGET} PROPERTY CXX_STANDARD 23)


# TODO: Add tests and install targets if needed.
