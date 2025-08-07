# MetaTool - C++ Code Generation Tool

<div align="center">

*A powerful C++ code generation tool that analyzes source code using Clang's AST and generates shader code for the Omniforce Engine*

</div>

## 📋 Table of Contents

- [Overview](#overview)
- [✨ Features](#-features)
- [🔧 Architecture](#-architecture)
- [📋 Requirements](#-requirements)
- [🚀 Usage](#-usage)
- [⚙️ Configuration](#️-configuration)
- [📊 Performance](#-performance)
- [🔍 Code Analysis](#-code-analysis)
- [📝 Generated Code](#-generated-code)
- [⚡ Caching System](#-caching-system)
- [🎯 Use Cases](#-use-cases)
- [🚧 Limitations](#-limitations)
- [🤝 Contributing](#-contributing)

## Overview

MetaTool is a sophisticated C++ code generation tool designed specifically for the Omniforce Engine. It leverages Clang's powerful Abstract Syntax Tree (AST) parsing capabilities to analyze C++ source code and automatically generate shader source code. The tool is particularly focused on generating shader input structures, shared types and other boilerplate that would be tedious to write manually.

## ✨ Features

### 🔍 **Advanced Code Analysis**
- **Clang AST Parsing**: Deep analysis of C++ source code using LLVM's Clang
- **Multi-threaded Processing**: Parallel parsing of multiple source files
- **Dependency Resolution**: Automatic detection and handling of type dependencies
- **Template Support**: Full C++ template instantiation and analysis

### 🏗️ **Code Generation**
- **Shader Input Structures**: Automatic generation of GPU-compatible data structures
- **Serialization Code**: Generate serialization/deserialization functions
- **Reflection Metadata**: Create runtime reflection information
- **Module Assembly**: Intelligent code organization and module generation

### ⚡ **Performance Optimizations**
- **Incremental Processing**: Smart caching system for faster rebuilds
- **Parallel Execution**: Multi-threaded parsing and code generation
- **Memory Efficient**: Optimized memory usage for large codebases
- **Cache Management**: Persistent cache for build acceleration

### 🔧 **Development Tools**
- **Statistics Reporting**: Detailed performance and processing metrics
- **Error Handling**: Comprehensive error reporting and validation
- **Integration**: Seamless integration with Omniforce Engine build system

## 🔧 Architecture

### **Core Components**

```
MetaTool
├── Parser (Clang AST Analysis)
├── ASTVisitor (Code Structure Extraction)
├── CodeGenerator (Output Generation)
├── CacheManager (Build Acceleration)
```

### **Processing Pipeline**

1. **🔍 Setup**: Initialize Clang indexes and parser arguments
2. **📖 Parse**: Multi-threaded AST traversal of source files
3. **💾 Cache**: Store parsed data for incremental builds
4. **🏗️ Generate**: Create optimized output code
5. **📦 Assemble**: Organize generated code into modules
6. **📊 Report**: Display processing statistics

## 📋 Requirements

### **System Requirements**
- **OS**: Windows 10/11 (64-bit)
- **Compiler**: MSVC (Visual Studio 2019 or newer)
- **Memory**: 4GB RAM minimum, 8GB recommended
- **Storage**: 1GB available space

### **Development Requirements**
- **LLVM/Clang**: Latest stable release with development headers
- **CMake**: 3.23 or newer
- **C++ Standard**: C++23 support
- **Build System**: Omniforce Engine build environment

### **Dependencies**
- **libclang**: LLVM's C language family frontend library
- **nlohmann/json**: JSON parsing and serialization
- **spdlog**: Logging library
- **Standard Library**: C++23 standard library features

## 🚀 Usage

### **Build Integration**

MetaTool is automatically integrated into the Omniforce Engine build system:

```bash
# Build with MetaTool enabled
cmake --build Build/ --config Release

# MetaTool runs automatically during build process
# Generated files appear in Omniforce/Shaders/
```

### **Standalone Execution**

```bash
# Run MetaTool directly
./Build/Release/MetaTool.exe

# With custom configuration
./MetaTool.exe --config custom_config.json
```

## ⚙️ Configuration

### **Parser Arguments**

MetaTool automatically configures Clang parser arguments based on the engine's build configuration:

```cpp
// Automatic include paths from engine
"-I${ENGINE_INCLUDE_DIRS}"

// Engine-specific defines
"-DOMNIFORCE_STATIC"
"-DMETATOOL"

// C++23 standard
"-std=c++23"
"-fsyntax-only"
"-emit-ast"
```

## 📊 Performance

### **Processing Statistics**

MetaTool provides detailed performance metrics:

```
Available statistics:
- Session Duration: 2.45s
- Targets Generated: 47
- Targets Skipped: 12
```

### **Optimization Features**

- **🔄 Incremental Processing**: Only re-process changed files
- **⚡ Parallel Parsing**: Multi-threaded AST traversal
- **💾 Smart Caching**: Persistent cache for build acceleration
- **🎯 Dependency Tracking**: Minimal recompilation based on changes

## 🔍 Code Analysis

### **AST Traversal**

MetaTool performs deep analysis of C++ source code:

```cpp
// Analyzes structures like this:
struct META(ShaderExpose, Module = "MyModule") ShaderShared {
    glm::vec4 position;
    glm::vec2 texcoord;
    glm::vec3 normal;
};

// Generates Slang version:
module MyModule;
namespace Omni {

  public struct ShaderInput_GPU {
    public float4 position;
    public float2 texcoord;
    public float3 normal;
};

}
```

### **Type Detection**

- **🔍 Struct Analysis**: Field types, sizes, and alignment
- **📊 Enum Processing**: Value extraction and validation
- **🔗 Template Instantiation**: Full template support
- **📝 Documentation**: Comment extraction and processing

## 📝 Generated Code

### **Shader Input Structures**

```cpp
// Generated from C++ structs
struct META(ShaderExpose, ShaderInput, "PathTracingInput") PathTracingInput {
    uint32 NumBounces;
    uint32 SamplerMode;
};

// Becomes Slang source code:
module PathTracingInput;
namespace Omni {

  public struct PathTracingInput {
    uint NumBounces;
    uint SamplerMode;
  };

  [[vk::push_constant]]
  public ConstantBuffer<PathTracingInput> Input;
}
```

## ⚡ Caching System

### **Cache Structure**

Cache stores already generated modules, parsed files, types and allows sped up code generation.
It stores fully parsed types - their definition, type and metadata.

### **Cache Benefits**

- **🚀 Faster Builds**: Skip unchanged files
- **💾 Memory Efficiency**: Load only necessary data
- **🔄 Incremental Updates**: Process only modified code
- **📊 Build Statistics**: Track processing performance

## 🎯 Use Cases

### **Engine Development**
- **🎨 Shader System**: Automatic generation of shader code that 1:1 maps to the C++ code
- **💾 Shader inputs**: Omniforce Engine utilizes constant buffers as shader inputs, and MetaTool is able to automatically generate such definitions.
- **🔍 Debugging**: Generate slang-code that may be used for debugging

### **Build System**
- **⚡ Acceleration**: Reduce build times through caching
- **🔄 Incremental**: Smart dependency tracking
- **📊 Monitoring**: Build performance analytics
- **🔧 Integration**: Seamless CMake integration

## 🚧 Limitations

### **Current Limitations**
- **🔧 Platform**: Windows-only (LLVM/Clang dependency)
- **📝 Language**: C++ only (no other languages supported)
- **🎯 Scope**: Engine-specific (not general-purpose)
- **💾 Memory**: Large codebases may require significant RAM

### **Future Improvements**
- **🌍 Cross-platform**: Linux and macOS support
- **🔧 General-purpose**: Framework for other projects
- **📝 Multi-language**: Support for additional languages
- **⚡ Performance**: Further optimization and caching improvements

## 🤝 Contributing

This tool is part of the **Omniforce Engine** project. For issues or improvements, please refer to the main project documentation or contact the project's author.

### **Development Guidelines**
- Follow the existing code style and conventions
- Ensure all code compiles without warnings
- Add appropriate documentation for new features
- Test thoroughly with various code structures

---

<div align="center">

**Made with ❤️ by Daniil Vinnik**

</div> 