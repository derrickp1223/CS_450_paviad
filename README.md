# CS 450 (Computer Graphics)
***Spring 2025***  
***Author: Derrick Pavia***  
***Original Author: Dr. Michael J. Reale***  
***SUNY Polytechnic Institute*** 

## Software Dependencies
You will need to install the following manually:
- C++ compilers
- CMake
- Visual Code
- Vulkan SDK

The rest of the dependencies will be automatically fetched by CMake.

### C++ Compilers
* ***For Windows:*** Download and install ONE of the following:
  * [Visual Studio 2022](https://visualstudio.microsoft.com/) (and install the C++ toolchain) ***(STRONGLY recommended if you are taking another class with me)***
  * [Build Tools for Visual Studio 2022](https://aka.ms/vs/17/release/vs_BuildTools.exe
  ) 
* ***For Linux:*** GNU g++ compilers should work.
* ***For Mac:*** The included Clang g++ compilers should be fine.

### CMake
You will need **version 3.24** or higher.

* ***For Windows:***
  * Download the CMake installer from [here](https://cmake.org/download/).
  * If given the option, make sure to **"Add CMake to the system PATH for all users"**.
* ***For Mac:***
  * Download the latest "macOS 10.13 or later" .dmg from [here](https://cmake.org/download/)
  * Install Cmake and run it
  * Select ```"Tools" -> "How to Install for Command Line Use"```
  * Copy the second option into a terminal: ```sudo "/Applications/CMake.app/Contents/bin/cmake-gui" --install```


### Visual Code
I strongly recommend Visual Code as your IDE.
* [Download](https://code.visualstudio.com/) and install Visual Code
  * I suggest enabling the context menu options.
  * ***For Mac:*** Follow [these instructions](https://code.visualstudio.com/docs/setup/mac)
* Install the following extensions:
  * **"C/C++ Extension Pack"** by Microsoft
  * **"Shader languages support for VS Code"** by slevesque
  * **"Shaderc GLSL Linter"** by Dave Frampton
  * **"Git Graph"** by mhutchie
  * **"Markdown All in One"** by Yu Zhang

### Vulkan SDK

Download and install the **Vulkan SDK** for your platform:

* ***For Windows and MacOS:*** Download the installer [here](https://vulkan.lunarg.com/sdk/home) and run it.
* ***For Linux:*** Follow instructions found [here](https://vulkan.lunarg.com/doc/sdk/latest/linux/getting_started.html).

***For MacOS specifically:*** After running the installer:
1. Open a terminal
2. cd to ```/Users/\<username\>/VulkanSDK/\<version number\>```
3. If python is not installed: ```brew install python```
4. Either:
```
sudo ./install_vulkan.py
```
or
```
sudo python3 install_vulkan.py
```

### Other Libraries
The following libraries will be automatically fetched into your project and compiled with your code:
- GLFW
- GLM
- Assimp
- vk-bootstrap

***For Linux:*** Install the following:
```
sudo apt install libwayland-dev libxkbcommon-dev xorg-dev gdb
```