#include "check.h"

#include <algorithm>

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
    // M2 layout: static camera above the scene looking down at the origin; the
    // 2x2 emissive frame ring rotates about Y and the angle is set explicitly.
    // The center pixel (s = t = 0.5) is the camera's look direction, which
    // crosses the cube's +z face (z = 0.5) at y ~ 0.205, x = 0 -- inside the
    // face and, at theta = pi, through the ring's hole (|y| < 0.94 at z = 3).
    const Camera cam(Vec3(0, 2, 9), Vec3(0, 0.1f, 0), Vec3(0, 1, 0), 1.0f, 50.f * kPi / 180.f);

    // LIT: frame at theta = pi (ring at world z = +3, between camera and cube).
    // The center pixel of the cube's +z face sees the ring directly: NEE
    // contributes the red emission. Measured 64-spp anchors: seed 11 ->
    // x = 0.0608, seed 211 -> x = 0.0601 (seed-to-seed spread ~0.1%), so the
    // bounds sit well inside one order of magnitude of NEE strength: a broken
    // NEE / light normal / frame rotation would drop x to ~0, a halved or
    // doubled contribution would land outside [0.05, 0.07].
    {
        Scene sceneLit;
        sceneLit.setFrameAngle(kPi);
        const Vec3 lit11 = average(sceneLit, cam, 0.5f, 0.5f, 64, 11);
        const Vec3 lit211 = average(sceneLit, cam, 0.5f, 0.5f, 64, 211);
        for (int i = 0; i < 2; ++i) {
            const Vec3 lit = (i == 0 ? lit11 : lit211);
            CHECK(lit.x > 0.05f);
            CHECK(lit.x < 0.07f);
            CHECK(lit.x > 3.0f * std::max(lit.y, lit.z));
        }
    }

    // SHADOW: default theta = 0, ring at z = -3 behind the cube. From the
    // +z face every ring point has cosL <= 0 (it lies behind the face), and
    // cosine bounces from that face stay in the +z hemisphere, so the pixel
    // receives exactly zero light (measured: lengthSq == 0.0 for both seeds).
    {
        Scene sceneShadow;  // ctor leaves the frame at theta = 0
        const Vec3 sh12 = average(sceneShadow, cam, 0.5f, 0.5f, 64, 12);
        const Vec3 sh212 = average(sceneShadow, cam, 0.5f, 0.5f, 64, 212);
        CHECK(sh12.lengthSq() < 1e-12f);
        CHECK(sh212.lengthSq() < 1e-12f);
    }

    // FRAME: primary ray aimed straight at the bottom bar (theta = pi), whose
    // center is at world (0, -0.97, 3) (local (0, -0.97, -3) through rotY(pi)).
    // The bar is the first hit (the cube is at y ~ -2.2 along this ray, far
    // outside it), and an emissive hit at bounce 0 returns the raw emission,
    // so framePix is exactly {4.0, 0.18, 0.12} with no MC noise (measured
    // exactly).
    {
        Scene sceneFrame;
        sceneFrame.setFrameAngle(kPi);
        const Ray r{Vec3(0, 2, 9), (Vec3(0, -0.97f, 3.f) - Vec3(0, 2, 9)).unit()};
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
