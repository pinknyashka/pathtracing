# pathtracer

CPU path tracer: a white rough-plastic cube lit by a neon-glowing red square frame that
wobbles around it (12 s period, pitching about the horizontal axis through the cube center
— an equatorial ring), seen from a static camera.
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
    build\pathtracer.exe --time 0.0 --frames 1 --out smoke4   # deterministic still (ring face-on)
    build\pathtracer.exe --no-accumulate     # disable the temporal blend (window mode blends by default)

Scene: unit white cube (rough plastic — a white diffuse base with a soft GGX microfacet
sheen) at the origin; a 2x2 m neon-glowing red frame (4 thin emissive boxes, an equatorial
ring — its plane passes through the cube center, `z = 0` locally) pitches
about the horizontal X axis, one full wobble every 12 s (face-on at t = 0, edge-on at
t = 3 / 9 s, inverted at t = 6 s); static camera at `(0, 2, 9)` looking at the origin.
Materials (Milestone 4, `docs/plan-materials.md`): the cube is white rough plastic (diffuse
+ microfacet sheen) and the frame a neon glow (emission).
Window mode defaults to `--spp 4` with temporal accumulation on (explicit `--spp`
overrides the default); PPM mode keeps the `--spp 8` default and blends only with
`--accumulate`.
