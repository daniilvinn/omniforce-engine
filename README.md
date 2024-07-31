# Omniforce Engine
![photo_2024-02-29_00-40-08](https://github.com/daniilvinn/omniforce-engine/assets/86977774/f8d4fcad-d846-4c93-b531-887861119f7c)

## Features
* Physics based rendering (PBR) based on mesh shaders
* Virtual geometry (WIP)
* Completely custom mesh compression algorithm for runtime decoding
* Instance frustum culling, meshlet frustum / backface culling
* High-performance 2D sprite renderer
* Rigid body simulation with box and sphere colliders based on Jolt Physics.
* C# scripting using Mono runtime - full physics control from C# side, collision callbacks and logging.
* BC7-compressed textures (BC6 and BC5 are planned)
* Project system

## How to build
Build instructions can be found in `BUILD.md`<br/>
The main requirements for the Omniforce engine:
* The engine was only tested with MSVC compiler under Windows 11
* Vulkan SDK 1.3 (was tested specifically with SDK 1.3.280)
* A GPU which supports Mesh Shaders - NVIDIA Turing / AMD RDNA2 or newer.

## Goals
* Virtual geometry renderer to allow rendering of high-fidelity meshes in real-time
* Software rasterizer for micro-polygons
* Ray-traced shadows and reflection with hardware acceleration
* Various post-processing effects, such as chromatic aberration, vignette and other
* Skeletal animations for meshletized virtual geometry

## Used third-party libraries
* GLFW
* VMA
* GLM
* robin-hood hash map
* spdlog
* shaderc
* spirv-reflect
* tinyfiledialogs
* imgui
* ImGuizmo
* stb-image
* EnTT
* nlohmann_json
* Jolt Physics
* bc7enc-rdo
* NVIDIA libdeflate
* volk
* meshoptimizer
* metis
* fmt (pulled from spdlog)
