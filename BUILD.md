# Building the engine
## Requirements
* Vulkan SDK (tested on 1.3.280) installed on the machine (path to the Vulkan SDK must be added to the PATH system, in order for CMake to find the SDK). It can be downloaded from this page: https://www.lunarg.com/vulkan-sdk/
* CMake 3.23. It can be downloaded from https://cmake.org/download/.
<br><br>
To run the engine, **a GPU with support of Mesh Shader technology is required**: NVIDIA Turing (and newer) or AMD RDNA2 (and newer) or any other GPU which supports Mesh Shaders.
<br>

## Build instructions
1. Create desired directory for the engine and open a console within that directory
2. Run this command: `git clone --recursive  https://github.com/daniilvinn/omniforce-engine.git` and wait until cloning finishes
3. Go to the `Scripts` directory
4. If you want to generate IDE projects, run `GenerateProjects.bat`. After it finishes, projects can be found in `build` directory.
5. If you want to generate projects, build and install, run `BuildAndInstall.bat`. It can take some time and after it finishes, projects can be found in in `build` directory and the installed engine in the `installation` directory, respectively.

