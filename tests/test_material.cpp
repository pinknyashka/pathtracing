#include "check.h"

#include <random>

#include "math/vec3.h"
#include "render/tracer.h"
#include "scene/material.h"

// Fixed-seed uniform-hemisphere Monte-Carlo of the GGX NDF:
// integral over the hemisphere of D dOmega = 1. A direction with cosTheta
// uniform in [0,1) and phi uniform in [0,2pi) is uniform on the hemisphere,
// so the estimator is the sample mean of D * 2pi.
static double ggxNdfIntegral(float alpha, int samples, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> uni(0.f, 1.f);
    double sum = 0.0;
    for (int i = 0; i < samples; ++i) {
        const float c = uni(rng);  // cos(theta), uniform in [0,1)
        sum += (double)(2.0 * kPi) * (double)ggxD(c, alpha);
    }
    return sum / samples;
}

int main() {
    // --- GGX NDF: normalization + shape ------------------------------------
    CHECK_NEAR(ggxNdfIntegral(0.4f, 2000000, 42), 1.0, 0.05);
    CHECK_NEAR(ggxNdfIntegral(0.1f, 2000000, 43), 1.0, 0.10);
    // D is monotone decreasing in cosH for alpha < 1 (grazing lobe of the
    // Trowbridge-Reitz distribution).
    CHECK(ggxD(0.1f, 0.4f) > ggxD(0.5f, 0.4f));
    CHECK(ggxD(0.5f, 0.4f) > ggxD(0.9f, 0.4f));
    // Rougher microfacets spread the (normalized) lobe wider: the peak at
    // normal incidence drops as alpha grows.
    CHECK(ggxD(1.f, 0.4f) > ggxD(1.f, 0.1f));

    // --- Fresnel-Schlick -----------------------------------------------------
    const Vec3 f0(0.04f, 0.04f, 0.04f);
    {
        const Vec3 fn = schlickF(f0, 1.0f);  // normal incidence -> F0
        CHECK_NEAR(fn.x, 0.04f, 1e-5f);
        CHECK_NEAR(fn.y, 0.04f, 1e-5f);
        CHECK_NEAR(fn.z, 0.04f, 1e-5f);
        const Vec3 fg = schlickF(f0, 0.0f);  // grazing -> 1
        CHECK_NEAR(fg.x, 1.0f, 1e-5f);
        CHECK_NEAR(fg.y, 1.0f, 1e-5f);
        CHECK_NEAR(fg.z, 1.0f, 1e-5f);
        const Vec3 fm = schlickF(f0, 0.25f);
        CHECK(fm.x > 0.04f && fm.x < 1.0f);
        CHECK(fm.y > 0.04f && fm.y < 1.0f);
        CHECK(fm.z > 0.04f && fm.z < 1.0f);
    }
    // Energy split: F in [f0, 1] and (1 - F) in [0, 1] at every angle.
    for (const float c : {1.0f, 0.9f, 0.5f, 0.1f, 0.0f}) {
        const Vec3 F = schlickF(f0, c);
        const Vec3 oneMinusF = Vec3(1.f, 1.f, 1.f) - F;
        CHECK(F.x >= 0.04f && F.x <= 1.0f);
        CHECK(F.y >= 0.04f && F.y <= 1.0f);
        CHECK(F.z >= 0.04f && F.z <= 1.0f);
        CHECK(oneMinusF.x >= 0.f && oneMinusF.x <= 1.f);
        CHECK(oneMinusF.y >= 0.f && oneMinusF.y <= 1.f);
        CHECK(oneMinusF.z >= 0.f && oneMinusF.z <= 1.f);
    }

    // --- Smith G1 ------------------------------------------------------------
    CHECK(ggxG1(1.0f, 0.4f) > 0.5f && ggxG1(1.0f, 0.4f) < 1.001f);
    CHECK_NEAR(ggxG1(1.0f, 0.4f), 1.0f, 1e-5f);  // unmasked at normal incidence
    CHECK(ggxG1(0.3f, 0.4f) < ggxG1(0.9f, 0.4f));  // more masking at grazing
    CHECK(ggxG1(0.f, 0.4f) == 0.0f);

    // --- combined BRDF -------------------------------------------------------
    const Vec3 n(0.f, 0.f, 1.f);
    const Vec3 wo = Vec3(0.f, 0.3f, 0.95f).unit();  // wo.n > 0
    const Vec3 li = Vec3(0.f, 0.f, 1.f);            // li.n > 0

    // Diffuse-only material: exact isotropic Lambert (f0 = 0 -> F_v = 0, so
    // the full albedo/pi weight remains and the specular branch is off).
    {
        const Material m = Material::diffuse(Vec3(0.9f, 0.9f, 0.9f));
        const Vec3 b = combinedBrdf(m, wo, li, n);
        CHECK_NEAR(b.x, 0.9f / kPi, 1e-5f);
        CHECK_NEAR(b.y, 0.9f / kPi, 1e-5f);
        CHECK_NEAR(b.z, 0.9f / kPi, 1e-5f);
    }

    // Specular-only material: the evaluated BRDF must equal the reference
    // microfacet expression F_v * F_l * D * G1(cv) * G1(cl) / (4 cv cl).
    {
        Material m;
        m.f0 = f0;
        m.roughness = 0.4f;
        const float cv = std::max(0.f, wo.dot(n));
        const float cl = std::max(0.f, li.dot(n));
        const Vec3 h = (wo + li).unit();
        const float a = ggxAlpha(0.4f);
        const float ref = schlickF(f0, cv).x * schlickF(f0, cl).x
                        * ggxD(std::max(1e-3f, h.dot(n)), a)
                        * ggxG1(cv, a) * ggxG1(cl, a) / (4.f * cv * cl);
        const Vec3 b = combinedBrdf(m, wo, li, n);
        CHECK(b.x > 0.f && std::isfinite(b.x));
        CHECK(b.y > 0.f && std::isfinite(b.y));
        CHECK(b.z > 0.f && std::isfinite(b.z));
        CHECK_NEAR(b.x, ref, 1e-6f);
        CHECK_NEAR(b.y, ref, 1e-6f);
        CHECK_NEAR(b.z, ref, 1e-6f);
    }

    // Split linearity: plastic BRDF = (1 - F_v) * lambert + F_v-weighted
    // specular = (1 - F_v) * (diffuse-only BRDF) + (specular-only BRDF).
    {
        const Material mPlastic = Material::plastic(Vec3(0.9f, 0.9f, 0.9f), f0, 0.4f);
        const Material mDiff = Material::diffuse(Vec3(0.9f, 0.9f, 0.9f));
        Material mSpec;
        mSpec.f0 = f0;
        mSpec.roughness = 0.4f;
        const Vec3 Fv = schlickF(f0, std::max(0.f, wo.dot(n)));
        const Vec3 ref = (Vec3(1.f, 1.f, 1.f) - Fv) * combinedBrdf(mDiff, wo, li, n)
                       + combinedBrdf(mSpec, wo, li, n);
        const Vec3 b = combinedBrdf(mPlastic, wo, li, n);
        CHECK_NEAR(b.x, ref.x, 1e-6f);
        CHECK_NEAR(b.y, ref.y, 1e-6f);
        CHECK_NEAR(b.z, ref.z, 1e-6f);
    }

    // Neon material: pure emission, no scattering, exact BRDF zero.
    {
        const Material m = Material::neon(Vec3(4.0f, 0.18f, 0.12f));
        CHECK(m.isEmissive());
        CHECK(!m.isDiffuse());
        CHECK(!m.isSpecular());
        const Vec3 b = combinedBrdf(m, wo, li, n);
        CHECK(b == Vec3(0.f, 0.f, 0.f));
    }

    // --- presets -------------------------------------------------------------
    {
        const Material p = Material::plastic(Vec3(0.9f, 0.9f, 0.9f), f0, 0.4f);
        CHECK(p.albedo == Vec3(0.9f, 0.9f, 0.9f));
        CHECK(p.f0 == f0);
        CHECK(p.roughness == 0.4f);
        CHECK(p.emission == Vec3(0.f, 0.f, 0.f));
        CHECK(p.isDiffuse());
        CHECK(p.isSpecular());
        CHECK(!p.isEmissive());

        const Material e = Material::neon(Vec3(4.0f, 0.18f, 0.12f));
        CHECK(e.emission == Vec3(4.0f, 0.18f, 0.12f));
        CHECK(e.albedo == Vec3(0.f, 0.f, 0.f));
        CHECK(e.f0 == Vec3(0.f, 0.f, 0.f));
        CHECK(e.roughness == 0.f);
        CHECK(e.isEmissive());
        CHECK(!e.isDiffuse());
        CHECK(!e.isSpecular());

        const Material d = Material::diffuse(Vec3(0.5f, 0.5f, 0.5f));
        CHECK(d.albedo == Vec3(0.5f, 0.5f, 0.5f));
        CHECK(d.f0 == Vec3(0.f, 0.f, 0.f));
        CHECK(d.emission == Vec3(0.f, 0.f, 0.f));
        CHECK(d.isDiffuse());
        CHECK(!d.isSpecular());
        CHECK(!d.isEmissive());
    }

    return checkFinish();
}
