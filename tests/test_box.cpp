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

    // ---- Rotated / translated boxes (M2 rigid-transform API) ----
    // Convention: world = R*local + T. Mat3 stores columns;
    // Mat3::rotY(a) = [c 0 s; 0 1 0; -s 0 c], i.e. R*(x,y,z) = (cx+sz, y, -sx+cz),
    // and the inverse (transpose) is (cx-sz, y, sx+cz).

    // Y-90 (rotY(pi/2)): R*(x,y,z) = (z, y, -x). The box's local -x face (normal
    // (-1,0,0)) now faces world +z, since R*(-1,0,0) = (0,0,1).
    // Ray (0,0,5) dir (0,0,-1): local ray = R^T*(world) = from (-5,0,0) along
    // (1,0,0) -> enters the local -x face at t=4.5 (t is preserved by the rigid
    // transform). point_w = R*(-0.5,0,0) = (0,0,0.5); normal_w = (0,0,1).
    {
        Box b90(Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.5f, 0.5f, 0.5f), mat);
        b90.R = Mat3::rotY(0.5f * kPi);
        CHECK(b90.intersect(Ray(Vec3(0, 0, 5), Vec3(0, 0, -1)), 0.f, 1e30f, h));
        CHECK_NEAR(h.t, 4.5f, 1e-4f);
        CHECK_NEAR(h.point.x, 0.f, 1e-4f);
        CHECK_NEAR(h.point.y, 0.f, 1e-4f);
        CHECK_NEAR(h.point.z, 0.5f, 1e-4f);
        CHECK_NEAR(h.normal.x, 0.f, 1e-4f);
        CHECK_NEAR(h.normal.y, 0.f, 1e-4f);
        CHECK_NEAR(h.normal.z, 1.f, 1e-4f);
        CHECK(h.material == &mat);
    }

    // Y-45: the xz cross-section of the rotated box is a diamond |x|+|z| <= sqrt(2)/2
    // (half-diagonal sqrt(2)/2 ~ 0.7071 at the axis). Hand derivation for the ray
    // (0.6, 0, -5) dir (0,0,1): with h = cos(45deg) = sqrt(2)/2, local ray is
    // from (5.6h, 0, -4.4h) along (-h, 0, h). The local +x slab entry is
    // t = (0.5 - 5.6h)/(-h) = 5.6 - h, which beats the local +z slab entry
    // (4.4 - 0.5/h = 3.69...), so the hit is the local +x face:
    //   t      = 5.6 - h                       ~ 4.89289
    //   point  = (0.6, 0, -5 + t) = (0.6, 0, 0.6 - h) ~ (0.6, 0, -0.10711)
    //   normal = R*(1,0,0) = (h, 0, -h)
    // This ray *misses* the unrotated box (x = 0.6 > 0.5): rotation changed the
    // silhouette. A ray at z = 0.9 is outside both the diamond (0.9 > 0.7071) and
    // the AABB, so it misses rotated and unrotated alike.
    {
        const float h45 = std::cos(0.25f * kPi);  // = sin(45deg) = sqrt(2)/2
        Box b45(Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.5f, 0.5f, 0.5f), mat);
        b45.R = Mat3::rotY(0.25f * kPi);
        CHECK(b45.intersect(Ray(Vec3(0.6f, 0, -5), Vec3(0, 0, 1)), 0.f, 1e30f, h));
        CHECK_NEAR(h.t, 5.6f - h45, 1e-4f);
        CHECK_NEAR(h.point.x, 0.6f, 1e-4f);
        CHECK_NEAR(h.point.y, 0.f, 1e-4f);
        CHECK_NEAR(h.point.z, 0.6f - h45, 1e-4f);
        CHECK_NEAR(h.normal.x, h45, 1e-4f);
        CHECK_NEAR(h.normal.y, 0.f, 1e-4f);
        CHECK_NEAR(h.normal.z, -h45, 1e-4f);
        CHECK(!b.intersect(Ray(Vec3(0.6f, 0, -5), Vec3(0, 0, 1)), 0.f, 1e30f, h));

        CHECK(!b45.intersect(Ray(Vec3(4, 0, 0.9f), Vec3(-1, 0, 0)), 0.f, 1e30f, h));
        CHECK(!b.intersect(Ray(Vec3(4, 0, 0.9f), Vec3(-1, 0, 0)), 0.f, 1e30f, h));
    }

    // Z-90 via a hand-built Mat3 (no rotZ helper): columns (c,s,0), (-s,c,0),
    // (0,0,1) with a = pi/2 -> R*(x,y,z) = (-y, x, z). The local +x face normal
    // (1,0,0) maps to world +y. Ray (0,5,0) dir (0,-1,0): local ray = R^T*(world)
    // = from (5,0,0) along (-1,0,0) -> enters the local +x face at t = 4.5;
    // point_w = R*(0.5,0,0) = (0,0.5,0); normal_w = (0,1,0).
    {
        Box bz(Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.5f, 0.5f, 0.5f), mat);
        bz.R = Mat3(Vec3(0.f, 1.f, 0.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 0.f, 1.f));
        CHECK(bz.intersect(Ray(Vec3(0, 5, 0), Vec3(0, -1, 0)), 0.f, 1e30f, h));
        CHECK_NEAR(h.t, 4.5f, 1e-4f);
        CHECK_NEAR(h.point.x, 0.f, 1e-4f);
        CHECK_NEAR(h.point.y, 0.5f, 1e-4f);
        CHECK_NEAR(h.point.z, 0.f, 1e-4f);
        CHECK_NEAR(h.normal.x, 0.f, 1e-4f);
        CHECK_NEAR(h.normal.y, 1.f, 1e-4f);
        CHECK_NEAR(h.normal.z, 0.f, 1e-4f);
    }

    // Explicit identity transform must be hit-for-hit identical to the default
    // (untransformed) box on a couple of asymmetric rays.
    {
        Box bId(Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.5f, 0.5f, 0.5f), mat);
        bId.R = Mat3::identity();
        bId.T = Vec3(0, 0, 0);
        const Ray r1(Vec3(0.2f, -0.3f, 5), Vec3(0, 0, -1));
        const Ray r2(Vec3(-5, 0.1f, -0.2f), Vec3(1, 0, 0));
        Hit ha, hb;
        CHECK(b.intersect(r1, 0.f, 1e30f, ha));
        CHECK(bId.intersect(r1, 0.f, 1e30f, hb));
        CHECK_NEAR(ha.t, hb.t, 1e-5f);
        CHECK_NEAR(ha.point.x, hb.point.x, 1e-5f);
        CHECK_NEAR(ha.point.y, hb.point.y, 1e-5f);
        CHECK_NEAR(ha.point.z, hb.point.z, 1e-5f);
        CHECK_NEAR(ha.normal.x, hb.normal.x, 1e-5f);
        CHECK_NEAR(ha.normal.y, hb.normal.y, 1e-5f);
        CHECK_NEAR(ha.normal.z, hb.normal.z, 1e-5f);
        CHECK(b.intersect(r2, 0.f, 1e30f, ha));
        CHECK(bId.intersect(r2, 0.f, 1e30f, hb));
        CHECK_NEAR(ha.t, hb.t, 1e-5f);
        CHECK_NEAR(ha.point.x, hb.point.x, 1e-5f);
        CHECK_NEAR(ha.point.y, hb.point.y, 1e-5f);
        CHECK_NEAR(ha.point.z, hb.point.z, 1e-5f);
        CHECK_NEAR(ha.normal.x, hb.normal.x, 1e-5f);
        CHECK_NEAR(ha.normal.y, hb.normal.y, 1e-5f);
        CHECK_NEAR(ha.normal.z, hb.normal.z, 1e-5f);
    }

    // Translation alone: T = (2,0,0), identity R. Ray (2,0,5) dir (0,0,-1) hits
    // the +z face at t = 4.5, point (2,0,0.5), normal (0,0,1).
    {
        Box bt(Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.5f, 0.5f, 0.5f), mat);
        bt.T = Vec3(2, 0, 0);
        CHECK(bt.intersect(Ray(Vec3(2, 0, 5), Vec3(0, 0, -1)), 0.f, 1e30f, h));
        CHECK_NEAR(h.t, 4.5f, 1e-4f);
        CHECK_NEAR(h.point.x, 2.f, 1e-4f);
        CHECK_NEAR(h.point.y, 0.f, 1e-4f);
        CHECK_NEAR(h.point.z, 0.5f, 1e-4f);
        CHECK_NEAR(h.normal.z, 1.f, 1e-4f);
    }

    // Combined T + R: T = (2,0,0), R = rotY(pi/2). Ray (2,0,5) dir (0,0,-1):
    // local ray = R^T*((o - T)) = from (-5,0,0) along (1,0,0), same as the Y-90
    // case above -> t = 4.5 on the local -x face; point_w = R*(-0.5,0,0) + T =
    // (2,0,0.5); normal_w = R*(-1,0,0) = (0,0,1). A parallel ray at z = 5 with
    // x = 5 maps to local z = 3 (outside [-0.5,0.5]) and misses.
    {
        Box btr(Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.5f, 0.5f, 0.5f), mat);
        btr.R = Mat3::rotY(0.5f * kPi);
        btr.T = Vec3(2, 0, 0);
        CHECK(btr.intersect(Ray(Vec3(2, 0, 5), Vec3(0, 0, -1)), 0.f, 1e30f, h));
        CHECK_NEAR(h.t, 4.5f, 1e-4f);
        CHECK_NEAR(h.point.x, 2.f, 1e-4f);
        CHECK_NEAR(h.point.y, 0.f, 1e-4f);
        CHECK_NEAR(h.point.z, 0.5f, 1e-4f);
        CHECK_NEAR(h.normal.x, 0.f, 1e-4f);
        CHECK_NEAR(h.normal.y, 0.f, 1e-4f);
        CHECK_NEAR(h.normal.z, 1.f, 1e-4f);
        CHECK(!btr.intersect(Ray(Vec3(5, 0, 5), Vec3(0, 0, -1)), 0.f, 1e30f, h));
    }

    return checkFinish();
}
