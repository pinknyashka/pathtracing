#include "check.h"

#include "render/tracer.h"
#include "scene/camera.h"
#include "scene/scene.h"

// Fresh Engine per sample (deterministic per seed), averaged over spp rays.
static Vec3 average(const Scene& scene, const Camera& cam, float s, float t, int spp, unsigned baseSeed) {
    Vec3 acc(0, 0, 0);
    for (int i = 0; i < spp; ++i) {
        Engine eng(scene, baseSeed + (unsigned)i * 101);
        acc = acc + eng.rayColor(cam.ray(s, t));
    }
    return acc * (1.f / (float)spp);
}

int main() {
    // M3 layout: static camera above the scene looking down at the origin; the
    // 2x2 emissive frame ring is centered on the cube center (ring plane
    // through the origin, light at z = +0.03) and pitches about the world X
    // axis; the ctor default is theta = 0 (face-on to the camera). The center
    // pixel (s = t = 0.5) is the camera's look direction, which hits the
    // cube's +z face (z = 0.5) at (0, y ~ 0.206) -- inside the face, and at
    // theta = 0 that face is never lit by the equatorial ring (see SHADOW).
    const Camera cam(Vec3(0, 2, 9), Vec3(0, 0.1f, 0), Vec3(0, 1, 0), 1.0f, 50.f * kPi / 180.f);

    // LIT: theta = 0. The cube's +z face (facing the camera) is never lit by
    // the ring (every light point has z < 0.5), but the front half of the
    // top face (z > 0.03) is lit red-dominant by the top bar. The pixel
    // (s = 0.5, t = 0.54003608) is the ray passing exactly through the
    // top-face point (0, 0.5, 0.3): it grazes just above the front face's
    // top edge (y ~ 0.534 where it crosses z = 0.5), so the top face is the
    // first hit (row ~ 173 of a 320x320 image).
    // M7: the cube is now white GLOSSY plastic (F0 = 0.15, roughness 0.30 --
    // pearl-level reflectance) and the ring red is 3x brighter (96.0, was
    // 32.0), so the top face reads BRIGHT WHITE with a STRONG red reflection
    // band: the ring's microfacet-sheen reflection. The geometric mirror ray
    // of every face points into the void (the ring girdles the cube at its
    // equator), so no true mirror image exists -- the sheen band IS the
    // ring's reflection on the cube. Probed (64 spp, 256 seeds): x in
    // [1.891, 3.061], y in [1.269, 1.442], x - y in [0.535, 1.670]
    // (mean 1.038), y/x mean 0.574. A ring reverted to M6's 32.0 erodes the
    // red lead to x - y ~ 0.4 (x/y ~ 1.3); a broken ring erodes it to ~0.
    // A broken ambient drops x to the ring-only level; a doubled term pushes
    // x past ~4.5.
    {
        Scene sceneLit;  // ctor leaves the frame at theta = 0
        const Vec3 lit11 = average(sceneLit, cam, 0.5f, 0.54003608f, 64, 11);
        const Vec3 lit211 = average(sceneLit, cam, 0.5f, 0.54003608f, 64, 211);
        for (int i = 0; i < 2; ++i) {
            const Vec3 lit = (i == 0 ? lit11 : lit211);
            CHECK(lit.x > 1.5f);      // bright: ambient fills the face white (M6 floor, still valid)
            CHECK(lit.x < 3.6f);      // not doubled (a doubled ambient pushes x past ~4.5)
            CHECK(lit.x - lit.y > 0.45f);  // the strong red reflection band (probed min 0.535); a ring reverted to M6's 32.0 (x-y ~ 0.4) or a broken ring (~0) erodes it
            CHECK(lit.x > 1.2f * lit.y);   // x/y probed min ~1.31; the red lead is now large (M6's floor was 1.05)
            CHECK(lit.x > lit.z);
        }
    }

    // FRONT-FACE (M3 "SHADOW"): theta = 0. The center pixel hits the front
    // face (z = 0.5), where every ring point has cosL < 0 (the light lies
    // behind the face) and both scatter lobes emit only into its outward +z
    // hemisphere, which never reaches the ring (z <= 0.03) -- so the face is
    // unlit by the neon ring. M5: the strong neutral ambient fill (NEE'd, and
    // visible to this face through the +z hemisphere the ring can't occlude)
    // lights it BRIGHT WHITE. It must read as neutral white (y ~= x), NOT as
    // the ring's red, and stay bright. M7: the cube is now white GLOSSY
    // plastic (F0 = 0.15, roughness 0.30) and the ring red is 96.0 -- but at
    // theta = 0 the front face is still unlit by the ring (all ring points
    // have z < 0.5 behind the face), so it stays bright NEUTRAL white
    // (probed 64 spp, 256 seeds: x in [1.941, 2.075], y/x = 1.000 exactly --
    // pure neutral, no ring red on this face). A broken ambient drops x to
    // exactly 0; a doubled ambient pushes it past ~4.5.
    {
        Scene sceneShadow;  // ctor leaves the frame at theta = 0
        const Vec3 sh12 = average(sceneShadow, cam, 0.5f, 0.5f, 64, 12);
        const Vec3 sh212 = average(sceneShadow, cam, 0.5f, 0.5f, 64, 212);
        for (int i = 0; i < 2; ++i) {
            const Vec3 sh = (i == 0 ? sh12 : sh212);
            CHECK(sh.x > 1.5f);              // bright: ambient fills it white
            CHECK(sh.x < 4.0f);              // ambient only, not doubled
            CHECK(sh.y > 0.9f * sh.x);       // neutral white, not the ring's red
        }
    }

    // FRAME: primary ray aimed straight at the top bar's front-face center
    // (0, 0.97, +0.03) at theta = 0 (the bar's front face is the +z side of
    // the ring plane). The ray passes OVER the cube (at z = 0.5 it is at
    // y ~ 1.024 > 0.5), so the bar is the first hit, and an emissive hit at
    // bounce 0 returns the raw emission (96.0, 0.18, 0.12) -- M7 tripled the
    // red channel (32.0 -> 96.0) -- with no MC noise (measured exactly).
    {
        Scene sceneFrame;  // ctor leaves the frame at theta = 0
        const Ray r{Vec3(0, 2, 9), (Vec3(0, 0.97f, 0.03f) - Vec3(0, 2, 9)).unit()};
        Vec3 acc(0, 0, 0);
        for (int i = 0; i < 8; ++i) {
            Engine eng(sceneFrame, 13u + (unsigned)i * 101);
            acc = acc + eng.rayColor(r);
        }
        const Vec3 framePix = acc * (1.f / 8.f);
        CHECK(framePix.x > 90.0f);  // emission x = 96.0
        CHECK_NEAR(framePix.x, 96.0f, 1e-6f);
        CHECK_NEAR(framePix.y, 0.18f, 1e-4f);
        CHECK_NEAR(framePix.z, 0.12f, 1e-4f);
    }

    return checkFinish();
}
