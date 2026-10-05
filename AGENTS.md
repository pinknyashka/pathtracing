# AGENTS.md

- Project: CPU path tracer in C++20, built with CMake. Scene: axis-aligned cube (Lambertian) at origin + thin square frame (4 thin boxes, ~2×2 ring) at `z ≈ -3` emitting red (HDR); camera orbits the scene over time.
- Everything is computed on CPU by design — no GPU shaders/hardware acceleration. Frame-rate drops at high `--spp`/`--res` are expected, not bugs.
- SDL2 is an optional dependency: if `find_package(SDL2)` fails, the renderer must fall back to writing PPM frames to `frames/`. Don't assume a window will open.
- Build: out-of-source `build/` (never commit it).
  - Configure: `cmake -B build`
  - Build: `cmake --build build -j`
  - Test: `ctest --test-dir build` (single test: `ctest --test-dir build -R <name>`)
- Layout: `src/math` (Vec3/Ray), `src/scene` (geometry, materials, camera), `src/render` (tracer, display), `tests/`, `src/main.cpp` (CLI + animation loop).
- Radiance is HDR through the tracer (emission > 1.0); tone-map/clamp only at display or PPM write time.
- Tracer correctness relies on next-event estimation sampling the frame directly — a thin frame is rarely hit by random bounces; don't remove NEE.
- Before declaring the renderer correct, run the full ctest suite plus a PPM smoke check: frame pixels are bright red, the cube face facing the frame is lit, the far side is in shadow.
- Until the first `CMakeLists.txt` and test suite land, verify assumptions in code rather than relying on the commands above.
- Keep this file updated as the build system, compiler flags, and test setup are finalized.
