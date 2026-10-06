#include "check.h"

#include <random>

#include "scene/scene.h"

// M3 layout: the frame ring is centered on the cube center (ring plane through
// the origin, kFrameZ = 0) and pitches about the world X axis. For each test
// angle we verify the light normal, the (rotation-invariant) light area, that
// sampled light points lie on the ring plane and on a bar (not in the hole),
// and that setFrameAngle rotated only the four frame bars.

int main() {
    // lightArea = 2*(2*0.06) + 2*(0.06*1.88) = 0.24 + 0.2256 = 0.4656 (4 bars).
    const float theta[3] = {0.f, 0.5f * kPi, kPi};
    const Vec3 expectedNormal[3] = {Vec3(0, 0, 1), Vec3(0, -1, 0), Vec3(0, 0, -1)};

    for (int i = 0; i < 3; ++i) {
        Scene s;
        s.setFrameAngle(theta[i]);

        // lightNormal = rotX(theta) * (0,0,1) = (0, -sin, cos), hand-computed above.
        CHECK_NEAR(s.lightNormal.x, expectedNormal[i].x, 1e-5f);
        CHECK_NEAR(s.lightNormal.y, expectedNormal[i].y, 1e-5f);
        CHECK_NEAR(s.lightNormal.z, expectedNormal[i].z, 1e-5f);
        // Area is rotation-invariant: the bars only rotate, they do not rescale.
        CHECK_NEAR(s.lightArea, 0.4656f, 1e-4f);

        // 2000 sampled points per angle (distinct seeds per angle).
        // Plane test: lightNormal.dot(p) = (0,0,1).dot(local) = kFrameZ + kFrameT/2
        // for any orthonormal frameR, so it is rotation-invariant in world space.
        // Bar/hole test: done in the frame's LOCAL coordinates (q = frameR^T * p).
        // The ring is centered on the rotation axis (kFrameZ = 0), but the local
        // coordinates are the invariant check for any pitch angle.
        const Mat3 frameRinv = s.frameR.transpose();
        std::mt19937 rng(0xC0FFEEu + 12345u * (unsigned)(i + 1));
        for (int k = 0; k < 2000; ++k) {
            const Vec3 p = s.sampleLightPoint(rng);
            CHECK_NEAR(s.lightNormal.dot(p), Scene::kFrameZ + Scene::kFrameT * 0.5f, 1e-4f);
            const Vec3 q = frameRinv.mul(p);
            const float ax = std::fabs(q.x), ay = std::fabs(q.y);
            CHECK(ax <= 1.0001f && ay <= 1.0001f);
            CHECK(!((ax < 0.9399f) && (ay < 0.9399f)));  // on a bar, not in the hole
        }

        // All four frame bars carry the frame rotation: R * (0,0,1) == lightNormal
        // (Mat3 has no operator==, so compare the action on a probe vector).
        for (int k = 0; k < 4; ++k) {
            const Vec3 n = s.boxes[k]->R.mul(Vec3(0, 0, 1));
            CHECK_NEAR(n.x, s.lightNormal.x, 1e-5f);
            CHECK_NEAR(n.y, s.lightNormal.y, 1e-5f);
            CHECK_NEAR(n.z, s.lightNormal.z, 1e-5f);
            CHECK(s.boxes[k]->T == Vec3(0, 0, 0));  // pitch about the origin, no shift
        }

        // The cube (boxes[4]) keeps the identity transform after setFrameAngle.
        const Vec3 cp = s.boxes[4]->R.mul(Vec3(1, 2, 3));
        CHECK_NEAR(cp.x, 1.f, 1e-6f);
        CHECK_NEAR(cp.y, 2.f, 1e-6f);
        CHECK_NEAR(cp.z, 3.f, 1e-6f);
        CHECK(s.boxes[4]->T == Vec3(0, 0, 0));
    }

    return checkFinish();
}
