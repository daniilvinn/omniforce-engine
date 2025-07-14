# Omniforce Engine

<div align="center">

![Omniforce Engine](https://github.com/user-attachments/assets/ee73bddb-8a9f-4747-a64b-5d20ebd2c256)
![Engine Screenshot](https://github.com/user-attachments/assets/5358ed9c-4c21-41ab-9c08-689600c40b28)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-Windows-blue.svg)](https://www.microsoft.com/windows)
[![Vulkan](https://img.shields.io/badge/Vulkan-1.3-red.svg)](https://www.vulkan.org/)
[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)

*A high-performance, real-time rendering engine with hardware-accelerated ray tracing and advanced graphics features*

</div>

## 📋 Table of Contents

- [Overview](#overview)
- [✨ Features](#-features)
- [🚀 Quick Start](#-quick-start)
- [📋 Requirements](#-requirements)
- [🔧 Building](#-building)
- [📚 Documentation](#-documentation)
- [🎯 Roadmap](#-roadmap)
- [🤝 Contributing](#-contributing)
- [📄 License](#-license)

## Overview

Omniforce Engine is a cutting-edge real-time rendering engine that leverages modern GPU technologies to deliver stunning visual fidelity. Built with Vulkan and designed for high-performance applications, it features hardware-accelerated ray tracing, advanced mesh processing, and a comprehensive physics simulation system.

## ✨ Features

### 🎨 Rendering & Graphics
- **Hardware Ray Tracing**: Path tracing using Vulkan RT API for photorealistic lighting
- **Physics-Based Rendering (PBR)**: Advanced material system with mesh shader support
- **Virtual Geometry**: High-fidelity mesh rendering with real-time LOD systems
- **Custom Mesh Compression**: Proprietary algorithm for efficient runtime mesh decoding
- **Advanced Culling**: Instance frustum culling, meshlet frustum/backface culling
- **2D Sprite Renderer**: High-performance 2D graphics system

### 🎮 Physics & Simulation
- **Rigid Body Physics**: Box and sphere colliders powered by Jolt Physics
- **Collision Detection**: Real-time collision callbacks and physics logging
- **Physics Integration**: Full physics control from scripting layer

### 💻 Scripting & Development
- **C# Scripting**: Mono runtime integration with full engine API access
- **Project System**: Complete project management and asset pipeline
- **Hot Reload**: Real-time shader and script reloading capabilities

### 🖼️ Asset Pipeline
- **Texture Compression**: BC7-compressed textures (BC6 and BC5 planned)
- **Asset Management**: Comprehensive asset import and management system
- **Mesh Optimization**: Advanced mesh processing and optimization tools

## 🚀 Quick Start

### Prerequisites
- Windows 10/11
- GPU with mesh shader support (NVIDIA Turing+ / AMD RDNA2+)
- Vulkan SDK 1.3+
- CMake 3.23+
- Python 3.13.1

### Installation

1. **Clone the repository**
   ```bash
   git clone --recursive https://github.com/daniilvinn/omniforce-engine.git
   cd omniforce-engine
   ```

2. **Run the setup script**
   ```bash
   cd Scripts
   Setup.bat
   ```

3. **Generate and build projects**
   ```bash
   GenerateProjects.bat
   # or for complete build and install
   BuildAndInstall.bat
   ```

For detailed build instructions, see [BUILD.md](BUILD.md).

## 📋 Requirements

### System Requirements
- **OS**: Windows 10/11 (64-bit)
- **GPU**: NVIDIA Turing (RTX 20 series) or newer / AMD RDNA2 or newer
- **Memory**: 8GB RAM minimum, 16GB recommended
- **Storage**: 2GB available space

### Development Requirements
- **Compiler**: MSVC (Visual Studio 2019 or newer)
- **Vulkan SDK**: 1.3.280 or newer
- **CMake**: 3.23 or newer
- **Python**: 3.13.1

## 🔧 Building

The engine uses CMake for build configuration. Several build scripts are provided for convenience:

| Script | Purpose |
|--------|---------|
| `Setup.bat` | Checks dependencies and generates initial project files |
| `GenerateProjects.bat` | Creates IDE project files in `Build/` directory |
| `BuildAndInstall.bat` | Complete build and installation process |

### Manual Build Process

1. Ensure all [requirements](#-requirements) are met
2. Run `Scripts/Setup.bat` to verify dependencies
3. Generate projects: `Scripts/GenerateProjects.bat`
4. Build using your preferred IDE or command line
5. Find the built engine in `Install/` directory

## 📚 Documentation

- **[BUILD.md](BUILD.md)** - Detailed build instructions
- **[Shader Hot Reload](Docs/ShaderHotReload.md)** - Shader development workflow
- **[Translucency](Docs/Translucency.md)** - Translucency rendering guide

## 🎯 Roadmap

### Planned Features
- **Virtual Geometry Renderer**: High-fidelity mesh rendering in real-time
- **Software Rasterizer**: Micro-polygon rendering system
- **Enhanced Ray Tracing**: Hardware-accelerated shadows and reflections
- **Post-Processing Pipeline**: Chromatic aberration, vignette, and more effects
- **Skeletal Animation**: Advanced animation system for meshletized geometry

### Current Development Focus
- Performance optimization and stability improvements
- Enhanced asset pipeline
- Extended scripting capabilities
- Additional rendering features

## 🤝 Contributing

We welcome contributions! Please feel free to submit issues, feature requests, or pull requests.

### Development Setup
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

### Code Style
- Follow the existing code style and conventions
- Ensure all code compiles without warnings
- Add appropriate documentation for new features

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## 📦 Third-Party Libraries

Omniforce Engine uses the following third-party libraries:

### Graphics & Rendering
- **Vulkan Memory Allocator (VMA)** - Memory management
- **GLM** - Mathematics library
- **shaderc** - Shader compilation
- **spirv-reflect** - SPIR-V reflection

### Physics & Simulation
- **Jolt Physics** - Physics simulation engine

### UI & Development
- **Dear ImGui** - Immediate mode GUI
- **ImGuizmo** - 3D manipulation gizmos
- **GLFW** - Window management and input

### Utilities
- **spdlog** - Logging library
- **nlohmann/json** - JSON parsing
- **EnTT** - Entity component system
- **robin-hood-hashing** - High-performance hash maps
- **meshoptimizer** - Mesh optimization
- **bc7enc-rdo** - Texture compression
- **miniaudio** - Audio processing

### Additional Libraries
- **volk** - Vulkan loader
- **stb_image** - Image loading
- **tinyfiledialogs** - File dialogs
- **fmt** - String formatting
- **metis** - Graph partitioning

---

<div align="center">

**Made with ❤️ by the Daniil Vinnik**

[GitHub](https://github.com/daniilvinn/omniforce-engine) • [Issues](https://github.com/daniilvinn/omniforce-engine/issues) • [Discussions](https://github.com/daniilvinn/omniforce-engine/discussions)

</div>
