#include "check.h"

#include <random>

#include "scene/scene.h"

int main() {
    const Scene scene;
    CHECK_NEAR(scene.lightArea, 0.4656f, 1e-4f);
    CHECK(scene.boxes.size() == 5);

    std::mt19937 rng(7);
    int counts[4] = {0, 0, 0, 0};
    for (int i = 0; i < 20000; ++i) {
        const Vec3 p = scene.sampleLightPoint(rng);
        CHECK_NEAR(p.z, Scene::kFrameZ + Scene::kFrameT * 0.5f, 1e-5f);
        const float ax = std::fabs(p.x), ay = std::fabs(p.y);
        CHECK(ax <= 1.0001f && ay <= 1.0001f);
        CHECK(!((ax < 0.9399f) && (ay < 0.9399f)));
        if (p.y < -0.9399f) counts[0]++;
        else if (p.y > 0.9399f) counts[1]++;
        else if (p.x < -0.0601f) counts[2]++;
        else if (p.x > 0.0601f) counts[3]++;
        else CHECK(false);
    }
    CHECK(counts[0] > 1000);
    CHECK(counts[1] > 1000);
    CHECK(counts[2] > 1000);
    CHECK(counts[3] > 1000);
    return checkFinish();
}
