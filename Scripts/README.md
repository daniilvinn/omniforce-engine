# Omniforce Engine Scripts

This directory contains setup and build scripts for the Omniforce Engine project.

## Setup for Cursor/VS Code with clangd

The Omniforce Engine uses MSBuild generator for CMake to support C# project generation, but this doesn't support `CMAKE_EXPORT_COMPILE_COMMANDS` which is required for clangd (used by Cursor and VS Code).

### Quick Setup

1. **Run the enhanced setup script** (recommended):
   ```bash
   python Scripts/Setup.py
   ```
   This will install all dependencies including Ninja and generate `compile_commands.json`.

2. **Or use the project generator**:
   ```bash
   Scripts/GenerateProjects.bat
   ```
   Choose option 2 or 3 to generate `compile_commands.json`.

3. **Or run the dedicated script**:
   ```bash
   Scripts/GenerateCompileCommands.bat
   ```

### Manual Setup

If you prefer to set up manually:

1. **Install Ninja** (required for compile_commands.json generation):
   - Download from: https://github.com/ninja-build/ninja/releases
   - Or install via package manager: `choco install ninja`

2. **Generate compile_commands.json**:
   ```bash
   mkdir Build
   cd Build
   mkdir BuildClangd
   cd BuildClangd
   cmake -G "Ninja" ..\..
   copy compile_commands.json ..\..\compile_commands.json
   ```

### How it Works

- **MSBuild projects** (in `Build/` directory): Used for normal development and C# project generation
- **Ninja projects** (in `Build/BuildClangd/` directory): Used only to generate `compile_commands.json` for clangd
- **Single CMakeLists.txt**: Automatically detects the generator and includes/excludes C# projects accordingly
- **compile_commands.json**: Copied to project root for clangd/Cursor to find

### Troubleshooting

- **clangd not working**: Ensure `compile_commands.json` exists in the project root
- **CMake errors**: Make sure all dependencies are installed (Vulkan SDK, CMake, Clang, Ninja)
- **Path issues**: Restart your terminal/IDE after running setup scripts
- **C# project errors**: The Ninja build automatically excludes ScriptEngine - use MSBuild for C# development

### Files Generated

- `Build/`: MSBuild solution and project files
- `Build/BuildClangd/`: Ninja build files (for compile_commands.json generation)
- `compile_commands.json`: Clangd configuration file (in project root)
- `Install/`: Installed binaries and resources 