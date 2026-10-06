# Milestone 7 plan: reflections on the cube surface

Date: 2026-10-06. Status: implemented and verified 2026-10-06 (Milestone 7 complete; see
AGENTS.md for results and smoke v7 numbers). Builds on M5 (ambient fill light) and M6
(distinct neon glow); geometry, camera, tracer, and NEE design are all unchanged.

## Goal

The ring's cast on the cube (M6) was a red *tint* on a matte cube (top face R−G ≈ 10
tone-mapped levels). Make the cube's surface read as **reflective**: the neon ring's glow
should appear on the cube as a clear **reflection band**, and the white faces should read as
glossy plastic — a reflective surface — rather than flat satin.

## Key geometric finding (why this milestone is values-only)

The ring **girdles the cube at its equator** (plane through the cube center), and the camera
sits above the scene at `(0, 2, 9)`. For a mirror-like surface the reflection visible at a
pixel is the scene along that pixel's **mirror direction**, and for this camera/scene the
mirror direction of *every visible face* points into the **void** — never back to the ring:

- **Front face** (n = +z, facing the camera): mirror of the view ≈ `(0, +0.2, −0.98)` — up and
  *back*. By the time it reaches the ring plane (z ≈ 0) it has risen only ≈ 0.1 in y, far below
  the top bar at y = ±1 → misses the ring by a wide margin.
- **Top face** (n = +y): mirror ≈ `(0, −0.16, +0.99)` — toward the camera side; the ring sits
  on the *other* side of the top face (z < 0.03) → into the void.
- **Side faces**: same story (mirror exits through the ±x void).

So a true point-mirror image of the ring on the cube **does not exist** in this scene, and
making the lobe tighter (lower roughness) cannot create one: a tight GGX lobe only *shrinks*
the ring's NEE specular contribution (the ring's light directions sit 35–45° off the lobe
peak, where D(·) collapses as the lobe narrows). Probe-confirmed: with roughness swept
0.40 → 0.12 the cube's radiance moves < 0.5% and the low-spp noise spread is bit-identical
(the spread is set by the 2-sample diffuse ring-NEE estimator, not the lobe).

What the scene *can* show, and what a glossy white object in front of a red ring actually
shows, is the ring's **microfacet sheen** — a broad, localized reflection image: on the top
face it is a red band strongest where the lobe overlaps the top bar's light directions (the
front-middle of the face, fading toward the back edge where the bar is seen from its
back side, `cosEmit < 0`, and toward the front edge where the lobe alignment degrades).
During the wobble that band moves and warps with the ring — reading as the ring's
reflection dancing across the cube's surface. M7 makes that band **strong** and gives the
surface a **glossy** character.

## Change (values + one preset; no tracer/geometry changes)

- **Cube = white glossy plastic** — new preset `Material::glossyWhite()` in
  `src/scene/material.h`: albedo `(0.9, 0.9, 0.9)`, **F0 = 0.15** (pearl-level reflectance,
  above the dielectric 0.04), **roughness 0.30** (was 0.04 / 0.40 satin). F0 = 0.15 is a
  deliberate artistic value: with F0 = 0.04 the reflectance channel is too weak for the
  scene's moderate light levels to surface a visible highlight, and energy-conserving
  behavior means raising F0 *steals* diffuse weight — the probes show the cube's white base
  stays bright (tone-mapped ≈ 150–175) with a clear red band on top.
- **Ring red ×3**: `frameMat = neon({96.0, 0.18, 0.12})` (was 32.0 in M6; G/B unchanged, so
  the hue stays a deep saturated red and the frame still tone-maps to a clamped bright red
  `(252, 33, 23)`). The ring's cast scales linearly in the radiance, so this is the lever
  that turns M6's R−G ≈ 10-levels tint into a **distinct red reflection band** (top face
  R−G ≈ 26–36 tone-mapped levels, maxR ≈ 193 at the sheen hotspot).
- Ambient fill (M6's ~0.75× gradient `3.4/2.6/1.9`) is unchanged: the faces keep reading as
  bright **white** and the void stays **black**; the ring stays the only saturated source.

## Why not (alternatives considered and rejected by the probes)

- **Mirror-glossy lobe (roughness ≤ 0.15)**: converges fine (probe: noise identical), but
  the ring's NEE specular term → 0 (light directions off-peak) *and* the environment's
  specular highlight term stays ~0 (the lobe peak on each face points at the cube's own
  interior or the void, both "blocked"), so the cube just darkens. No visible reflection.
- **Higher F0 only (ring at M6's 32.0)**: energy conservation steals the diffuse weight
  faster than the sheen term grows — the red cast *weakens* (x−y: 0.47 → 0.34) and the white
  base dims. Needs the ring boost to pay off.
- **GGX-importance-sampling the sky NEE**: would only matter for tight lobes (rejected
  above); at roughness 0.30 the cosine-sampled sky term is low-variance, and adding a
  second sampling strategy for a term that measures ≈ 0 here is not worth the code.

## Probes

Scratch probes (fresh `Engine` per sample, 64-spp averages, 256-seed sweeps for anchors;
same estimator as `test_tracer`):

- Round 1 (ring 32, F0 0.04–0.30, rg 0.20–0.40): rg inert; F0 only dims the white base and
  slightly erodes the red excess → the sheen channel is ≈ 0.03 of the cube's radiance.
- Round 2 (ring 32/96/160 × F0 0.10/0.15/0.20 × rg 0.25/0.30/0.35): ring brightness is the
  strong lever (top face x−y: 0.44 → 1.25 → 2.08 at F0 = 0.15); F0 = 0.15, rg = 0.30 chosen
  as the glossy balance (white base bright, red band distinct, frame still brightRed-class).
- Round 3 (noise gauge at ring 32): low-spp (4 spp) per-seed spread of the top face is
  50–52% of the converged value for **every** (F0, rg) combination — set entirely by the
  2-sample diffuse ring NEE, unchanged by M7 → window mode (4 spp + temporal blend) behaves
  exactly as before.
- Final anchors (ring 96, F0 0.15, rg 0.30, 64 spp, 256 seeds): LIT top face
  x ∈ [1.891, 3.061] (mean 2.412), y ∈ [1.269, 1.442], x−y ∈ [0.535, 1.670] (mean 1.038),
  y/x ≈ 0.574; FRONT face x ∈ [1.941, 2.075], y/x = 1.000 (still pure neutral — the ring
  cannot reach this face at θ = 0); FRAME exact emission (96.0, 0.18, 0.12).

## Verification

- `test_tracer` re-anchored: LIT asserts `x ∈ (1.5, 3.6)`, `x − y > 0.45`, `x > 1.2·y`,
  `x > z` (a ring reverted to M6's 32.0 erodes x−y to ~0.4 and x/y to ~1.3 — both floors
  catch it; a broken ring erodes both to ~0); FRONT keeps `x ∈ (1.5, 4.0)`, `y > 0.9·x`;
  FRAME = exact (96.0, 0.18, 0.12). `test_material` gains a `glossyWhite()` preset check
  and the neon constants move to 96.0. ctest 9/9.
- Smoke v7 (320×320 @ 32 spp, `--time 0.0`): 978 bright-red px; ring bands tone-map to
  (≈252, 33, 23) — clamped HDR, G/B identical to M6; **cube top** (177.8, 151.4, 151.4),
  maxR = 193 — a clear red reflection band (R−G ≈ 26, M6: ~10), all 93 region px lit;
  front face bright neutral white; all four background corners exactly black; dark ≈ 97.3%.
- Window acceptance: 960×540 @ 4 spp, 35.7 ms first frame, 28.1 ms steady (≈35 fps), clean
  exit rc=0 — the glossy white cube framed by the glowing red ring, the reflection band
  tracking the ring through the wobble (visual, user-confirmed).
