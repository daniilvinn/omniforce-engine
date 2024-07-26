# Building the engine
## Requirements
To build the engine, Vulkan SDK, preferrably the newest version, is required to be installed on the machine. It can be downloaded on https://www.lunarg.com/vulkan-sdk/ page.
<br><br>
To run the engine, a GPU with support of Mesh Shader technology is required: NVIDIA Turing (and newer) or AMD RDNA2 (and newer) or any other GPU which supports Mesh Shaders.

## Generating projects
Go to `{root}/Scripts` directory and run `BuildProjects.bat`. After building, project files can be found in `{root}/build` directory.

## Installation
Go to `{root}/Scripts` directory and run `BuildAndInstall.bat`. It will lead to generating projects, building the engine and installing to `{root}/installation` directory, where Editor's executable file can be found. To launch the engine, simply run the `Editor.exe` file.