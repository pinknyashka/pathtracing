#pragma once
#include "../math/vec3.h"

enum class MaterialType { Diffuse, Emissive };

struct Material {
    MaterialType type = MaterialType::Diffuse;
    Vec3 albedo{0, 0, 0};
    Vec3 emission{0, 0, 0};

    static Material diffuse(Vec3 a) {
        Material m;
        m.type = MaterialType::Diffuse;
        m.albedo = a;
        return m;
    }
    static Material emissive(Vec3 e) {
        Material m;
        m.type = MaterialType::Emissive;
        m.emission = e;
        return m;
    }
};
