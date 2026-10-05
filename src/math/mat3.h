#pragma once
#include <cmath>

#include "vec3.h"

// 3x3 orthonormal matrix, stored as column vectors.
struct Mat3 {
    Vec3 c0{1, 0, 0}, c1{0, 1, 0}, c2{0, 0, 1};

    Mat3() = default;
    Mat3(const Vec3& a, const Vec3& b, const Vec3& c) : c0(a), c1(b), c2(c) {}

    Vec3 mul(const Vec3& v) const { return c0 * v.x + c1 * v.y + c2 * v.z; }

    Mat3 transpose() const {
        return Mat3(Vec3(c0.x, c1.x, c2.x), Vec3(c0.y, c1.y, c2.y), Vec3(c0.z, c1.z, c2.z));
    }

    static Mat3 identity() { return Mat3(); }

    // Rotation by a radians about the Y axis (right-handed rule).
    static Mat3 rotY(float a) {
        const float c = std::cos(a), s = std::sin(a);
        return Mat3(Vec3(c, 0, -s), Vec3(0, 1, 0), Vec3(s, 0, c));
    }
};
