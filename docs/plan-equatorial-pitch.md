# Milestone 3 plan: equatorial ring — frame centered on the cube, pitching about X

Date: 2026-10-06. Status: complete (committed `8bc8759`, pushed to `origin main`; see
AGENTS.md "Milestone 3"). Supersedes the M2 "frame at z = −3 orbiting about Y" layout and
its smoke recipe v2.

## Goal

The 2×2 red emissive frame becomes an **equatorial ring**: its plane passes through the
**cube center** (local `z = 0`, ring center == cube center == pitch pivot at the origin) and
it **pitches about the world X axis** (the screen-horizontal axis through the camera view),
12 s period. With the static camera at `(0, 2, 9)` the result is a red square hoop that
wobbles up and down around the white cube: face-on to the camera at t = 0 (a horizontal
square around the cube, seen obliquely from above), edge-on/vertical at t = 3 s and
t = 9 s, inverted at t = 6 s, back to face-on at t = 12 s.

The M2 layout put the ring a full 3 units behind the scene center, so the "orbit" read as
the frame sweeping far away and past the camera; M3 keeps the light source co-located with
the subject, which also shortens NEE distances and makes the cube lighting read more
strongly.

## Changes (relative to M2)

- `src/scene/scene.h`: `kFrameZ: -3.0 → 0.0` (ring plane through the origin);
  `setFrameAngle(θ)` now sets `R = Mat3::rotX(θ)` instead of `rotY(θ)`.
- `src/math/mat3.h`: added `Mat3::rotX(a)` (right-handed rotation about X).
- `src/main.cpp`: `orbitPeriod` → `pitchPeriod` (same 12 s value).
- NEE needs no formula changes: `lightNormal = frameR·(0,0,1)` and `sampleLightPoint`
  already track `frameR`; `lightArea` is rotation-invariant. With `kFrameZ = 0` the ring
  is centered on the pivot, so `lightNormal·p` plane tests stay rotation-invariant.

## Lighting consequences (used by the re-anchored tests / smoke)

At θ = 0 (face-on):
- the cube's **+z face (camera-facing) is unlit** — every ring point has z ≤ 0.03 < 0.5,
  so cosL < 0 there, and cosine bounces from that face stay in the +z hemisphere which
  can never reach the ring → the center pixel receives exactly 0 light (SHADOW anchor);
- the **top face (y = 0.5) is lit red-dominant** by the top bar (y ≈ 0.97, above the face):
  the front half of the top face (z > 0.03) sees the bar, the back half doesn't;
- all four background corners are black (no light behind/above them).

## Tests (re-anchored, all in the working tree)

- `test_scene`: expected `lightNormal` per θ ∈ {0°, 90°, 180°} is now `rotX(θ)·(0,0,1) =
  (0, −sin θ, cos θ)`; ring-plane and bar/hole checks unchanged (local-coordinate based,
  invariant under the pivot move).
- `test_tracer` (static camera, fresh-Engine-per-ray estimator):
  - **LIT**: θ = 0, pixel (s = 0.5, t = 0.54003608) — the ray grazing just above the
    front face's top edge, first hit = top-face point (0, 0.5, 0.3). lit.x ∈ [0.06, 0.14]
    (64-spp over 32 seeds: x ∈ [0.065, 0.131], mean ≈ 0.093; 512-spp 8-seed mean ≈ 0.084 —
    the close-range NEE per-ray estimator is heavy-tailed, ~17 % sd, so the band is wider
    than M2's), plus exact channel ratios y/x = 0.045, z/x = 0.03 (catches wrong light
    color).
  - **SHADOW**: θ = 0, center pixel (front face) → exactly 0.0 radiance, both seeds.
  - **FRAME**: θ = 0, primary ray aimed at the top bar's front-face center
    `(0, 0.97, +0.03)` (passes over the cube at y ≈ 1.024 when z = 0.5) → exact emission
    (4.0, 0.18, 0.12), no MC noise.
- `ctest`: 8/8 pass (`test_vec3`, `test_box`, `test_camera`, `test_light`, `test_scene`,
  `test_tracer`, `test_args`, `test_display`).

## Smoke recipe v3 (replaces v2)

    pathtracer --width 320 --height 320 --spp 32 --frames 1 --time 0.0 --out smoke3
    smoke_stats smoke3/frame_0001.ppm

At t = 0 s the ring is face-on to the camera (horizontal square around the cube center).
`smoke_stats` regions retuned for the face-on layout (top band y≈126–128, bottom band
y≈197–199, side bars x≈122–124/195–197, cube top y≈146–148). PASS (2026-10-06, 320×320
@ 32 spp): 828 bright-red px; ring top/bottom bands avgR≈193–195 (all region px lit,
maxR 204 = clamped HDR); side bars avgR≈171–172; **cube top** avgR = 15.4 / avgG = 0.8
(all 93 region px lit, red-dominant); all four background corners exactly black (0.00).

## Done

- Window acceptance: `pathtracer.exe --frames 2` (no `--out`) opened the SDL window (no
  PPM fallback), 960×540 @ 4 spp ≈ 35 ms first frame / 23 ms second (≈43 fps), clean
  exit rc=0 (verified 2026-10-06; the wobble reads visually as a hoop tilting up/down
  about the horizontal axis).
- Milestone committed (`8bc8759`) and pushed to `origin main` (2026-10-06).
