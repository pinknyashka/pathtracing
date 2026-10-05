# pathtracer

CPU path tracer: a white cube lit by a glowing red square frame that rotates around it
(12 s period, about the vertical axis), seen from a static camera.
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

SDL2 is optional. If `find_package(SDL2)` fails, frames are written to `frames/` as PPM files
(and the window falls back to the same PPM output if it cannot open).
On this machine SDL2 is built from source into a local prefix:

    -DSDL2_DIR=D:/Projects/pathtracing/third_party/sdl2/lib/cmake/SDL2

(see `third_party/build_sdl2.ps1` for the rebuild recipe; `third_party/` is gitignored).
OpenMP is used for row-parallel sampling when available.

## Run

    build\pathtracer.exe                     # window (SDL2) or frames/ PPMs; real-time (spp 4, blend on)
    build\pathtracer.exe --spp 32 --width 1280 --height 720
    build\pathtracer.exe --frames 1 --out out --spp 64   # one still to out/frame_0001.ppm
    build\pathtracer.exe --time 6.0 --frames 1 --out smoke2   # deterministic still (frame at 180 deg)
    build\pathtracer.exe --no-accumulate     # disable the temporal blend (window mode blends by default)

Scene: unit white cube (Lambertian) at the origin; a 2x2 m red emissive frame (4 thin boxes,
`z = -3` locally) rotates about the Y axis, one revolution every 12 s; static camera at
`(0, 2, 9)` looking at the origin. Window mode defaults to `--spp 4` with temporal
accumulation on (explicit `--spp` overrides the default); PPM mode keeps the `--spp 8`
default and blends only with `--accumulate`.
