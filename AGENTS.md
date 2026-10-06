# AGENTS.md

- Project: CPU path tracer in C++20, built with CMake. Scene: white rough-plastic unit cube (white diffuse + GGX microfacet sheen) at the origin + a thin 2×2 neon-glowing red square frame (emission; 4 transformed boxes, ring plane at local `z = 0`, i.e. centered on the cube center — an equatorial ring) pitching about the world X axis (12 s period); static camera at `(0, 2, 9)` looking at the origin. Frame bars are OBBs: `Box` carries a rigid transform (`Mat3 R`, `Vec3 T`); `Scene::setFrameAngle(θ)` drives the pitch (`R = Mat3::rotX(θ)`). The material model (rough-plastic cube, neon frame) landed in Milestone 4 — `docs/plan-materials.md`. Milestone 5 adds a **strong neutral ambient fill light** (`Scene::envRadiance(dir)`, a vertical-gradient light estimated by NEE) that illuminates the cube so it reads as a bright **white** object; it is a light, not a visible sky — the background (empty void) stays **black** because nothing out there reflects light, and the neon ring remains the only saturated (red) source. Milestone 6 makes the ring's glow **distinct on the cube**: the neon red is 8× brighter (`neon({32.0, 0.18, 0.12})`, G/B unchanged) and the ambient is scaled to ~0.75× (`3.4/2.6/1.9`), so the cube's top face reads bright **white with a clear red cast** (tone-mapped R−G ≈ 10 levels) instead of M5's sub-level tinge. Milestone 7 makes the cube's surface read as **reflective**: the cube is now white **glossy** plastic (`Material::glossyWhite()` — F0 0.04→0.15 pearl-level, roughness 0.40→0.30) and the ring red is 3× brighter (`neon({96.0, 0.18, 0.12})`), so the ring's cast concentrates into a strong red **reflection band** across the cube's top face (tone-mapped R−G ≈ 26–36 levels, vs M6's ~10) while the faces stay bright white and the void stays black. True point-mirror images of the ring on the cube are geometrically impossible (every face's mirror ray points into the void — the ring girdles the cube at its equator), so the band is the ring's microfacet-sheen reflection; full analysis: `docs/plan-reflections.md`.
- Everything is computed on CPU by design — no GPU shaders/hardware acceleration. Frame-rate drops at high `--spp`/`--res` are expected, not bugs.
- SDL2 is an optional dependency: if `find_package(SDL2)` fails, the renderer must fall back to writing PPM frames to `frames/`. Don't assume a window will open.
- Build: out-of-source `build/` (never commit it).
  - Configure: `cmake -B build` (this machine: use the README "Local toolchain" line — local CMake/Ninja + `C:/msys64/ucrt64/bin/g++.exe`)
  - Build: `cmake --build build -j`
  - Test: `ctest --test-dir build` (single test: `ctest --test-dir build -R <name>`)
- Layout: `src/math` (Vec3/Ray), `src/scene` (geometry, materials, camera), `src/render` (tracer, display), `tests/`, `src/main.cpp` (CLI + animation loop).
- Radiance is HDR through the tracer (emission > 1.0); tone-map/clamp only at display or PPM write time.
- Tracer correctness relies on next-event estimation sampling the frame directly — a thin frame is rarely hit by random bounces; don't remove NEE.
- Before declaring the renderer correct, run the full ctest suite plus a PPM smoke check. Deterministic recipe (static camera, no timing race): `pathtracer --width 320 --height 320 --spp 32 --frames 1 --time 0.0 --out smoke7` then `smoke_stats smoke7/frame_0001.ppm` — at t=0 s the equatorial ring is face-on (θ=0, ring plane at z=0 through the cube center): expect bright-red ring top/bottom bands + side bars (≈978 brightRed px; the neon frame is exact emission and unchanged by the ambient, now tone-mapping to a clamped (≈252, 33, 23)), the cube's **top** face bright **white with a strong red reflection band** (avgR≈178 vs avgG≈avgB≈151 — M7's distinct sheen reflection, R−G ≈ 26 levels; all region px lit, maxR≈193 = the sheen hotspot), the **front** face bright **neutral** white too (ambient-only at θ=0), and all four background corner boxes **black** (0.00 — the void reflects nothing; `dark` ≈ 97% of pixels).
- Keep this file updated as the build system, compiler flags, and test setup are finalized.
- Todo discipline: maintain a session todo list and update it every time a subgoal is finished — mark a task `in_progress` before starting it and `completed` as soon as it is verified. Never batch status updates at the end of a session.

# Task Distribution Matrix (Routing Contract)

You are the lead coordinating agent (Plan/Build mode) powered by Qwen. Your task is to decompose complex requirements and assign highly specialized sub-tasks to sub-agents.

## Available sub-agents:
1. `@tester` — Use this agent ALWAYS when the main code is written and requires test coverage or regression testing.
2. `@scout` — Built-in agent. Call this agent if you need to analyze external documentation or cached dependencies.
3. `@doc-writter` — Call this agent if you need to write documentation for the new feature.

## Invocation instructions:
If the task is complex:
1. Create a high-level plan.
2. Delegate test writing to the sub-agent by sending a command in this format: `call @tester to generate tests for src/auth.py`.
3. If you lack information about external libraries, frameworks, or existing codebase architecture, delegate the research to @scout before generating code.
4. Wait for the sub-agent's response, analyze the result, and conclude the session.

## Status (as of 2026-10-06, after Milestone 7: reflections on the cube surface)

- Repo at https://github.com/pinknyashka/pathtracing (public); local `origin` = that URL. Remote `main` is in sync with local through the Milestone-4 commit `2a2c54b`; Milestones 5–7 were verified locally in the working tree (left uncommitted by the M6 session and, by user choice, M7 was built on top) and are committed together as one commit and pushed in this session.
  - Push: a plain `git push origin main` works — Windows Credential Manager holds a GitHub credential for `pinknyashka` with push scope (pushed `f8d5c72..6b7d5a8` on 2026-10-05 and `1861242..8bc8759` on 2026-10-06 without an explicit token). Fallback if that credential expires: push with a token that has `public_repo`/`repo` scope (a read-only token gets 403): `git -c credential.helper= push https://x-access-token:<TOKEN>@github.com/pinknyashka/pathtracing.git main` — the `credential.helper=` override stops the token being saved to Windows Credential Manager.
- Toolchain blocker RESOLVED offline (no msys2 package changes): `D:\Projects\__tools\mingw_7_2_0` on the machine PATH shadowed ucrt64's runtime DLLs, so gcc-15 frontends crashed at startup with `0xC00000FD`. Fix: User PATH now starts with `C:\msys64\ucrt64\bin`, plus 8 DLLs staged next to cc1/cc1plus. Full report + prevention runbook: `docs/toolchain-incident-2026-10-05.md`.
- NEE self-blocking fixed in `src/render/tracer.h`: the visibility test now skips emissive boxes (it used to block on the sampled bar's own front face), and the emission contribution uses `cosEmit/d²`. Verified with a 1024-spp probe: lit.x=0.0503, shadow=0.0, framePix=(4.0, 0.18, 0.12); `test_tracer` expects lit.x in 0.035–0.065.
- CLI parser fixed: `parseArgs` called its `value()` lambda twice (each call bumps the arg index, so every option value was skipped — `--width 320` parsed as 0). Parser moved to `src/cli_args.h`, `--accumulate` branch restored, covered by `tests/test_args.cpp`.
- CLI strictness fixed: `parseArgs` now rejects non-numeric/overflow values for `--width/--height/--spp/--frames` and negative `--seed` (previously `atoi` silently mapped junk to 0, e.g. `--frames abc` ran forever in PPM mode). `Display::writePPM` returns bool; `main` exits 1 with a diagnostic if a frame can't be written. Covered by extended `test_args` + new `test_display` (toneMap checks + PPM write/read round trip).
- ctest 9/9 pass: `test_vec3`, `test_box`, `test_camera`, `test_light`, `test_scene`, `test_material`, `test_tracer`, `test_args`, `test_display`.
- PPM smoke check PASS (320x320, 16 spp, camera behind the frame at t≈9.6s, analyzed with `build/smoke_stats.exe`): 2298 bright-red frame px; the cube face facing the frame is lit and red-dominant (maxR≈13 = tone-mapped lit.x≈0.05; ~65% of face px lit, the rest in bar shadow); top face + background black. Re-verified after the CLI/display changes: frames at t_wall 9.3–9.8s show 2600–3700 bright-red px, lit red-dominant cube front, black top face + background boxes. (Note: `smoke_stats` regions are tuned for t≈9.3–9.8s; earlier in the behind-frame window, e.g. t≈9.0s, a frame bar can project into the lower-left "background" region — that is the bar itself, not a lighting leak.)
- All work pushed to GitHub `main` (through the Milestone-4 commit `2a2c54b`); Milestones 5–7 ride along as one commit in this session.
- **Milestone 7 (complete, 2026-10-06): reflections on the cube surface** — the ring's cast on
  the cube was a red *tint* on a matte cube (M6: top-face R−G ≈ 10 tone-mapped level). Make the
  surface read as **reflective**: the cube is now white **glossy** plastic (new preset
  `Material::glossyWhite()` in `src/scene/material.h` — albedo 0.9, **F0 = 0.15** pearl-level,
  **roughness 0.30**; was F0 0.04 / roughness 0.40 satin) and the ring red is **3× brighter**
  (`frameMat = neon({96.0, 0.18, 0.12})`, G/B unchanged — the hue stays a deep saturated red
  and the bands tone-map to a clamped (≈252, 33, 23)), so the ring's cast concentrates into a
  strong red **reflection band** across the top face (tone-mapped R−G ≈ 26 levels at 32 spp,
  maxR ≈ 193 at the sheen hotspot) while the faces stay bright white and the void stays black.
  **Key geometric finding** (full analysis: `docs/plan-reflections.md`): the ring girdles the
  cube at its equator, so the **mirror direction of every visible face points into the void**
  (front face: up-and-back, misses the top bar by a wide margin; top face: toward the camera
  side, ring sits on the other side) — a true point-mirror image of the ring on the cube does
  **not** exist in this scene, and tightening the lobe cannot create one (the ring's light
  directions sit 35–45° off the lobe peak, where D collapses as roughness drops; probe:
  roughness 0.40→0.12 moves the cube's radiance < 0.5% and the 4-spp noise spread is
  bit-identical — it is set by the 2-sample diffuse ring NEE, not the lobe). The ring's
  "reflection" on the cube is therefore its **microfacet-sheen band** — the NEE combined-BRDF
  specular term (which the probes show is only ≈ 0.03 of the cube's radiance at M6 values),
  made visible by the ring boost, strongest on the front-middle of the top face (fading toward
  the back edge, where the top bar is seen from its back side, `cosEmit < 0`, and toward the
  front edge, where the lobe alignment degrades); during the wobble the band moves and warps
  with the ring. Values-only milestone: no tracer/geometry changes; `test_tracer` re-anchored
  (64 spp × 256 seeds): LIT top face `x∈[1.89,3.06]`, `x−y∈[0.54,1.67]` → asserts
  `x∈(1.5,3.6)`, `x−y > 0.45`, `x > 1.2·y`, `x > z` (a ring reverted to M6's 32.0 erodes
  x−y to ~0.4 / x·y to ~1.3 — both floors catch it; a broken ring erodes both to ~0);
  FRONT stays ambient-only neutral white `x∈[1.94,2.08]`, `y/x=1.000` → keeps
  `x∈(1.5,4.0)`, `y > 0.9·x`; FRAME exact emission now (96.0, 0.18, 0.12). `test_material`
  gains the `glossyWhite()` preset check; neon constants updated to (96.0, 0.18, 0.12).
  ctest 9/9, smoke v7 PASS (978 brightRed px; cube top (177.8, 151.4, 151.4), maxR 193;
  bands (252, 33, 23); corners black; dark ≈ 97.3%).
- **Milestone 6 (complete, 2026-10-06): distinct neon glow on the cube** — the ring's cast on the
  cube was a sub-level tinge under the M5 ambient (top-face R−G ≈ 1 tone-mapped level). Values-only
  change in `src/scene/scene.h`: `frameMat = neon({32.0, 0.18, 0.12})` (red 8× brighter, G/B
  unchanged — hue stays deep saturated red, the `brightRed` smoke classification still applies,
  bands now tone-map to (≈245, 33, 23)) and the ambient scaled to ~0.75×
  (`envZenith/Horizon/Nadir` 4.5/3.5/2.5 → 3.4/2.6/1.9). The cube still reads bright white, the
  void stays black, the ring stays the only saturated source. `test_tracer` re-anchored (64 spp ×
  256 seeds): LIT top face `x∈[1.78,2.15]`, `y/x∈[0.71,0.87]`, `x−y∈[0.24,0.63]` → asserts
  `x∈(1.5,3.0)`, `x > z`, plus the red-glow floors `x − y > 0.1` and `x > 1.05·y`; FRONT-FACE
  `x∈[2.19,2.36]`, `y/x=1.000` → keeps `x∈(1.5,4.0)`, `y > 0.9·x`; FRAME exact emission now
  (32.0, 0.18, 0.12). `test_material` neon constants updated to match. ctest 9/9, smoke v6 PASS.
- **Milestone 5 (complete, 2026-10-06): ambient fill light** — a **strong neutral** vertical-gradient
  environment light (`Scene::envZenith/Horizon/Nadir`, `Scene::envRadiance(dir)`) is the scene's
  fill light. It is estimated by **NEE** (cosine-sampled, `src/render/tracer.h`), not by bounces: a
  bounce against a smooth light field carries a `1/cos` weight whose second moment is infinite
  (heavy-tailed, never converges cleanly), whereas the cosine-sampled NEE term cancels `cosL`
  against the pdf so each diffuse sample is just `L·albedo` (bounded, low-variance) and the same
  samples light the GGX sheen. The ambient is a **light, not a visible sky**: escape rays (primary
  or bounce) return **black** — the background is an empty void that reflects nothing, so it stays
  black, while the NEE term illuminates the cube so it reads as a bright **white** object. It is
  **neutral** (not warm) so the cube reads white and the neon ring stays the only saturated (red)
  source. `test_tracer` re-anchored: LIT top face now `x∈(1.5,3.0)` + `x > z` (bright white with a
  faint red tinge — the strong neutral ambient breaks M4's exact `y/x=0.045`,`z/x=0.03` ring ratios
  and pulls `y/x` to ~0.97, so only a weak `x`-lead remains); the M3 "SHADOW" (front face, unlit by
  the equatorial ring) is now ambient-lit bright **neutral white** (`x∈(1.5,4.0)`, `y > 0.9·x`),
  no longer exactly 0; FRAME stays exact emission. ctest 9/9. Smoke v5: see below.
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
  Milestone committed (`2a2c54b`) and pushed to GitHub `main` 2026-10-06; local and remote in sync.

## Milestone 5 (complete, 2026-10-06): ambient fill light — strong neutral light on a black void

- Goal: the cube read as black/gray against a black background (only the ring-lit top face had
  any light). Add a **strong neutral ambient fill light** so the cube reads as a bright **white**
  object, while the background stays **black** (empty void — nothing out there reflects light) and
  the neon ring stays the only saturated (red) source.
- Implementation: `Scene` gains `envZenith/envHorizon/envNadir` + `envRadiance(dir)` (a
  zenith→horizon→nadir gradient, `src/scene/scene.h`) — a strong (≈2.5–4.5), neutral light. In the
  tracer (`src/render/tracer.h`): a surface's ambient illumination is added by a **second NEE
  term** that cosine-samples the hemisphere, tests visibility against *all* geometry (the ring
  occludes part of the light, unlike the ring NEE which skips emitters), and weights by
  `combinedBrdf · kPi` — for the diffuse lobe the `cosL` cancels the cosine pdf so each sample is
  `L·albedo` (bounded, low-variance), and the same samples light the GGX sheen. Escape rays
  (primary or bounce) return **black**: the ambient is a light, not a visible sky, so the void
  stays black. (Estimating the ambient through the bounce instead would carry a `1/cos` weight
  with infinite second moment — a smooth light field is a bad bounce target — and would double-
  count the NEE'd term.) Neutral (not warm) on purpose, so the cube reads white and the ring is
  the only saturated source.
- Design trade-off accepted: pure NEE for the ambient captures direct ambient + surface
  interreflection but not multi-bounce ambient transport (light→face→face) — negligible in this
  one-cube scene. The specular sheen is NEE'd with a cosine pdf (some variance on grazing faces)
  rather than GGX-importance-sampled, in exchange for the clean diffuse term.
- `test_tracer` re-anchored (probed 64 spp × 256 seeds): **LIT** top face `x∈[1.98,2.21]`, bright
  white with a faint red tinge `y/x≈0.97` (the strong neutral ambient breaks M4's exact
  `y/x=0.045`,`z/x=0.03` ring ratios and pulls `y/x` to ~0.97, so only a weak `x`-lead remains)
  → asserts `x∈(1.5,3.0)` and `x > z`; **FRONT-FACE** (ex-M3 SHADOW) is now ambient-lit bright
  **neutral white** `x∈[2.92,3.15]`, `y/x=1.000` → asserts `x∈(1.5,4.0)` and `y > 0.9·x` (not the
  ring's red); **FRAME** unchanged (bounce-0 exact emission). `test_scene`/`test_light`/others
  unaffected (ring sampling only). ctest 9/9.
- Smoke v5 PASS (320×320 @ 32 spp, `--time 0.0`, seed-robust): **832** bright-red px (the frame
  is exact emission, unchanged); **dark ≈ 97%** (the void is black again); ring top/bottom bands
  avgR≈195.0/192.8, side bars ≈171.4/172.1 (region boxes unchanged); **cube top** avgR=176.0/
  avgG=175.0/avgB=175.0 (M4: 9.6/0.4/0.05 — now bright **white** with a faint warm tinge, all 93
  region px lit, maxR=187); all four background corner boxes exactly **black** (0.00). `smoke5/`
  added to `.gitignore`.
- Window acceptance: the cube now reads as a bright white object on a black void, framed by the
  glowing red ring (visual, user-confirmed).

## Milestone 6 (complete, 2026-10-06): distinct neon glow on the cube

- Goal: in M5 the ring's cast on the cube (pre-tone-map red excess ≈ 0.05 on top of the ≈ 2.1
  neutral ambient) was a sub-level tint (top-face R−G ≈ 1 tone-mapped level) — the glowing frame's
  effect on the cube's surface was "almost invisible". Make the red glow distinct.
- Change (values only in `src/scene/scene.h`; no tracer/geometry changes): the neon **red channel
  is 8× brighter** — `frameMat = neon({32.0, 0.18, 0.12})` (G/B unchanged, so the hue stays a deep
  saturated red and the `brightRed` smoke rule `r>150, g<100, b<100` still classifies the frame;
  the bands now tone-map to (≈245, 33, 23), maxR 247 = clamped HDR) — and the **ambient fill is
  scaled to ~0.75×** (`envZenith/Horizon/Nadir` 4.5/3.5/2.5 → 3.4/2.6/1.9) so the ring light is
  not washed out. The cube still reads bright white, the void stays black, and the ring remains
  the only saturated (red) source.
- `test_tracer` re-anchored (probed 64 spp × 256 seeds): **LIT** top face `x∈[1.78,2.15]`,
  `y/x∈[0.71,0.87]`, `x−y∈[0.24,0.63]` (a distinct red glow, vs M5's sub-0.05 tinge) → asserts
  `x∈(1.5,3.0)`, `x > z`, plus new red-glow floors `x − y > 0.1` and `x > 1.05·y` (a broken ring
  or a full-strength ambient erodes both); **FRONT-FACE** is ambient-only neutral white
  `x∈[2.19,2.36]`, `y/x=1.000` → keeps `x∈(1.5,4.0)`, `y > 0.9·x`; **FRAME** = exact emission
  (32.0, 0.18, 0.12). `test_material` neon constants updated to (32.0, 0.18, 0.12). ctest 9/9.
- Smoke v6 PASS (320×320 @ 32 spp, `--time 0.0`): **965** bright-red px; ring top/bottom bands
  (245.4, 33.5, 23.2) / (244.7, 33.2, 23.0), side bars ≈ (221.7, 29.5, 20.4) (region boxes
  unchanged); **cube top** avgR=169.1/avgG=158.7/avgB=158.6 (M5: 176.0/175.0/175.0 — now a clear
  **red cast**, R−G ≈ 10 levels, all 93 region px lit, maxR=179 = the ring's red sheen hotspot);
  all four background corner boxes exactly **black**; `dark` ≈ 97.3%. `smoke6/` added to
  `.gitignore`.

## Milestone 7 (complete, 2026-10-06): reflections on the cube surface

- Goal: the ring's cast on the cube was a red *tint* on a matte cube (M6: top-face R−G ≈ 10
  tone-mapped level). Make the cube's surface read as **reflective**: the neon ring's glow
  should appear on the cube as a clear **reflection band**, and the white faces should read
  as glossy plastic.
- **Key geometric finding** (full plan + probes + rejected alternatives:
  `docs/plan-reflections.md`): the ring girdles the cube at its **equator** (plane through
  the cube center) and the camera sits above the scene at `(0, 2, 9)`, so the **mirror
  direction of every visible face points into the void** — never back to the ring (front
  face: mirror ≈ `(0, +0.2, −0.98)` up-and-back, misses the top bar at y = ±1 by a wide
  margin by the time it reaches the ring plane; top face: toward the camera side, ring on
  the other side; side faces: exit through the ±x void). A true point-mirror image of the
  ring on the cube **does not exist** in this scene, and tightening the GGX lobe cannot
  create one — the ring's light directions sit 35–45° off the lobe peak, where D(·)
  collapses as roughness drops (probe: roughness 0.40→0.12 moves the cube's radiance
  < 0.5%, and the 4-spp per-seed noise spread is bit-identical for every roughness — it is
  set by the 2-sample diffuse ring-NEE estimator, not the lobe). The ring's "reflection"
  on the cube is its **microfacet-sheen band**: the NEE combined-BRDF specular term, which
  at M6 values is only ≈ 0.03 of the cube's radiance (the visible red is diffuse ring
  light), concentrated on the front-middle of the top face (the top bar is seen from its
  front side only where z > 0.03 — `cosEmit` kills the back edge; the front edge loses
  lobe alignment) and moving/warping with the ring during the wobble.
- Change (values + one preset; no tracer/geometry changes): the cube is now white
  **glossy** plastic — new preset `Material::glossyWhite()` in `src/scene/material.h`
  (albedo 0.9, **F0 = 0.15** pearl-level reflectance, above the dielectric 0.04;
  **roughness 0.30**; was F0 0.04 / roughness 0.40 satin) — and the neon red is **3×
  brighter**: `frameMat = neon({96.0, 0.18, 0.12})` (G/B unchanged, hue stays a deep
  saturated red, bands tone-map to a clamped (≈252, 33, 23)). The ring's cast scales
  linearly, so the boost turns M6's R−G ≈ 10-levels tint into a **distinct red
  reflection band** (R−G ≈ 26–36 tone-mapped levels) while the ambient (M6's ~0.75×
  gradient) keeps the faces bright **white** and the void **black**; the ring stays the
  only saturated source. Rejected by the probes: mirror-glossy lobes (roughness ≤ 0.15 —
  the ring's NEE specular term → 0 and the env highlight term stays ≈ 0 because each
  face's lobe peak points at the cube's own interior or the void, so the cube just
  darkens) and higher-F0-only (energy conservation steals diffuse weight faster than the
  sheen grows — the red cast *weakens*).
- `test_tracer` re-anchored (probed 64 spp × 256 seeds): **LIT** top face
  `x∈[1.89,3.06]`, `y∈[1.27,1.44]`, `x−y∈[0.54,1.67]` (mean 1.04), `y/x≈0.57` →
  asserts `x∈(1.5,3.6)`, `x−y > 0.45`, `x > 1.2·y` (M6's floor was 1.05), `x > z`;
  **FRONT-FACE** stays ambient-only neutral white `x∈[1.94,2.08]`, `y/x = 1.000` → keeps
  `x∈(1.5,4.0)`, `y > 0.9·x`; **FRAME** = exact emission (96.0, 0.18, 0.12).
  `test_material` gains the `glossyWhite()` preset check; neon constants updated to
  (96.0, 0.18, 0.12). ctest 9/9.
- Smoke v7 PASS (320×320 @ 32 spp, `--time 0.0`): **978** bright-red px (M6: 965 — a few
  bar-edge pixels newly cross the threshold once the bands clamp to 252); ring top/bottom
  bands (251.6, 33.5, 23.2) / (251.3, 33.2, 23.0) — clamped HDR, G/B identical to M6, side
  bars ≈ (229.9, 29.5, 20.4) (region boxes unchanged); **cube top** (177.8, 151.4, 151.4)
  (M6: 169.1/158.7/158.6 — now a strong red **reflection band**, R−G ≈ 26 levels, all 93
  region px lit, maxR = 193 = the sheen hotspot); all four background corner boxes exactly
  **black**; `dark` ≈ 97.3%. `smoke7/` added to `.gitignore`.
- Window acceptance: `pathtracer.exe --frames 2` (no `--out`) opened the SDL window,
  960×540 @ 4 spp ≈ 35.7 ms first frame, 28.1 ms second (≈35 fps), clean exit rc=0
  (verified 2026-10-06). The glossy white cube framed by the glowing red ring, the
  reflection band tracking the ring through the wobble (visual, user-confirmed).

## Toolchain notes (this machine)

- Compiler: msys2 ucrt64 GCC 15.2.0 at `C:\msys64\ucrt64\bin\g++.exe`; CMake/Ninja are local at `D:\Projects\__tools\{cmake,ninja}` (configure line in README).
- **PATH pitfall**: the machine PATH contains `D:\Projects\__tools\mingw_7_2_0\mingw64\bin` (2017-era libgmp/libmpfr/libmpc/libwinpthread + binutils). If `C:\msys64\ucrt64\bin` is not ahead of it on PATH, cc1/cc1plus load the 2017 DLLs and crash at startup with `0xC00000FD`, no diagnostics. `C:\msys64\ucrt64\bin` is first in the **User** PATH (set 2026-10-05).
- **Stale-session gotcha**: processes started before a PATH change (including this CLI) keep the old PATH. In shell commands, re-derive it first: `$u=[Environment]::GetEnvironmentVariable("Path","User"); $m=[Environment]::GetEnvironmentVariable("Path","Machine"); $env:PATH="$u;$m"`.
- Backstop: the frontends' own directory (`C:\msys64\ucrt64\lib\gcc\x86_64-w64-mingw32\15.2.0\`) holds staged copies of libwinpthread-1, libgcc_s_seh-1, libgmp-10, libmpfr-6, libmpc-3, libisl-23, zlib1, libzstd. After any msys2 gcc update (new version dir), re-stage them — see `docs/toolchain-incident-2026-10-05.md` section 6.
- Network was down (no VPN) on 2026-10-05 morning but GitHub became reachable later that day (push of `8226713..cc58337` succeeded, see Status). If the network is down again, msys2 mirrors and GitHub push are unreachable — never plan a fix that requires `pacman -Sy` or a push; verify connectivity (`git ls-remote`) first.
