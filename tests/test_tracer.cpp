#include "check.h"

#include <algorithm>

#include "render/tracer.h"
#include "scene/camera.h"
#include "scene/scene.h"

static Vec3 average(const Scene& scene, const Camera& cam, float s, float t, int spp, unsigned baseSeed) {
    Vec3 acc(0, 0, 0);
    for (int i = 0; i < spp; ++i) {
        Engine eng(scene, baseSeed + (unsigned)i * 101);
        acc = acc + eng.rayColor(cam.ray(s, t));
    }
    return acc * (1.f / (float)spp);
}

int main() {
    const Scene scene;
    const float fovY = 50.f * kPi / 180.f;
    const Camera camLit(Vec3(0, 0, -7), Vec3(0, 0, 0), Vec3(0, 1, 0), 1.0f, fovY);
    const Camera camShadow(Vec3(0, 0, 7), Vec3(0, 0, 0), Vec3(0, 1, 0), 1.0f, fovY);

    const Vec3 lit = average(scene, camLit, 0.5f, 0.5f, 32, 11);
    CHECK(lit.x > 0.2f);
    CHECK(lit.x > 3.0f * std::max(lit.y, lit.z));

    const Vec3 shadow = average(scene, camShadow, 0.5f, 0.5f, 32, 12);
    CHECK(shadow.lengthSq() < 1e-4f);

    const Vec3 framePix = average(scene, camLit, 0.5f, 0.76f, 8, 13);
    CHECK(framePix.x > 3.0f);

    return checkFinish();
}
