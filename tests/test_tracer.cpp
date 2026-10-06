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
    // Measured anchors (probe, same fresh-Engine-per-ray estimator): 64-spp
    // over 32 seeds x in [0.065, 0.131], mean ~ 0.093 -- the close-range
    // NEE per-ray estimator is heavy-tailed (~17% sd; the cosL*cosEmit/d^2
    // integrand varies ~20x across the top bar); 512-spp 8-seed mean
    // x ~ 0.084. The band below covers the observed estimator range: a
    // broken NEE / light normal / emission drops x to ~0, a halved or
    // doubled contribution lands outside [0.06, 0.14].
    {
        Scene sceneLit;  // ctor leaves the frame at theta = 0
        const Vec3 lit11 = average(sceneLit, cam, 0.5f, 0.54003608f, 64, 11);
        const Vec3 lit211 = average(sceneLit, cam, 0.5f, 0.54003608f, 64, 211);
        for (int i = 0; i < 2; ++i) {
            const Vec3 lit = (i == 0 ? lit11 : lit211);
            CHECK(lit.x > 0.06f);
            CHECK(lit.x < 0.14f);
            // Every radiance term is emission (4.0, 0.18, 0.12) times a
            // channel-independent scalar, so y/x = 0.045 and z/x = 0.03
            // hold exactly (red-dominant; catches a wrong light color).
            CHECK_NEAR(lit.y / lit.x, 0.18f / 4.0f, 1e-3f);
            CHECK_NEAR(lit.z / lit.x, 0.12f / 4.0f, 1e-3f);
        }
    }

    // SHADOW: theta = 0. The center pixel hits the front face (z = 0.5),
    // where every ring point has cosL < 0 (the light lies behind the face),
    // and cosine bounces from that face stay in the +z hemisphere, which can
    // never reach the ring (z <= 0.03) -- the pixel receives exactly zero
    // light (measured: lengthSq == 0.0 for both seeds).
    {
        Scene sceneShadow;  // ctor leaves the frame at theta = 0
        const Vec3 sh12 = average(sceneShadow, cam, 0.5f, 0.5f, 64, 12);
        const Vec3 sh212 = average(sceneShadow, cam, 0.5f, 0.5f, 64, 212);
        CHECK(sh12.lengthSq() < 1e-12f);
        CHECK(sh212.lengthSq() < 1e-12f);
    }

    // FRAME: primary ray aimed straight at the top bar's front-face center
    // (0, 0.97, +0.03) at theta = 0 (the bar's front face is the +z side of
    // the ring plane). The ray passes OVER the cube (at z = 0.5 it is at
    // y ~ 1.024 > 0.5), so the bar is the first hit, and an emissive hit at
    // bounce 0 returns the raw emission (4.0, 0.18, 0.12) with no MC noise
    // (measured exactly).
    {
        Scene sceneFrame;  // ctor leaves the frame at theta = 0
        const Ray r{Vec3(0, 2, 9), (Vec3(0, 0.97f, 0.03f) - Vec3(0, 2, 9)).unit()};
        Vec3 acc(0, 0, 0);
        for (int i = 0; i < 8; ++i) {
            Engine eng(sceneFrame, 13u + (unsigned)i * 101);
            acc = acc + eng.rayColor(r);
        }
        const Vec3 framePix = acc * (1.f / 8.f);
        CHECK(framePix.x > 3.0f);  // emission x = 4.0
        CHECK_NEAR(framePix.x, 4.0f, 1e-6f);
        CHECK_NEAR(framePix.y, 0.18f, 1e-4f);
        CHECK_NEAR(framePix.z, 0.12f, 1e-4f);
    }

    return checkFinish();
}
