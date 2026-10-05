#include "check.h"

#include "scene/geometry.h"

int main() {
    static const Material mat = Material::diffuse({1.f, 1.f, 1.f});
    Box b(Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.5f, 0.5f, 0.5f), mat);
    Hit h;

    CHECK(b.intersect(Ray(Vec3(0, 0, 5), Vec3(0, 0, -1)), 0.f, 1e30f, h));
    CHECK_NEAR(h.t, 4.5f, 1e-4f);
    CHECK(h.normal == Vec3(0, 0, 1));
    CHECK(h.point == Vec3(0, 0, 0.5f));
    CHECK(h.material == &mat);

    CHECK(b.intersect(Ray(Vec3(0, 0, -5), Vec3(0, 0, 1)), 0.f, 1e30f, h));
    CHECK_NEAR(h.t, 4.5f, 1e-4f);
    CHECK(h.normal == Vec3(0, 0, -1));

    CHECK(b.intersect(Ray(Vec3(5, 0, 0), Vec3(-1, 0, 0)), 0.f, 1e30f, h));
    CHECK(h.normal == Vec3(1, 0, 0));
    CHECK_NEAR(h.t, 4.5f, 1e-4f);

    CHECK(b.intersect(Ray(Vec3(-5, 0, 0), Vec3(1, 0, 0)), 0.f, 1e30f, h));
    CHECK(h.normal == Vec3(-1, 0, 0));

    CHECK(!b.intersect(Ray(Vec3(5, 0, 5), Vec3(1, 0, 1)), 0.f, 1e30f, h));
    CHECK(!b.intersect(Ray(Vec3(0, 0, -5), Vec3(0, 0, -1)), 0.f, 1e30f, h));
    CHECK(!b.intersect(Ray(Vec3(0, 0, 5), Vec3(0, 0, 1)), 0.f, 1e30f, h));
    // at t=5 the ray position is (0,0,0), inside the box: the segment intersects,
    // exiting through the -z face at t=5.5
    CHECK(b.intersect(Ray(Vec3(0, 0, 5), Vec3(0, 0, -1)), 5.f, 1e30f, h));
    CHECK_NEAR(h.t, 5.5f, 1e-4f);
    CHECK(h.normal == Vec3(0, 0, -1));

    CHECK(b.intersect(Ray(Vec3(0, 0, 0), Vec3(0, 0, 1)), 0.01f, 1e30f, h));
    CHECK_NEAR(h.t, 0.5f, 1e-4f);
    CHECK(h.normal == Vec3(0, 0, 1));

    return checkFinish();
}
