#include "check.h"

#include "scene/camera.h"

int main() {
    const Camera cam(Vec3(5, 2, 0), Vec3(0, 0, 0), Vec3(0, 1, 0), 1.5f, 0.9f);
    const Vec3 fwd = (Vec3(0, 0, 0) - Vec3(5, 2, 0)).unit();
    CHECK_NEAR(cam.w.dot(fwd), 1.0, 1e-5);
    CHECK_NEAR(cam.u.dot(cam.w), 0.0, 1e-5);
    CHECK_NEAR(cam.v.dot(cam.w), 0.0, 1e-5);
    CHECK_NEAR(cam.u.dot(cam.v), 0.0, 1e-5);
    CHECK_NEAR(cam.u.length(), 1.0, 1e-5);
    CHECK_NEAR(cam.v.length(), 1.0, 1e-5);

    const Ray c = cam.ray(0.5f, 0.5f);
    CHECK_NEAR(c.dir.dot(fwd), 1.0, 1e-4);
    const float dist = (Vec3(5, 2, 0) - Vec3(0, 0, 0)).length();
    const Vec3 p = c.origin + c.dir * dist;
    CHECK_NEAR(p.length(), 0.0, 1e-3);

    const Camera cz(Vec3(0, 0, 7), Vec3(0, 0, 0), Vec3(0, 1, 0), 1.f, 0.9f);
    CHECK_NEAR(cz.v.dot(Vec3(0, 1, 0)), 1.0, 1e-5);
    CHECK_NEAR(cz.u.dot(Vec3(1, 0, 0)), 1.0, 1e-5);

    const Camera o = Camera::orbit(0.f, 5.f, 2.f, 0.9f, 1.5f);
    CHECK_NEAR(o.origin.x, 5.0, 1e-4);
    CHECK_NEAR(o.origin.y, 2.0, 1e-4);
    const Camera o2 = Camera::orbit(kPi, 5.f, 2.f, 0.9f, 1.5f);
    CHECK_NEAR(o2.origin.x, -5.0, 1e-4);

    return checkFinish();
}
