#include "check.h"

#include "math/vec3.h"

int main() {
    Vec3 a(1, 2, 3), b(4, 5, 6);
    CHECK(a + b == Vec3(5, 7, 9));
    CHECK(b - a == Vec3(3, 3, 3));
    CHECK(a * 2.f == Vec3(2, 4, 6));
    CHECK(2.f * a == Vec3(2, 4, 6));
    CHECK(a + b - b == a);
    a += b;
    CHECK(a == Vec3(5, 7, 9));
    a -= b;
    CHECK(a == Vec3(1, 2, 3));

    CHECK_NEAR(Vec3(3, 4, 0).length(), 5.0, 1e-6);
    CHECK_NEAR(Vec3(1, 2, 3).dot(Vec3(4, 5, 6)), 32.0, 1e-6);
    CHECK(Vec3(1, 0, 0).cross(Vec3(0, 1, 0)) == Vec3(0, 0, 1));
    CHECK(Vec3(0, 1, 0).cross(Vec3(1, 0, 0)) == Vec3(0, 0, -1));
    const Vec3 n = Vec3(3, 4, 0).unit();
    CHECK_NEAR(n.x, 0.6, 1e-6);
    CHECK_NEAR(n.y, 0.8, 1e-6);
    CHECK_NEAR(n.length(), 1.0, 1e-6);

    const Ray r(Vec3(0, 0, -5), Vec3(0, 0, 1));
    CHECK(r.at(5.f) == Vec3(0, 0, 0));
    return checkFinish();
}
