# Building the engine
## Using the release
The simplest way to launch and test the engine is to simply download the latest release on GitHub and follow the guidelines.
<br>
However, if the newest version of engine is required, follow the steps below.

## Requirements
* Vulkan SDK (tested on 1.3.280) installed on the machine (path to the Vulkan SDK must be added to the PATH system, in order for CMake to find the SDK). It can be downloaded from this page: https://www.lunarg.com/vulkan-sdk/
* CMake 3.23. It can be downloaded from https://cmake.org/download/.
* Windows 10/11. The engine currently works only on Windows.
* Python 3.13.1
<br><br>
To run the engine, **a GPU with support of mesh shader technology is required**: NVIDIA Turing (and newer) or AMD RDNA2 (and newer) or any other GPU which supports Mesh Shaders.
<br>

## Build instructions
1. Create desired directory for the engine and open a console within that directory
2. Run this command: `git clone --recursive  https://github.com/daniilvinn/omniforce-engine.git` and wait until cloning finishes
3. Go to the `Scripts` directory
4. Run the `Setup.bat` file that will check if Vulkan SDK and CMake are present on the machine. If they are not, it will automatically install them and launch project files generation.
4. If you want to generate IDE projects, run `GenerateProjects.bat`. After it finishes, projects can be found in `Build` directory.
5. If you want to generate projects, build and install, run `BuildAndInstall.bat`. It can take some time and after it finishes, projects can be found in in `Build` directory and the installed engine in the `Install` directory, respectively.

