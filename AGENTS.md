# AGENTS.md

- Project: CPU path tracer in C++20, built with CMake. Scene: white rough-plastic unit cube (white diffuse + GGX microfacet sheen) at the origin + a thin 2×2 neon-glowing red square frame (emission; 4 transformed boxes, ring plane at local `z = 0`, i.e. centered on the cube center — an equatorial ring) pitching about the world X axis (12 s period); static camera at `(0, 2, 9)` looking at the origin. Frame bars are OBBs: `Box` carries a rigid transform (`Mat3 R`, `Vec3 T`); `Scene::setFrameAngle(θ)` drives the pitch (`R = Mat3::rotX(θ)`). The material model (rough-plastic cube, neon frame) landed in Milestone 4 — `docs/plan-materials.md`.
- Everything is computed on CPU by design — no GPU shaders/hardware acceleration. Frame-rate drops at high `--spp`/`--res` are expected, not bugs.
- SDL2 is an optional dependency: if `find_package(SDL2)` fails, the renderer must fall back to writing PPM frames to `frames/`. Don't assume a window will open.
- Build: out-of-source `build/` (never commit it).
  - Configure: `cmake -B build` (this machine: use the README "Local toolchain" line — local CMake/Ninja + `C:/msys64/ucrt64/bin/g++.exe`)
  - Build: `cmake --build build -j`
  - Test: `ctest --test-dir build` (single test: `ctest --test-dir build -R <name>`)
- Layout: `src/math` (Vec3/Ray), `src/scene` (geometry, materials, camera), `src/render` (tracer, display), `tests/`, `src/main.cpp` (CLI + animation loop).
- Radiance is HDR through the tracer (emission > 1.0); tone-map/clamp only at display or PPM write time.
- Tracer correctness relies on next-event estimation sampling the frame directly — a thin frame is rarely hit by random bounces; don't remove NEE.
- Before declaring the renderer correct, run the full ctest suite plus a PPM smoke check. Deterministic recipe (static camera, no timing race): `pathtracer --width 320 --height 320 --spp 32 --frames 1 --time 0.0 --out smoke4` then `smoke_stats smoke4/frame_0001.ppm` — at t=0 s the equatorial ring is face-on (θ=0, ring plane at z=0 through the cube center): expect bright-red ring top/bottom bands + side bars (≈832 brightRed px), the cube's **top** face lit and red-dominant (avgR≈9.6; the M4 rough-plastic material suppresses the diffuse term by (1−F_v) at the grazing view and adds a soft red sheen; the front face is unlit by the equatorial ring), and all four background corner boxes black (0.00).
- Keep this file updated as the build system, compiler flags, and test setup are finalized.
- Todo discipline: maintain a session todo list and update it every time a subgoal is finished — mark a task `in_progress` before starting it and `completed` as soon as it is verified. Never batch status updates at the end of a session.

# Task Distribution Matrix (Routing Contract)

You are the lead coordinating agent (Plan/Build mode) powered by Qwen. Your task is to decompose complex requirements and assign highly specialized sub-tasks to sub-agents.

## Available sub-agents:
1. `@tester` — Use this agent ALWAYS when the main code is written and requires test coverage or regression testing.
2. `@scout` — Built-in agent. Call this agent if you need to analyze external documentation or cached dependencies.

## Invocation instructions:
If the task is complex:
1. Create a high-level plan.
2. Delegate test writing to the sub-agent by sending a command in this format: `call @tester to generate tests for src/auth.py`.
3. If you lack information about external libraries, frameworks, or existing codebase architecture, delegate the research to @scout before generating code.
4. Wait for the sub-agent's response, analyze the result, and conclude the session.

## Status (as of 2026-10-06, after Milestone 4: materials — white rough-plastic cube + neon-glow frame)

- Repo at https://github.com/pinknyashka/pathtracing (public); local `origin` = that URL. Remote `main` is in sync with local (through the Milestone-4 commit; the Milestone-3 commit was `8bc8759`).
  - Push: a plain `git push origin main` works — Windows Credential Manager holds a GitHub credential for `pinknyashka` with push scope (pushed `f8d5c72..6b7d5a8` on 2026-10-05 and `1861242..8bc8759` on 2026-10-06 without an explicit token). Fallback if that credential expires: push with a token that has `public_repo`/`repo` scope (a read-only token gets 403): `git -c credential.helper= push https://x-access-token:<TOKEN>@github.com/pinknyashka/pathtracing.git main` — the `credential.helper=` override stops the token being saved to Windows Credential Manager.
- Toolchain blocker RESOLVED offline (no msys2 package changes): `D:\Projects\__tools\mingw_7_2_0` on the machine PATH shadowed ucrt64's runtime DLLs, so gcc-15 frontends crashed at startup with `0xC00000FD`. Fix: User PATH now starts with `C:\msys64\ucrt64\bin`, plus 8 DLLs staged next to cc1/cc1plus. Full report + prevention runbook: `docs/toolchain-incident-2026-10-05.md`.
- NEE self-blocking fixed in `src/render/tracer.h`: the visibility test now skips emissive boxes (it used to block on the sampled bar's own front face), and the emission contribution uses `cosEmit/d²`. Verified with a 1024-spp probe: lit.x=0.0503, shadow=0.0, framePix=(4.0, 0.18, 0.12); `test_tracer` expects lit.x in 0.035–0.065.
- CLI parser fixed: `parseArgs` called its `value()` lambda twice (each call bumps the arg index, so every option value was skipped — `--width 320` parsed as 0). Parser moved to `src/cli_args.h`, `--accumulate` branch restored, covered by `tests/test_args.cpp`.
- CLI strictness fixed: `parseArgs` now rejects non-numeric/overflow values for `--width/--height/--spp/--frames` and negative `--seed` (previously `atoi` silently mapped junk to 0, e.g. `--frames abc` ran forever in PPM mode). `Display::writePPM` returns bool; `main` exits 1 with a diagnostic if a frame can't be written. Covered by extended `test_args` + new `test_display` (toneMap checks + PPM write/read round trip).
- ctest 9/9 pass: `test_vec3`, `test_box`, `test_camera`, `test_light`, `test_scene`, `test_material`, `test_tracer`, `test_args`, `test_display`.
- PPM smoke check PASS (320x320, 16 spp, camera behind the frame at t≈9.6s, analyzed with `build/smoke_stats.exe`): 2298 bright-red frame px; the cube face facing the frame is lit and red-dominant (maxR≈13 = tone-mapped lit.x≈0.05; ~65% of face px lit, the rest in bar shadow); top face + background black. Re-verified after the CLI/display changes: frames at t_wall 9.3–9.8s show 2600–3700 bright-red px, lit red-dominant cube front, black top face + background boxes. (Note: `smoke_stats` regions are tuned for t≈9.3–9.8s; earlier in the behind-frame window, e.g. t≈9.0s, a frame bar can project into the lower-left "background" region — that is the bar itself, not a lighting leak.)
- All work pushed to GitHub `main` (through the Milestone-4 commit); local and remote in sync. Nothing outstanding.
- **Milestone 4 (complete, 2026-10-06): materials** — the 2-variant `Diffuse`/`Emissive` material
  is now a unified struct (diffuse `albedo` + GGX specular `f0`/`roughness` + `emission`) with
  `plastic`/`neon` presets. The **cube = white rough plastic** (`plastic({0.9,0.9,0.9},{0.04,0.04,0.04},0.4)`),
  the **frame = neon glow** (`neon({4.0,0.18,0.12})`, value unchanged). The tracer uses a **combined
  BRDF** `F·brdf_ggx + (1−F)·brdf_lambert` in NEE (so both the diffuse lighting and the specular sheen
  are lit by the ring in one low-variance shot) and **transmissive** (GGX / cosine) bounce sampling with
  `scale *= brdf_sampled/pdf_sampled`. SHADOW stays exactly 0, FRAME stays exact emission. Full plan +
  results + the one real fix found along the way: `docs/plan-materials.md`, `src/scene/material.h`,
  `src/render/tracer.h`.

## Milestone 2 (complete, 2026-10-05): windowed real-time — white cube + rotating red frame

- Goal: `pathtracer.exe` opens an SDL2 window and renders in real time: a **white cube** at the origin with a 2×2 **red emissive frame rotating about the Y axis** (12 s period) around it; **static camera** at `(0, 2, 9)`; temporal accumulation ON by default in window mode (spp defaults to 4 there). PPM mode stays as the no-SDL2 fallback and the deterministic smoke path.
- Full plan with work items W1–W8: `docs/plan-rotating-frame-window.md`.
- SDL2 is NOT installable via msys2 on this network (mirrors DNS-fail, `pacman` unusable) → built from GitHub: `git clone --depth 1 --branch SDL2 https://github.com/libsdl-org/SDL.git third_party/sdl2-src` (standalone SDL2 repo is gone; monorepo releases are SDL3 which dropped `SDL_Renderer`), then `third_party/build_sdl2.ps1` (static, video-only) → prefix `third_party/sdl2`. Reconfigure with `-DSDL2_DIR=D:/Projects/pathtracing/third_party/sdl2/lib/cmake/SDL2`. `third_party/` is gitignored.
- Geometry: `Box` now carries a rigid transform (`Mat3 R`, `Vec3 T`, `src/math/mat3.h`) — rotate ray into local space (rigid ⇒ t preserved), transform point/normal back. Frame bars get `R = rotY(θ)` per frame via `Scene::setFrameAngle(θ)`; `lightNormal`/`sampleLightPoint` track the rotation; NEE weight (`lightArea`) is rotation-invariant.
- CLI: new `--time T` (freeze animation at wall time T, for deterministic stills; T ≥ 0) and `--no-accumulate` (window mode blends by default).
- Smoke recipe v2 (replaces the M1 orbiting-camera recipe — static camera now, no timing race):
  `pathtracer --width 320 --height 320 --spp 32 --frames 1 --time 6.0 --out smoke2` then `smoke_stats smoke2/frame_0001.ppm` — at t=6 s the frame is at θ=180° (ring at z=+3 between camera and cube): expect bright-red ring bands, lit red-dominant cube inside the ring hole, all four background corners black. `smoke_stats` regions are retuned for the static layout.
- Window acceptance: run `pathtracer.exe` (no `--out`) → SDL window, red ring orbiting the white cube, noise settling in ~2–3 s (visual, user-confirmed). Verified 2026-10-05 on this machine: `pathtracer.exe --frames 2` opened the SDL window (no PPM fallback), 960×540 @ 4 spp ≈ 66 ms first frame (init), 25.6 ms steady (≈39 fps), clean exit rc=0.
- SDL2 local prefix was built (W1 finished): `third_party/sdl2` (static, video-only; logs `third_party/sdl2-{configure,build,install}.log`). CMake links `SDL2::SDL2` + the prefix include dir explicitly (the static install config exports no include dirs) + `SDL_MAIN_HANDLED` (our `main` calls `SDL_Init` itself, so libSDL2main.a is not linked) + win32 libs (`winmm imm32 ole32 ...`).
- ctest 8/8: `test_vec3`, `test_box` (OBB rotation cases: Y-45/Y-90, Z-90, T+R, identity equivalence), `test_camera`, `test_light`, `test_scene` (new: ring sampling on the rotated plane for θ ∈ {0°,90°,180°}, lightArea/lightNormal invariance), `test_tracer` (re-anchored to the static camera: LIT θ=π center-pixel lit.x≈0.060 ∈ [0.05,0.07] red-dominant; SHADOW θ=0 exactly 0.0; FRAME direct ray = exact emission (4.0, 0.18, 0.12)), `test_args` (`--time`, `--no-accumulate`, `sppSet`), `test_display`.
- Smoke v2 PASS (320×320 @ 32 spp, `--time 6.0`, seed-robust): 1740 bright-red px; ring top/bottom bands avgR≈204, side bars ≈125–130; cube front avgR=14.3/avgG=1.0 (all 960 region px lit, red-dominant); all four background corners exactly black. `smoke_stats` regions retuned for the static layout (bands y≈145–149/246–250, bars x≈103–108/210–216).

## Milestone 3 (complete, 2026-10-06): equatorial ring — frame centered on the cube, pitching about X

- Goal: the 2×2 red emissive frame becomes an **equatorial ring** — its plane passes through
  the **cube center** (`kFrameZ: -3.0 → 0.0`, ring center == cube center == pitch pivot at the
  origin) and it **pitches about the world X axis** (`setFrameAngle` now sets
  `R = Mat3::rotX(θ)`, new `Mat3::rotX`), 12 s period. With the static camera at `(0, 2, 9)`
  the red square hoop wobbles up and down around the white cube (face-on to camera at t = 0 /
  t = 12 s, vertical at t = 3 / 9 s, inverted at t = 6 s). Full plan: `docs/plan-equatorial-pitch.md`.
- NEE needed no formula change: `lightNormal`/`sampleLightPoint` already track `frameR`;
  `lightArea` is rotation-invariant; with `kFrameZ = 0` the ring is centered on the pivot so
  the plane test stays rotation-invariant.
- Lighting consequence at θ = 0 (face-on): the camera-facing +z face is **unlit** (every ring
  point has z < 0.5 ⇒ cosL < 0; cosine bounces stay in the +z hemisphere and can't reach the
  ring) → the center pixel is exactly 0; the **top face** is lit red-dominant by the top bar
  (its front half, z > 0.03).
- Tests re-anchored: `test_scene` `lightNormal` = `rotX(θ)·(0,0,1) = (0,−sinθ,cosθ)`;
  `test_tracer` LIT = top-face pixel (s=0.5, t=0.54003608), lit.x ∈ [0.06,0.14] + exact
  channel ratios y/x=0.045, z/x=0.03 (close-range NEE estimator is heavy-tailed, ~17 % sd);
  SHADOW = center pixel exactly 0.0; FRAME = ray to top-bar front face `(0,0.97,+0.03)` →
  exact emission (4.0,0.18,0.12).
- ctest 8/8 pass. Smoke v3 PASS (320×320 @ 32 spp, `--time 0.0`, seed-robust): 828
  bright-red px; ring top/bottom bands avgR≈193–195 (all region px lit, maxR 204 = clamped
  HDR); side bars avgR≈171–172; **cube top** avgR=15.4/avgG=0.8 (all 93 region px lit,
  red-dominant); all four background corners exactly black. `smoke_stats` regions retuned
  for the face-on layout (bands y≈126–128/197–199, bars x≈122–124/195–197, cube top
  y≈146–148). `smoke3/` added to `.gitignore`.
- Window acceptance: `pathtracer.exe --frames 2` (no `--out`) opened the SDL window (no PPM
  fallback), 960×540 @ 4 spp ≈ 35 ms first frame, 23 ms second (≈43 fps), clean exit rc=0
  (verified 2026-10-06; the wobble itself is visual). Milestone committed (`8bc8759`) and
  pushed to GitHub `main` 2026-10-06; local and remote in sync.

## Milestone 4 (complete, 2026-10-06): materials — white rough-plastic cube + neon-glow frame

- Goal: replace the 2-variant `Diffuse`/`Emissive` `Material` enum with a **unified material**
  (diffuse `albedo` + GGX specular `f0`/`roughness` + `emission`) so the two objects get real,
  named materials: the **cube = white rough plastic** (white diffuse base + a soft GGX
  microfacet sheen, `plastic({0.9,0.9,0.9}, {0.04,0.04,0.04}, 0.4)`) and the **frame = neon
  glow** (pure emission, `neon({4.0,0.18,0.12})` — value unchanged from M3). Full plan, BRDF /
  NEE design, and work items W1–W8: `docs/plan-materials.md`.
- Tracer (`src/render/tracer.h`): free-function GGX helpers (`schlickF`, `ggxD`, `ggxG1`,
  `combinedBrdf = F_v·brdf_ggx + (1−F_v)·brdf_lambert`); the NEE term uses the **combined BRDF**
  (the scene's only ring→surface light path — a bounce-only specular lobe would be invisible);
  `scale *= albedo` + `cosineBounce` becomes transmissive `sampleScatter` (lobe choice ∝ F_v vs
  (1−F_v), GGX importance sampling with reject-retry for into-surface reflections, cosine
  fallback) with `scale *= brdf_sampled / pdf_sampled`; the sampler's pdf is the true one,
  `p_wi = D(h·n) / (4·wo·h)` (no G1 — the sampler does not mask); emissive-hit rule unchanged
  (bounce 0 → emission, bounce > 0 → nothing).
- **Real fix found: NDF normalization.** The raw Trowbridge-Reitz/GGX formula
  `α²/(π((1−α²)c²+α²)²)` is **not** normalized over the hemisphere (its integral is
  `Z = 1 + atan(√((1−α²)/α²))/√(α²(1−α²))` ≈ 4.17 at roughness 0.4, →∞ as α→0; the "÷4" pdf
  trick used by BRDF-ratio renderers papers over this, but NEE uses the BRDF as an absolute
  value). `ggxD` now divides by Z. Bounce sampling is insensitive (the D cancels in the
  brdf/pdf ratio; the sampling CDF shape is unchanged) — only the NEE sheen term was biased
  (×Z too strong) before the fix.
- Also: F_v is exactly zero for non-specular materials, so a pure-diffuse surface keeps its
  full `albedo/π` weight even at grazing view angles (Schlick with f0=0 would otherwise grow
  to 1 at grazing).
- Tests: new `tests/test_material.cpp` (GGX NDF integrates to 1 by fixed-seed Monte-Carlo +
  shape/endpoint checks, Fresnel-Schlick endpoints + energy split, G1 masking, combined-BRDF
  diffuse-exact / specular-reference / split-linearity / neon-zero probes, preset fields) +
  CMake `material` target; ctest 9/9. `test_tracer` **LIT** re-anchored to `lit.x ∈ [0.03,0.08]`
  (probe: 64-spp 32 seeds [0.029,0.077] mean 0.053; 512-spp 8-seed mean 0.0486 — the top face
  is viewed near-grazing, so (1−F_v) ≈ 0.1–0.2 suppresses most of the diffuse and the rest is
  the NEE-sampled red sheen). **The plan's expectation that the exact channel-ratio asserts
  break did not materialize**: f0 and albedo are gray, so every term is still emission × a
  gray scalar (the "white sheen" reflects the red ring) and `y/x = 0.045`, `z/x = 0.03` still
  hold exactly — the exact asserts were kept (stronger than red-dominant). **SHADOW** stays
  exactly 0.0 (both lobes emit into the +z hemisphere; even tunneling rays exit through
  unlit faces); **FRAME** stays exact emission `(4.0,0.18,0.12)`. `test_scene`/`test_light`
  unaffected.
- Smoke v4 PASS (320×320 @ 32 spp, `--time 0.0`, seed-robust): 832 bright-red px (M3: 828);
  ring top/bottom bands avgR≈195.0/192.8 (M3 ≈193–195); side bars avgR≈171.4/172.1 (M3
  ≈171–172) — the neon frame is pixel-identical. **Cube top** avgR=9.55/avgG=0.39 (M3:
  15.4/0.8 — the (1−F_v) diffuse suppression at the grazing view), red-dominant, 89/93 region
  px lit, maxR=19 (a soft sheen bump above the M3 level); all four background corners exactly
  black. `smoke_stats` region boxes unchanged (geometry unchanged); only the expected numbers
  moved. `smoke4/` gitignored.
- Window acceptance: `pathtracer.exe --frames 2` (no `--out`) opened the SDL window (no PPM
  fallback), 960×540 @ 4 spp ≈ 36.5 ms first frame, 28.0 ms second, clean exit rc=0 (verified
  2026-10-06; the satin sheen vs flat-matte difference is visual, user-confirmed).
- Design decisions (plan's "Decisions") all implemented as resolved: full GGX + combined-BRDF
  NEE, pure neon emission (no glass-tube shell), Schlick `(1−F)` on the diffuse split.
  Milestone committed and pushed to GitHub `main` 2026-10-06; local and remote in sync.

## Toolchain notes (this machine)

- Compiler: msys2 ucrt64 GCC 15.2.0 at `C:\msys64\ucrt64\bin\g++.exe`; CMake/Ninja are local at `D:\Projects\__tools\{cmake,ninja}` (configure line in README).
- **PATH pitfall**: the machine PATH contains `D:\Projects\__tools\mingw_7_2_0\mingw64\bin` (2017-era libgmp/libmpfr/libmpc/libwinpthread + binutils). If `C:\msys64\ucrt64\bin` is not ahead of it on PATH, cc1/cc1plus load the 2017 DLLs and crash at startup with `0xC00000FD`, no diagnostics. `C:\msys64\ucrt64\bin` is first in the **User** PATH (set 2026-10-05).
- **Stale-session gotcha**: processes started before a PATH change (including this CLI) keep the old PATH. In shell commands, re-derive it first: `$u=[Environment]::GetEnvironmentVariable("Path","User"); $m=[Environment]::GetEnvironmentVariable("Path","Machine"); $env:PATH="$u;$m"`.
- Backstop: the frontends' own directory (`C:\msys64\ucrt64\lib\gcc\x86_64-w64-mingw32\15.2.0\`) holds staged copies of libwinpthread-1, libgcc_s_seh-1, libgmp-10, libmpfr-6, libmpc-3, libisl-23, zlib1, libzstd. After any msys2 gcc update (new version dir), re-stage them — see `docs/toolchain-incident-2026-10-05.md` section 6.
- Network was down (no VPN) on 2026-10-05 morning but GitHub became reachable later that day (push of `8226713..cc58337` succeeded, see Status). If the network is down again, msys2 mirrors and GitHub push are unreachable — never plan a fix that requires `pacman -Sy` or a push; verify connectivity (`git ls-remote`) first.
