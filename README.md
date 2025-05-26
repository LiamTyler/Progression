# Progression [![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

## Description
A C++ game engine I have been developing for Windows and Linux

## Features
- A fully bindless (textures, buffers, and materials) Vulkan 1.3 renderer
- Custom render graph solution. Handles all of the resource allocation, render passes, barriers, and also aliases resources as much as possible
- GPU frustum culling
- Meshlet pipeline rendered using mesh shaders
- A fairly extensive asset pipeline. It has a lot of features, including texture composition to reduce the number of fetches in shader, bc compression, meshlet generation, skipping rebuilds of assets that haven't changed at all, tightly packed serialization into binary packages for optimized loads, and more. For more info on that, see [here](https://liamtyler.github.io/posts/asset_pipeline_part_1/)
- Optimized asset loads that batch gpu uploads in a double buffered queue on the transfer queue
- CPU profiling using [Tracy](https://github.com/wolfpld/tracy)
- GPU profiling using timestamps, automatically added for each render pass
- An offline path tracer to help provide reference images. Very limited material support currently, still work in progress
- Multiple build configurations to give various levels of optimization and debugging capability: Debug, Release, Profile, Ship
- Lua scripting system for game logic
- Entity component system
- In game debug console to enter developer commands
- A lot more :)

## Prerequisites
### Operating Systems and Compilers
This engine requires at least C++ 20, and has only been tested on the following platforms and compilers:
- Windows 10 or 11 with MSVC 2022
- Ubuntu 24 with GCC 13

### Libraries and SDKs
1. ISPC:
  You can find the download for any platform on the ISPC github releases [page](https://github.com/ispc/ispc/releases/).
It needs to be extracted and the ispc executable added to the PATH. If you are unfamiliar with that, directions are below.

    Windows: unzip the folder wherever you would like it installed. Then in the windows search bar type in "environment variables". Open that, then do to "edit environment variables". Click on the PATH variable (there are 2 PATHs, one for the local user and one for the any user on the computer. Use whichever one you want, it doesn't matter). You should see a list of folders that make up the PATH. Click "Add" and paste in the full path to the ispc bin folder (inside the unzipped result).

    Linux: you can either use `sudo snap install ispc` or manually download and then extract the .tar.gz file with `tar -xvf [filename]`. Move that directory to wherever you would like. Now you can either add the bin directory to the PATH variable, or you can just make a symlink for the ispc executable into a directory that is already in the PATH. Ex: `sudo ln -s [path/to/bin/ispc] /usr/local/bin/ispc`

2. Vulkan 1.3: The download for this can be found [here](https://vulkan.lunarg.com/)

3. bzip2 (linux only): `sudo apt-get install libbz2-dev`

## Compiling

### Windows
From git bash:
```
git clone --recursive https://github.com/LiamTyler/Progression.git
cd Progression 
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
```
Then either open the Visual Studio .sln file and build there (ctrl-shift-B) or you can build same git bash command line with `cmake --build . --config [Debug/Release/Profile/Ship]`

### Linux
Don't forget to install the dependencies mentioned above.

From the command line:
```
git clone --recursive https://github.com/LiamTyler/Progression.git
cd Progression 
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=[Debug/Release/Profile/Ship] ..
make -j
```

## Usage
At this point all the executables should be in the build/bin folder. Before you can run a scene, the converter first has to process. For example, processing the `sponza` scene would be done like this:
```
./build/bin/Converter.exe sponza
```

To actually run a scene once it's been converted, just pass the scene name as the first arg to the engine:
```
./build/bin/Engine_r.exe sponza
```
