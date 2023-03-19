# Omniforce Engine
## A game engine, designed to be fully multithreaded, capable of running single-player games and aimed for rendering stunning graphics.

## Features
* Fully multithreaded from top to bottom. The engine uses its custom job system to dispatch work for another threads, such as audio computation, skinning, command-buffer recording and so on.
* Vulkan 1.3. Cursed Engine uses Vulkan to render graphics, thus it is capable of using full power of the GPU

## Goals
* PBR renderer with shader permutation
* 3D physics using NVIDIA PhysX, skeleton animation and ragdolls based on PhysX joints
* Simple audio engine to play game's OST without complex audio physics simulation (for now).
* Editor 