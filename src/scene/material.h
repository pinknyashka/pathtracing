#pragma once
#include "../math/vec3.h"

// Unified material: a surface participates in whichever terms are non-zero.
//   diffuse  when albedo != 0   (Lambertian base color)
//   specular when f0 != 0       (GGX microfacet sheen, F0 + roughness)
//   emissive when emission != 0 (neon light; terminates the path)
struct Material {
    Vec3  albedo{0, 0, 0};    // diffuse base color
    Vec3  f0{0, 0, 0};        // specular reflectance (Fresnel F0); (0,0,0) = no specular
    float roughness = 0.f;    // GGX microfacet roughness: 0 = mirror, 1 = fully rough
    Vec3  emission{0, 0, 0};  // emissive radiance (neon); (0,0,0) = not a light

    bool isDiffuse()  const { return albedo.maxComponent() > 0.f; }
    bool isSpecular() const { return f0.maxComponent() > 0.f; }
    bool isEmissive() const { return emission.maxComponent() > 0.f; }

    static Material diffuse(Vec3 a) {
        Material m;
        m.albedo = a;
        return m;
    }
    static Material plastic(Vec3 a, Vec3 f0v, float rough) {
        Material m;
        m.albedo = a;
        m.f0 = f0v;
        m.roughness = rough;
        return m;
    }
    // M7: white glossy plastic -- a pearl-level reflectance (F0 = 0.15, above
    // the dielectric 0.04) with a moderately tight sheen (roughness 0.30), so
    // the surface reads as reflective: it mirrors the environment as a soft
    // highlight and the neon ring as a strong red sheen band.
    static Material glossyWhite() {
        return plastic({0.9f, 0.9f, 0.9f}, {0.15f, 0.15f, 0.15f}, 0.30f);
    }
    static Material neon(Vec3 e) {
        Material m;
        m.emission = e;
        return m;
    }
};
