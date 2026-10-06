#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

#include "../math/vec3.h"
#include "../scene/camera.h"
#include "../scene/scene.h"

// --- GGX microfacet BRDF helpers (M4) -------------------------------------

// Schlick Fresnel, per channel: f0 + (1 - f0) * (1 - cosT)^5.
static inline Vec3 schlickF(const Vec3& f0, float cosT) {
    const float t = std::pow(std::max(0.f, 1.f - cosT), 5.f);
    return f0 + (Vec3(1.f, 1.f, 1.f) - f0) * t;
}

// GGX (Trowbridge-Reitz) NDF, normalized to integrate to 1 over the
// hemisphere (the raw TR formula integrates to Z = 1 + atan(sqrt((1-a2)/a2))/
// sqrt(a2(1-a2)) > 1; the normalization matters for the NEE term, which uses
// the BRDF as an absolute value -- bounce sampling is insensitive to it, the
// D cancels in the brdf/pdf ratio and the sampling CDF shape is unchanged).
static inline float ggxD(float cosH, float alpha) {
    const float a2 = alpha * alpha;
    const float d = cosH * cosH * (1.f - a2) + a2;
    const float un = a2 / (kPi * d * d);
    if (a2 >= 0.99f) return un;
    const float z = 1.f + std::atan(std::sqrt((1.f - a2) / a2)) / std::sqrt(a2 * (1.f - a2));
    return un / z;
}

// Smith G1 (Walter et al. 2007, explicit), valid on both sides of alpha = 1.
static inline float ggxG1(float cosT, float alpha) {
    if (cosT <= 0.f) return 0.f;
    const float a2 = alpha * alpha;
    const float lam = (alpha < 1.f)
        ? (std::sqrt((a2 - 1.f) * cosT * cosT + 1.f) - alpha * cosT) / cosT
        : (std::sqrt((a2 - 1.f) * cosT * cosT + a2) - 1.f) / (a2 * cosT);
    return 1.f / (1.f + lam);
}

// Roughness range where the GGX direction sampler below is well-behaved.
static inline float ggxAlpha(float roughness) {
    return std::clamp(roughness, 1e-3f, 0.99f);
}

// Combined microfacet BRDF: F_v * brdf_ggx + (1 - F_v) * brdf_lambert,
// with F_v = Schlick at the view angle; brdf_ggx carries its own Fresnel
// (Schlick at the light angle). Gray f0 keeps every term a gray scalar.
static inline Vec3 combinedBrdf(const Material& m, const Vec3& wo, const Vec3& li, const Vec3& n) {
    const float cv = std::max(0.f, wo.dot(n));
    const float cl = std::max(0.f, li.dot(n));
    // F_v is exactly zero without a specular lobe, so a pure-diffuse surface
    // keeps its full albedo/pi weight even at grazing view angles.
    const Vec3 Fv = m.isSpecular() ? schlickF(m.f0, cv) : Vec3(0.f, 0.f, 0.f);
    Vec3 brdf{0, 0, 0};
    if (m.isDiffuse())
        brdf = brdf + (Vec3(1.f, 1.f, 1.f) - Fv) * (m.albedo * (1.f / kPi));
    if (m.isSpecular() && cv > 0.f && cl > 0.f) {
        const Vec3 h = (wo + li).unit();
        const float ch = std::max(1e-3f, h.dot(n));
        const float a = ggxAlpha(m.roughness);
        const float ds = ggxD(ch, a) * ggxG1(cv, a) * ggxG1(cl, a) / (4.f * cv * cl);
        brdf = brdf + Fv * (schlickF(m.f0, cl) * ds);
    }
    return brdf;
}

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

            if (hit.material->isEmissive()) {
                if (bounce == 0)
                    acc = acc + scale * hit.material->emission;
                break;
            }

            // Next-event estimation with the full combined BRDF: both the
            // diffuse term and the GGX sheen are lit by the ring in one shot
            // (bounce rays that reach the frame add nothing, so NEE is the
            // only ring->surface estimator).
            const Vec3 wo = r.dir * (-1.f);
            Vec3 nee{0, 0, 0};
            for (int k = 0; k < kVisSamples; ++k) {
                const Vec3 lp = scene.sampleLightPoint(rng);
                const Vec3 toL = lp - hit.point;
                const float dist = toL.length();
                const Vec3 li = toL * (1.f / dist);
                const float cosL = hit.normal.dot(li);
                if (cosL <= 0.f) continue;
                const float cosEmit = (-li).dot(scene.lightNormal);
                if (cosEmit <= 0.f) continue;
                const Ray vis{hit.point + hit.normal * 1e-3f, li};
                bool blocked = false;
                for (const auto& b : scene.boxes) {
                    if (b->mat->isEmissive()) continue;
                    Hit h;
                    if (b->intersect(vis, 1e-3f, dist - 1e-3f, h)) { blocked = true; break; }
                }
                if (!blocked)
                    nee = nee + scene.frameMat.emission
                         * combinedBrdf(*hit.material, wo, li, hit.normal)
                         * cosL * cosEmit / (dist * dist);
            }
            nee = nee * (scene.lightArea / (float)kVisSamples);
            acc = acc + scale * nee;

            if (bounce >= 2) {
                const float p = std::min(0.95f, std::max(hit.material->albedo.maxComponent(), 0.05f));
                std::uniform_real_distribution<float> uni(0.f, 1.f);
                if (uni(rng) > p) break;
                scale = scale * (1.f / p);
            }

            const Scatter s = sampleScatter(hit, wo);
            scale = scale * (s.brdf / s.pdf);
            r = Ray{hit.point + hit.normal * 1e-3f, s.dir};
        }
        return acc;
    }

private:
    struct Scatter {
        Vec3 dir{0, 0, 1};
        float pdf = 1.f;
        Vec3 brdf{0, 0, 0};
    };

    Vec3 cosineDir(const Hit& h) const {
        const Vec3& n = h.normal;
        const Vec3 ref = std::fabs(n.z) < 0.9f ? Vec3(0, 0, 1) : Vec3(1, 0, 0);
        const Vec3 t1 = n.cross(ref).unit();
        const Vec3 t2 = n.cross(t1);
        std::uniform_real_distribution<float> uni(0.f, 1.f);
        const float phi = 2.f * kPi * uni(rng);
        const float rr = std::sqrt(uni(rng));
        return n * std::sqrt(1.f - rr * rr) + t1 * (rr * std::cos(phi)) + t2 * (rr * std::sin(phi));
    }

    // Transmissive lobe selection (weight ~ integrated lobe weight): GGX
    // importance sampling for the specular lobe, cosine for the diffuse.
    // Returns the direction with the matching pdf and the evaluated BRDF so
    // the caller pays scale *= brdf / pdf.
    Scatter sampleScatter(const Hit& h, const Vec3& wo) const {
        const Material& m = *h.material;
        const Vec3& n = h.normal;
        const float cv = std::max(0.f, wo.dot(n));
        std::uniform_real_distribution<float> uni(0.f, 1.f);

        const Vec3 Fv = m.isSpecular() ? schlickF(m.f0, cv) : Vec3(0.f, 0.f, 0.f);
        const float wSpec = (m.isSpecular() && cv > 1e-3f) ? Fv.maxComponent() : 0.f;
        const float wDiff = m.isDiffuse() ? (1.f - Fv.maxComponent()) : 0.f;
        const float w = wSpec + wDiff;

        if (w > 0.f && uni(rng) < wSpec / w) {
            const float a = ggxAlpha(m.roughness);
            const Vec3 ref = std::fabs(n.z) < 0.9f ? Vec3(0, 0, 1) : Vec3(1, 0, 0);
            const Vec3 t1 = n.cross(ref).unit();
            const Vec3 t2 = n.cross(t1);
            Vec3 h, li;
            bool ok = false;
            for (int k = 0; k < 4 && !ok; ++k) {
                const float phi = 2.f * kPi * uni(rng);
                const float u2 = uni(rng);
                const float cosT = std::sqrt(std::max(0.f, (1.f - u2) / (u2 * (a * a - 1.f) + 1.f)));
                const float sinT = std::sqrt(std::max(0.f, 1.f - cosT * cosT));
                const Vec3 hm = n * cosT + t1 * (sinT * std::cos(phi)) + t2 * (sinT * std::sin(phi));
                const Vec3 cand = wo * (2.f * wo.dot(hm)) - hm;
                if (cand.dot(n) > 1e-3f) { h = hm; li = cand; ok = true; }
            }
            if (ok) {
                const float ch = std::max(1e-3f, h.dot(n));
                Scatter s;
                s.dir = li;
                // True pdf of the reflected direction: q(h) = D(h.n) (the
                // normalized NDF is what the CDF above draws from) and
                // dOmega_wi = 4 (wo.h) dOmega_h. No G1 factor: the sampler
                // does not mask, so the ratio brdf/pdf keeps G1v.G1l.Fl.
                s.pdf = ggxD(ch, a) / (4.f * std::max(1e-3f, wo.dot(h)));
                s.brdf = combinedBrdf(m, wo, li, n);
                return s;
            }
            // every reflection landed in the surface: fall back to cosine
        }

        const Vec3 d = cosineDir(h);
        const float c = std::max(1e-4f, d.dot(n));
        Scatter s;
        s.dir = d;
        s.pdf = c / kPi;
        s.brdf = combinedBrdf(m, wo, d, n);
        return s;
    }
};
