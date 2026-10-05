#pragma once
#include <cmath>

#include "../math/vec3.h"

class Camera {
public:
    Vec3 origin{0, 0, 0};
    Vec3 u{1, 0, 0}, v{0, 1, 0}, w{0, 0, -1};
    float focal = 1.f;
    float aspect = 1.f;

    Camera() = default;

    Camera(const Vec3& pos, const Vec3& target, const Vec3& up, float aspect_, float fovYRad)
        : origin(pos), aspect(aspect_) {
        w = (target - pos).unit();
        Vec3 side = w.cross(up);
        if (side.lengthSq() < 1e-8f) side = Vec3(1, 0, 0);
        u = side.unit();
        v = u.cross(w);
        focal = 1.f / std::tan(0.5f * fovYRad);
    }

    Ray ray(float s, float t) const {
        Vec3 p = origin
              + w * focal
              + u * (aspect * (2.f * s - 1.f))
              + v * (2.f * t - 1.f);
        return Ray{origin, (p - origin).unit()};
    }

    static Camera orbit(float angle, float radius, float height, float fovYRad, float aspect) {
        Vec3 pos(radius * std::cos(angle), height, radius * std::sin(angle));
        return Camera(pos, Vec3(0, 0.1f, 0), Vec3(0, 1, 0), aspect, fovYRad);
    }
};
