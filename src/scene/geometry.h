#pragma once
#include <cmath>

#include "../math/mat3.h"
#include "../math/vec3.h"
#include "material.h"

struct Hit {
    float t = 0.f;
    Vec3 point{0, 0, 0};
    Vec3 normal{0, 0, 1};
    const Material* material = nullptr;
};

class Primitive {
public:
    virtual ~Primitive() = default;
    virtual bool intersect(const Ray& r, float tmin, float tmax, Hit& out) const = 0;
};

class Box : public Primitive {
public:
    Vec3 mn{0, 0, 0}, mx{0, 0, 0};
    const Material* mat = nullptr;
    // Rigid transform (rotation + translation). Identity by default, in which case
    // the box is axis-aligned as in the M1 layout. t is preserved exactly (no scale).
    Mat3 R = Mat3::identity();
    Vec3 T{0, 0, 0};

    Box(const Vec3& min, const Vec3& max, const Material& m) : mn(min), mx(max), mat(&m) {}

    bool intersect(const Ray& r, float tmin, float tmax, Hit& out) const override {
        const Mat3 Rt = R.transpose();
        const Vec3 ol = Rt.mul(r.origin - T);
        const Vec3 dl = Rt.mul(r.dir);
        const float o[3] = {ol.x, ol.y, ol.z};
        const float d[3] = {dl.x, dl.y, dl.z};
        const float lo[3] = {mn.x, mn.y, mn.z};
        const float hi[3] = {mx.x, mx.y, mx.z};

        float t0 = tmin, t1 = tmax;
        int axis = -1;
        int sgn = 0;
        for (int i = 0; i < 3; ++i) {
            if (std::fabs(d[i]) < 1e-6f) {
                if (o[i] < lo[i] || o[i] > hi[i]) return false;
            } else if (d[i] > 0.f) {
                float a = (lo[i] - o[i]) / d[i];
                float b = (hi[i] - o[i]) / d[i];
                if (a > t0) { t0 = a; axis = i; sgn = -1; }
                if (b < t1) t1 = b;
            } else {
                float a = (hi[i] - o[i]) / d[i];
                float b = (lo[i] - o[i]) / d[i];
                if (a > t0) { t0 = a; axis = i; sgn = +1; }
                if (b < t1) t1 = b;
            }
            if (t1 < t0) return false;
        }
        if (t0 < tmin - 1e-5f) return false;

        Vec3 n{0, 0, 0};
        if (axis >= 0) {
            out.t = t0;
            if (axis == 0) n.x = sgn; else if (axis == 1) n.y = sgn; else n.z = sgn;
        } else {
            out.t = t1;
            for (int i = 0; i < 3; ++i) {
                if (std::fabs(d[i]) < 1e-6f) continue;
                float exitT = d[i] > 0.f ? (hi[i] - o[i]) / d[i] : (lo[i] - o[i]) / d[i];
                if (std::fabs(exitT - t1) < 1e-5f) {
                    const float s = d[i] > 0.f ? 1.f : -1.f;
                    if (i == 0) n.x = s; else if (i == 1) n.y = s; else n.z = s;
                    break;
                }
            }
        }
        const Vec3 pl = Vec3(o[0], o[1], o[2]) + Vec3(d[0], d[1], d[2]) * out.t;
        out.point = R.mul(pl) + T;
        out.normal = R.mul(n);
        out.material = mat;
        return true;
    }
};
