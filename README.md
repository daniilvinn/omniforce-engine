# Omniforce Engine
![photo_2024-02-29_00-40-08](https://github.com/daniilvinn/omniforce-engine/assets/86977774/f8d4fcad-d846-4c93-b531-887861119f7c)

## Features
* Physics based rendering (PBR)
* Mesh shading
* Mesh frustum, meshlet frustum / backface culling
* High-performance 2D sprite renderer. It requires no VRAM to form vertex buffers, because it is not even used. All sprites are drawn in one draw call.
* Rigid body simulation with box and sphere colliders based on Jolt Physics.
* C# scripting based on Mono Runtime - full physics control from C# side, collision callbacks and logging.
* BC7 encoding for images (BC6 and BC5 are planned)

## Goals
* Skeletal animations
* High-fidelity renderer with various available graphics features

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
* bc7enc-rdo
* NVIDIA libdeflate
* volk
