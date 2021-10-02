# Progression [![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

## Build status
- Linux build: [![Build Status](https://travis-ci.org/LiamTyler/Progression.svg?branch=master)](https://travis-ci.org/LiamTyler/Progression)
#- Windows build: [![Build status](https://ci.appveyor.com/api/projects/status/1q6bt9qdmnm3p866?svg=true)](https://ci.appveyor.com/project/LiamTyler/progression)

## Description
A C++ game engine I have been developing for Linux and Windows. See [here](https://liamtyler.github.io/portfolio/Progression/) for more details.

## Installing + Building
This engine requires C++ 2020, and has only been tested on MSVC 2019 and GCC 9

First install ispc(https://github.com/ispc/ispc/) and make sure it's in your PATH
```
git clone --recursive https://github.com/LiamTyler/Progression.git
cd Progression 
mkdir build
cd build
(linux) cmake -DCMAKE_BUILD_TYPE=[Debug/Release/Ship] ..
(windows) cmake -G "Visual Studio 16 2016" -A x64 ..
```

Note with building with gcc: gcc actually seems to timeout while doing a max parallel build of assimp with "make -j", so I do have to do "make -j6" instead
