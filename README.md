# pathtracer

CPU path tracer: a cube lit by a glowing red square frame, with a camera orbiting the scene.
All rendering is computed on the CPU (no GPU). Frame rate trades off against `--spp`/resolution by design.

## Build

Requires CMake >= 3.20 and a C++20 compiler.

    cmake -B build
    cmake --build build -j
    ctest --test-dir build

Local toolchain (this machine, no system CMake):

    D:\Projects\__tools\cmake\bin\cmake -B build -G Ninja `
        -DCMAKE_MAKE_PROGRAM=D:/Projects/__tools/ninja/ninja.exe `
        -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe
    D:\Projects\__tools\cmake\bin\cmake --build build

SDL2 is optional. If `find_package(SDL2)` fails, frames are written to `frames/` as PPM files.
OpenMP is used for row-parallel sampling when available.

## Run

    build\pathtracer.exe                     # window (SDL2) or frames/ PPMs, orbits forever
    build\pathtracer.exe --spp 32 --width 1280 --height 720
    build\pathtracer.exe --frames 1 --out out --spp 64   # one still to out/frame_0001.ppm
    build\pathtracer.exe --accumulate        # temporal blend to smooth noise

Scene: unit cube (Lambertian) at the origin; 2x2 m red emissive frame (4 thin boxes) at `z = -3`;
camera orbits on a circle of radius 5 at height 2, one revolution every 12 s.
