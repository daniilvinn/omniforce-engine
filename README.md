# Omniforce Engine
## A game engine for 2D games.

## Features
* High-performance 2D sprite renderer. It requires no VRAM to form vertex buffers, because it is not even used. All sprites are drawn in one draw call.
* Rigid body simulation based on Jolt Physics.


## Goals
* C# scripting
* Audio system
* Engine-managed 2D animations
* Introduce multithreading to physics rigid body simulation and to the renderer.

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