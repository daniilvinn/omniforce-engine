# Omniforce Engine

## Features
* High-performance 2D sprite renderer. It requires no VRAM to form vertex buffers, because it is not even used. All sprites are drawn in one draw call.
* Rigid body simulation with box and sphere colliders based on Jolt Physics.
* C# scripting based on Mono Runtime - full physics control from C# side, collision callbacks and logging.
* BC7 encoding for images (BC6 and BC5 are planned)

## Goals
* Audio system
* Engine-managed 2D animations
* Introduce multithreading to physics rigid body simulation and to the renderer.
* Capability of handling high-fidelity meshes and textures using various optimization techniques

## Used third-party libraries
* GLFW
* VMA
* GLM
* robin-hood hash map
* spdlog
* shaderc
* spirv-reflect
* fmt
* tinyfiledialogs
* imgui
* stb-image
* EnTT
* nlohmann_json
* Jolt Physics
* bc7enc
* NVIDIA libdeflate