# Progression [![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

## Description
A rewrite of a C++ game engine I have been developing for Linux and Windows. See [here](https://liamtyler.github.io/portfolio/Progression/) for more details.
Note: The older version of this engine can be found here: https://github.com/LiamTyler/Progression-Vulkan-Old. If you go way back in the commits, like to Dec 7th 2018 (80b7b20bfb3d3b499b4236cd65550474ec27f846) the old opengl tiled deferred renderer is there

## Prerequisites
### Operating Systems and Compilers
This engine requires at least C++ 20, and has only been tested on the following platforms and compilers:
- Windows 10 with MSVC 2019
- Ubuntu 22 with GCC 12 and Clang 15

### Libraries and SDKs
1. ISPC:
  You can find the download for any platform on the ISPC github releases [page](https://github.com/ispc/ispc/releases/).
It needs to be extracted and the ispc executable added to the PATH. If you are unfamiliar with that, directions are below.

    Windows: unzip the folder wherever you would like it installed. Then in the windows search bar type in "environment variables". Open that, then do to "edit environment variables". Click on the PATH variable (there are 2 PATHs, one for the local user and one for the any user on the computer. Use whichever one you want, it doesn't matter). You should see a list of folders that make up the PATH. Click "Add" and paste in the full path to the ispc bin folder (inside the unzipped result).

    Linux: extract the .tar.gz file with `tar -xvf [filename]`. Move that directory to wherever you would like. Now you can either add the bin directory to the PATH variable, or you can just make a symlink for the ispc executable into a directory that is already in the PATH. Ex: `sudo ln -s [path/to/bin/ispc] /usr/local/bin/ispc`

2. Vulkan 1.3: The download for this can be found [here](https://vulkan.lunarg.com/)

## Compiling Windows
From git bash:
```
git clone --recursive https://github.com/LiamTyler/Progression.git
cd Progression 
mkdir build
cd build
cmake -G "Visual Studio 16 2016" -A x64 ..
```
Then either open the Visual Studio .sln file and build there (ctrl-shift-B) or you can build same git bash command line with `cmake --build . --config [Debug/Release]`

## Compiling Linux
From the command line:
```
git clone --recursive https://github.com/LiamTyler/Progression.git
cd Progression 
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=[Debug/Release/Ship] ..
make -j
```
Note with building with gcc: gcc actually seems to timeout while doing a max parallel build of assimp with "make -j", so I do have to do "make -j6" instead
