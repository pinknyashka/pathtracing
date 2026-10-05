# Milestone 2 plan: windowed real-time path tracer — white cube + rotating red frame

Date: 2026-10-05. Status: complete (2026-10-05, W1–W8 done; see AGENTS.md "Milestone 2"). Supersedes the M1 orbiting-camera smoke recipe.

## Goal

`pathtracer.exe` opens an **SDL2 window** and runs **real-time path tracing**: a **white cube**
at the origin with a 2×2 **red emissive square frame orbiting around it** (rotation about the
vertical Y axis through the cube, 12 s period). **Static camera** at `(0, 2, 9)` looking at the
origin (height 2, back off to z=9 so the 2×2 ring at z=+3 fits fully in frame at fov 50°). Temporal accumulation (exponential blend) smooths low-spp noise in real time.
PPM mode remains the no-SDL2 fallback and the deterministic smoke-check path.

Resulting app: a window showing a white cube lit by a glowing red square frame that sweeps
around it — frame passes behind the cube, around the sides, and in front (bright red ring
between camera and cube), continuously, with noise converging over a couple of seconds.

## Context (verified 2026-10-05)

- M1 complete (`cc58337`): AABB cube + static emissive frame + orbiting camera, NEE, CLI
  hardening, 7/7 ctest, PPM smoke pass. Baseline re-verified green this session (build no-op,
  7/7 ctest, smoke frame_0213: 2312 brightRed px, lit red-dominant cube front, black
  background boxes; origin `main` in sync with local).
- **SDL2 not installed** in any msys64 root; `pacman` unusable (not on PATH and the msys2
  mirrors `mirror.msys2.net`/`files.msys2.net` DNS-fail on this network). **GitHub is
  reachable** → SDL2 is built from source (see W1).
- `Box` is a strict AABB (`src/scene/geometry.h`) → a rotating frame needs a transformed box.
- NEE (`Scene::sampleLightPoint`, `Scene::lightNormal`, `frameMat` in
  `src/render/tracer.h:74`) is hardcoded to the static frame plane → must become
  time-dependent (W3).
- Performance model (measured): 320×320 @ 16 spp ≈ 30 ms/frame render → ≈18 ns per
  pixel-sample. 960×540 @ 4 spp ≈ 40 ms ≈ 25 fps. Window mode therefore defaults to
  **spp 4 + accumulation on** (W4); pixel count is the dominant cost, row-level OpenMP
  (`schedule(dynamic,16)`) is already adequate.

## Work items

### W1 — SDL2 from source (critical path)
- `git clone --depth 1 --branch SDL2 https://github.com/libsdl-org/SDL.git third_party/sdl2-src`
  (done). The standalone `libsdl-org/SDL2` repo is gone (API 404); the monorepo's releases
  are SDL3 3.4.x, which removed `SDL_Renderer` → incompatible with `src/render/display.h`,
  hence the `SDL2` branch (2.30.x series).
- Static build via local CMake/Ninja + ucrt64 gcc 15: script `third_party/build_sdl2.ps1`,
  prefix `third_party/sdl2`, only video/render/threads/timer/filesystem subsystems
  (audio, joystick, png, ttf, … OFF). Logs: `third_party/sdl2-{configure,build,install}.log`.
- Reconfigure main build with `-DSDL2_DIR=D:/Projects/pathtracing/third_party/sdl2/lib/cmake/SDL2`
  → `PATHTRACER_HAS_SDL`; final exe keeps `-static` (SDL2 is static → no DLL needed).
- Verify: `pathtracer.exe` (no `--out`) opens a window and closes cleanly (SDL_INIT_VIDEO
  succeeds on this machine).
- `third_party/` is gitignored; the re-clone command above is the rebuild recipe.

### W2 — transformed box (OBB): `src/math/mat3.h` (new), `src/scene/geometry.h`
- `Mat3`: 3×3 orthonormal matrix over `Vec3` — `mul(Vec3)`, `transpose()`, `static rotY(float)`.
- `Box` gains `Mat3 R = identity; Vec3 T = Vec3(0,0,0);` — identity ⇒ byte-identical behavior
  to M1, so all existing box tests stay valid without edits.
- `Box::intersect`: map the ray to local space (`o' = Rᵀ(o−T)`, `d' = Rᵀ d`), run the existing
  slab test. A rigid transform preserves the ray parameter `t` exactly (no scale), so hit `t`
  is unchanged; then `point = R·p_local + T`, `normal = R·n_local` (rotation keeps normals
  unit length).
- `@tester`: rotated-box cases (45°/90° about Y and Z), transformed normal checks, misses
  outside the rotated extent, identity-transform equivalence with the old code path.

### W3 — scene v2: white cube + rotating frame: `src/scene/scene.h`, `src/scene/camera.h`
- Cube albedo → `{0.9, 0.9, 0.9}` (white; renders red-tinted on the face lit by the frame).
- Frame bars keep their local mn/mx (ring at local `z = −3`); `Scene::setFrameAngle(θ)` sets
  `R = rotY(θ)` on all four bars (rotation about the world Y axis through the origin,
  `T = 0`). θ = 0 reproduces the M1 layout exactly (frame at `z = −3`).
- `lightNormal` (world) = `rotY(θ)·(0,0,1)`; `sampleLightPoint` samples the local ring and
  maps to world with `R`. `lightArea` is rotation-invariant (NEE weight unchanged).
- Camera: static `Camera((0, 2, 9), (0, 0.1, 0), (0,1,0), fovY 50°)`. `Camera::orbit` is
  kept (still unit-tested) but no longer used by `main`.
- Concurrency: the main thread calls `setFrameAngle` before the parallel render loop; the
  scene is read-only during a frame (same pattern as M1).

### W4 — CLI + main loop: `src/cli_args.h`, `src/main.cpp`
- `--time T` (T ≥ 0, validated like the other numeric opts): freeze the animation at wall
  time T → deterministic stills for smoke/tests (works for window and PPM modes). Default:
  wall clock since process start.
- Accumulation: **on by default in window mode**, off by default in PPM mode; `--accumulate`
  forces on, new `--no-accumulate` forces off.
- Window mode: if the user did not pass `--spp`, use 4 (≈25–30 fps at 960×540 here); PPM
  mode keeps the 8 default. `parseArgs` records whether `--spp` was given (`sppSet`).
- Per frame: `t = (--time ≥ 0) ? --time : wallClock`; `scene.setFrameAngle(2π·t/12)`.

### W5 — display: `src/render/display.h`
- Reuse the ARGB staging buffer across `present()` calls (allocated once per resolution);
  tone mapping unchanged (display-time only, per project rule).

### W6 — tests via `@tester`
- `test_box`: W2 rotation cases.
- New `test_scene`: for θ ∈ {0°, 90°, 180°} — every `sampleLightPoint` lies on the ring
  (`lightNormal·p ≈ −3` within the bar thickness) and `lightArea` is invariant;
  `lightNormal` ⊥ frame plane.
- `test_tracer` (re-anchored for the static camera): θ = π (frame at `z = +3`, between
  camera and cube) → the cube's +z (camera-facing) face point is lit, red-dominant, magnitude
  in the M1 acceptance class (lit.x ≈ 0.035–0.065); θ = 0 (frame behind the cube) → that
  face is dark (shadow side); primary ray into a frame bar at θ = π returns HDR red
  (≈ emission 4.0).
- `test_args`: `--time` parsing (rejects negative/non-numeric), `--no-accumulate`, `sppSet`.

### W7 — smoke / acceptance: `tests/smoke_stats.cpp` + recipe
New deterministic recipe (static camera, no timing race):

    pathtracer --width 320 --height 320 --spp 32 --frames 1 --time 6.0 --out smoke2
    smoke_stats smoke2/frame_0001.ppm

At `t = 6.0 s` the frame is at θ = 180° (ring at `z = +3`, between the camera at `z = 9` and
the cube). Projected at 320×320, fov 50°: the ring spans most of the frame (bars near the
edges, slightly trapezoid/tilted from the camera height), the cube (~37 px wide) sits inside
the ring hole. Exact pixel regions are measured from a rendered frame (ASCII dump) before
locking the `smoke_stats` boxes. Expect:
- red ring bright red: horizontal/vertical center bands → brightRed in the hundreds/thousands;
- white cube inside the ring hole: camera-facing face lit, red-dominant;
- all four background corners black.

`smoke_stats` regions are retuned for the static layout (W7). Window acceptance: run
`pathtracer.exe` (defaults) → a window shows the red ring orbiting the white cube, noise
settling within ~2–3 s (visual, user-confirmed on this machine).

### W8 — docs + status
- `AGENTS.md`: Milestone-2 section (goal, SDL2-from-source toolchain note, new smoke
  recipe), old M1 status kept as history.
- `README.md`: scene description (white cube, frame rotating about Y, static camera), new
  flags (`--time`, `--no-accumulate`, window-mode defaults), SDL2 local-prefix note.
- Commit the milestone; push when the user asks (token recipe in AGENTS.md).

## Risks / open questions
- **SDL2 static build runtime**: the win32 video driver must init on this machine; if a
  window can't open (display/session quirk), the app auto-falls back to PPM and the window
  acceptance is deferred to the user's own run.
- **Accumulation ghosting**: the 0.6/0.4 exponential blend smears the fast-moving ring into
  a short trail — accepted as an aesthetic (light-sweep) for v1; revisit only if it reads
  as a defect.
- **SDL2-branch CMake toggles**: all `-DSDL_*` options are verified against the configure
  log in W1 (unknown options are ignored by CMake, so the log is the proof).

## Execution order
W1 (running in background) → W2 → W3 → W4 → W5 → build → W6 (`@tester`) → `ctest` →
W7 smoke → window run → W8 docs → milestone commit.
