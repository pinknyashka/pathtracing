# Milestone 4 plan: materials — white rough-plastic cube + neon-glow frame

Date: 2026-10-06. Status: implemented and verified 2026-10-06 (Milestone 4 complete; see
AGENTS.md for the results — including the GGX NDF-normalization fix that this plan did not
anticipate, and the fact that the exact channel-ratio asserts in `test_tracer` were kept
rather than relaxed, since gray f0/albedo keep every term a gray-scaled emission). Replaces
the M3 material model
(a 2-variant `Diffuse`/`Emissive` enum) with a unified material. The M3 geometry is
unchanged: equatorial ring (plane through the cube center) pitching about the world X axis,
OBB `Box` transforms, NEE-driven lighting, static camera at `(0, 2, 9)`.

## Goal

Give the two objects real, named materials instead of the current `enum { Diffuse, Emissive }`:

- **Cube = white rough plastic** — a dielectric with a soft microfacet (GGX) sheen on top of
  a white diffuse base. It should read as matte-but-satin white plastic: not flat matte
  (the current state) and not mirror-glossy.
- **Frame = neon glow** — the red emissive frame is the scene's only light source; formalize
  it as a neon (emission) material. The M3 emission value `(4.0, 0.18, 0.12)` already reads
  as neon; M4 keeps the value and gives it a first-class material identity. Decided: the frame
  is **pure emission — no glass-tube shell** (keeps the FRAME anchor exact and NEE simple).

Visual target: the white cube shows a soft red-tinted sheen where its faces mirror the neon
ring, on top of the existing red-dominant diffuse lighting; the ring keeps its bright neon
look. In the window the cube should stop looking like flat matte paper.

## Why

The current `Material` (`src/scene/material.h`) is an enum with `albedo`/`emission`. It cannot
express *roughness*, so the cube is flat Lambertian — "matte", not "rough plastic". A unified
material makes rough plastic and neon first-class, and extends the tracer to a microfacet BRDF
while keeping the NEE-driven lighting the scene depends on (see the NEE note below).

## Material model

Replace the enum `Material` with a unified struct (a surface participates in whichever terms
are non-zero; drop `MaterialType` or keep it only as a derived "has-emission" helper):

```cpp
struct Material {
    Vec3  albedo{0, 0, 0};    // diffuse (Lambertian) base color
    Vec3  f0{0, 0, 0};        // specular reflectance (Fresnel F0); (0,0,0) = no specular
    float roughness = 0.f;    // microfacet roughness: 0 = mirror, 1 = fully rough
    Vec3  emission{0, 0, 0};  // emissive radiance (neon); (0,0,0) = not a light

    static Material plastic(Vec3 a, Vec3 f0v, float rough);  // diffuse + GGX sheen
    static Material neon(Vec3 e);                            // pure emission
};
```

- A surface is **diffuse** when `albedo != 0`, **specular** when `f0 != 0`, **emissive** when
  `emission != 0`. An emissive surface still terminates the path exactly as in M3.
- `Scene::cubeMat  = Material::plastic({0.9f, 0.9f, 0.9f}, {0.04f, 0.04f, 0.04f}, 0.4f);`
  — white rough plastic (F0 = 0.04 is the Schlick value for n ≈ 1.5 plastic; roughness 0.4 =
  satin, not gloss).
- `Scene::frameMat = Material::neon({4.0f, 0.18f, 0.12f});` — neon glow (value unchanged from
  M3, so `lightArea`/`lightNormal`/`sampleLightPoint` and every emission anchor stay valid).

## BRDF + sampling (cube = Lambert diffuse + GGX specular)

- **Diffuse (Lambertian):** `brdf_d = albedo / π`.
- **Specular (GGX microfacet):** `brdf_s = D_GGX(α) · G_Smith · F_Schlick(f0) / (4·cosθi·cosθo)`,
  with `α = roughness²`. `F_Schlick = f0 + (1 − f0)·(1 − cosθi)^5`.
- **Energy-conserving blend (decided: Schlick split):** weight by the Schlick Fresnel
  `brdf = F · brdf_s + (1 − F) · brdf_d` — the `(1 − F)` on the diffuse is the Schlick split
  (the exact `(1 − F)(1 − V·...)` correction is out of scope for this scene).
- **Transmissive bounce:** pick the lobe with probability ∝ its integrated weight, then sample
  it (GGX importance sampling for specular, cosine for diffuse). Throughput becomes
  `scale *= brdf_sampled / pdf_sampled` — this replaces the current `scale *= albedo` +
  `cosineBounce` (which assumed a pure-Lambertian surface).
- **NEE (the workhorse — full BRDF):** at each non-emissive hit, sample the neon frame and add
  the combined direct term
  `brdf_total(wo→li) · cosL · cosEmit / d² · (lightArea / kVisSamples)`,
  where `brdf_total = F·brdf_s + (1−F)·brdf_d`. This generalizes the current `albedo · nee`
  term (whose `1/π` lives in the existing `kPi` denominator) to the full BRDF, so **both** the
  diffuse lighting and the specular sheen are lit by the ring in a single low-variance shot.
- **Emissive hit (unchanged from M3):** a primary (bounce 0) emissive hit returns raw emission;
  a continuation (bounce > 0) emissive hit adds nothing and breaks.

### NEE note (why the full BRDF in NEE, and why bounce-only specular would be wrong)

A continuation ray that hits the neon frame contributes **zero** (it breaks without adding) —
by design, so NEE is the *only* estimator for ring→surface light (AGENTS.md: "don't remove
NEE"). Consequence: the specular sheen **must** be captured in the NEE term (via `brdf_s`); a
bounce-only specular lobe would be invisible, because bounce rays that reach the frame add
nothing. Routing the full `brdf_total` through NEE is both correct and low-variance. Full MIS
between the BRDF sampler and the light sampler is an optional refinement (v1.1); with a single
large emitter, NEE already dominates the estimate.

## Tracer / source changes

- `src/scene/material.h`: unified `Material` struct + `plastic`/`neon` presets.
- `src/scene/scene.h`: `cubeMat`/`frameMat` switch to the new presets (same numeric
  albedo/emission values, so the light model and all M3 light tests stay valid).
- `src/render/tracer.h`:
  - "is emissive" test becomes `hit.material->emission != 0` (vec compare);
  - the diffuse-only NEE term (`albedo * nee` with the `kPi` denominator) becomes the
    combined-BRDF NEE term above;
  - `cosineBounce` + `scale *= albedo` becomes a `scatteringDirection(hit, rng)` /
    `brdfPdf(hit, dir)` pair (transmissive GGX/cosine) with `scale *= brdf_sampled / pdf_sampled`;
  - Russian roulette (from bounce ≥ 2) and `kMaxBounces` stay as-is.
- `CMakeLists.txt`: add `material` to the `foreach(t IN ITEMS ...)` test list.

## Tests

- **New `tests/test_material.cpp`:** GGX `D` integrates to ≈1 over the hemisphere
  (fixed-seed Monte-Carlo); Fresnel-Schlick equals `f0` at normal incidence and →1 at grazing;
  the combined BRDF is positive/finite and the F/(1−F) split stays in `[0,1]`; the
  `plastic()`/`neon()` presets expose the expected fields.
- **`test_tracer` re-anchored** (the only existing tests whose numbers change):
  - **LIT** (θ = 0, top-face pixel `t = 0.54003608`): the top face now also carries a soft
    white specular sheen, so the *exact* channel-ratio asserts (`y/x = 0.045`, `z/x = 0.03`)
    are relaxed to **red-dominant** (`x > y` and `x > z`, red still leads), and the `lit.x`
    band is re-probed. The `(1 − F)` diffuse reduction (≈ ×0.96) plus the white sheen shift
    the value; re-derive the band from a fresh probe (≥32 seeds, as in M3).
  - **SHADOW** (θ = 0, center pixel on the +z face): **expected unchanged — exactly 0.0.**
    Every ring point has z ≈ 0.03 < 0.5 ⇒ cosL < 0 (NEE skips it), and both the cosine and
    the GGX lobes on the +z face emit only into that face's outward (+z) hemisphere, which can
    never reach the ring at z ≈ 0.03. Verify with a probe; expect zero.
  - **FRAME** (primary ray to the top bar's front face): **expected unchanged — exact
    emission `(4.0, 0.18, 0.12)`.** The frame stays a neon (emission) material, so a bounce-0
    emissive hit returns raw emission with no MC noise.
- **Unaffected:** `test_vec3`, `test_box`, `test_camera`, `test_light` (lightArea / 5 boxes),
  `test_scene` (geometry + light sampling), `test_args`, `test_display`.

## Smoke (re-tune after implementation)

Same deterministic recipe (face-on ring at t = 0):

    pathtracer --width 320 --height 320 --spp 32 --frames 1 --time 0.0 --out smoke4
    smoke_stats smoke4/frame_0001.ppm

Expected vs M3: bright-red frame pixels ≈ unchanged (the frame is still neon emission); the
**cube** regions shift — diffuse a touch lower (`(1 − F)`) plus a soft red-tinted specular
sheen on the top/front faces; all four background corners stay exactly black. Re-run and
retune the `smoke_stats` region thresholds for the new cube statistics; add `smoke4/` to
`.gitignore`.

## Work items

- **W1** — Unified `Material` struct + `plastic`/`neon` presets (`src/scene/material.h`),
  keeping the M3 numeric values so the light model is unchanged.
- **W2** — GGX microfacet (`D`, Smith `G`, Fresnel-Schlick) + combined-BRDF value +
  transmissive sampling (GGX / cosine) in `src/render/tracer.h`; `brdf_sampled/pdf_sampled`
  throughput.
- **W3** — Combined-BRDF NEE term (replaces the diffuse-only term); keep the M3 emissive-hit
  rule (bounce 0 → emission, bounce > 0 → nothing).
- **W4** — Wire the cube to `plastic` and the frame to `neon` in `scene.h`.
- **W5** — New `tests/test_material.cpp` + the `material` target in `CMakeLists.txt`.
- **W6** — Probe and re-anchor `test_tracer` (LIT band + red-dominant ratios; confirm
  SHADOW/FRAME still hold).
- **W7** — Smoke v4: render `smoke4`, retune `smoke_stats`, record the PASS numbers.
- **W8** — Window acceptance: `pathtracer.exe --frames 2` (no `--out`) → the cube reads as
  satin white plastic with a red sheen and the ring reads as neon (visual, user-confirmed).

## Decisions (resolved 2026-10-06)

1. **Depth:** **full GGX + combined-BRDF NEE** — the honest "rough plastic"; the NEE term uses
   the full `F·brdf_ggx + (1−F)·brdf_lambert` (no lighter single-lobe fallback). MIS between
   the BRDF and light samplers is an optional v1.1 refinement, not part of M4.
2. **Frame glass:** **pure neon emission, no glass-tube shell** — keeps the FRAME anchor exact
   and NEE simple.
3. **Diffuse split:** **Schlick `(1 − F)`** on the diffuse (the exact `(1 − F)(1 − V·...)`
   correction is out of scope).

## Acceptance

- ctest 9/9 (adds `test_material`); `test_tracer` re-anchored and green, SHADOW still exactly
  0, FRAME still exact emission.
- Smoke v4 PASS with retuned stats: neon frame unchanged, cube reads as satin white plastic
  (red-dominant diffuse + soft red sheen), background corners black.
- Window: the cube shows a soft sheen (no longer flat matte) and the ring reads as neon.
- Milestone committed + pushed to `origin main`; AGENTS.md status updated to "M4 complete".
