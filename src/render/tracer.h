#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

#include "../math/vec3.h"
#include "../scene/camera.h"
#include "../scene/scene.h"

class Engine {
public:
    static constexpr int kMaxBounces = 8;
    static constexpr int kVisSamples = 2;

    const Scene& scene;
    mutable std::mt19937 rng;

    Engine(const Scene& s, std::uint32_t seed) : scene(s), rng(seed) {}

    Vec3 pixelColor(const Camera& cam, int x, int y, int width, int height) {
        std::uniform_real_distribution<float> uni(0.f, 1.f);
        const float s = (x + uni(rng)) / (float)width;
        const float t = (y + uni(rng)) / (float)height;
        return rayColor(cam.ray(s, t));
    }

    Vec3 rayColor(const Ray& primary) {
        Vec3 acc{0, 0, 0};
        Vec3 scale{1, 1, 1};
        Ray r = primary;

        for (int bounce = 0; bounce < kMaxBounces; ++bounce) {
            Hit hit;
            bool any = false;
            float tBest = 1e30f;
            for (const auto& b : scene.boxes) {
                Hit h;
                if (b->intersect(r, 1e-3f, 1e30f, h) && h.t < tBest) {
                    hit = h;
                    tBest = h.t;
                    any = true;
                }
            }
            if (!any) break;

            if (hit.material->type == MaterialType::Emissive) {
                if (bounce == 0)
                    acc = acc + scale * hit.material->emission;
                break;
            }

            const Vec3 albedo = hit.material->albedo;
            const float invPdf = scene.lightArea;
            Vec3 nee{0, 0, 0};
            for (int k = 0; k < kVisSamples; ++k) {
                const Vec3 lp = scene.sampleLightPoint(rng);
                const Vec3 toL = lp - hit.point;
                const float dist = toL.length();
                const Vec3 ld = toL * (1.f / dist);
                const float cosL = hit.normal.dot(ld);
                if (cosL <= 0.f) continue;
                const float cosEmit = (-ld).dot(scene.lightNormal);
                if (cosEmit <= 0.f) continue;
                const Ray vis{hit.point + hit.normal * 1e-3f, ld};
                bool blocked = false;
                for (const auto& b : scene.boxes) {
                    if (b->mat->type == MaterialType::Emissive) continue;
                    Hit h;
                    if (b->intersect(vis, 1e-3f, dist - 1e-3f, h)) { blocked = true; break; }
                }
                if (!blocked) {
                    nee = nee + scene.frameMat.emission * cosL * cosEmit * (1.f / (dist * dist));
                }
            }
            nee = nee * (invPdf / (kPi * (float)kVisSamples));
            acc = acc + scale * (albedo * nee);

            if (bounce >= 2) {
                const float p = std::min(0.95f, std::max(albedo.maxComponent(), 0.05f));
                std::uniform_real_distribution<float> uni(0.f, 1.f);
                if (uni(rng) > p) break;
                scale = scale * (1.f / p);
            }
            scale = scale * albedo;
            r = cosineBounce(hit);
        }
        return acc;
    }

private:
    Ray cosineBounce(const Hit& h) const {
        const Vec3& n = h.normal;
        const Vec3 ref = std::fabs(n.z) < 0.9f ? Vec3(0, 0, 1) : Vec3(1, 0, 0);
        const Vec3 t1 = n.cross(ref).unit();
        const Vec3 t2 = n.cross(t1);
        std::uniform_real_distribution<float> uni(0.f, 1.f);
        const float phi = 2.f * kPi * uni(rng);
        const float rr = std::sqrt(uni(rng));
        const Vec3 d = n * std::sqrt(1.f - rr * rr) + t1 * (rr * std::cos(phi)) + t2 * (rr * std::sin(phi));
        return Ray{h.point + n * 1e-3f, d};
    }
};
