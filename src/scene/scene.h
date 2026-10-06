#pragma once
#include <memory>
#include <random>
#include <vector>

#include "../math/mat3.h"
#include "../math/vec3.h"
#include "geometry.h"
#include "material.h"

struct Scene {
    // Ring plane passes through the cube center (frame center == cube center == pivot).
    static constexpr float kFrameZ = 0.0f;
    static constexpr float kFrameHalf = 1.0f;
    static constexpr float kFrameT = 0.06f;

    // M4: cube = white rough plastic (white diffuse + soft GGX sheen, satin);
    // frame = neon glow (pure emission, value unchanged from M3).
    Material cubeMat = Material::plastic({0.9f, 0.9f, 0.9f}, {0.04f, 0.04f, 0.04f}, 0.4f);
    Material frameMat = Material::neon({4.0f, 0.18f, 0.12f});

    std::vector<std::unique_ptr<Box>> boxes;
    float barArea[4] = {0.f, 0.f, 0.f, 0.f};
    float lightArea = 0.f;
    Vec3 lightNormal{0, 0, 1};
    float frameAngle = 0.f;
    Mat3 frameR = Mat3::identity();

    // Pitches the frame (and its light) about the world X axis through the origin
    // (= cube center = frame center). The cube keeps the identity transform.
    // Call on the main thread before rendering a frame; the scene is read-only
    // while the parallel pass runs.
    void setFrameAngle(float a) {
        frameAngle = a;
        frameR = Mat3::rotX(a);
        for (auto& b : boxes) {
            if (b->mat == &frameMat) b->R = frameR;
        }
        lightNormal = frameR.mul(Vec3(0, 0, 1));
    }

    Scene() {
        const float h = kFrameHalf, t = kFrameT;
        const float z0 = kFrameZ - 0.5f * t, z1 = kFrameZ + 0.5f * t;
        const auto addBar = [this, z0, z1](int idx, float x0, float y0, float x1, float y1) {
            boxes.emplace_back(std::make_unique<Box>(Vec3(x0, y0, z0), Vec3(x1, y1, z1), frameMat));
            barArea[idx] = (x1 - x0) * (y1 - y0);
            lightArea += barArea[idx];
        };
        addBar(0, -h, -h, h, -h + t);
        addBar(1, -h, h - t, h, h);
        addBar(2, -h, -h + t, -h + t, h - t);
        addBar(3, h - t, -h + t, h, h - t);

        boxes.emplace_back(std::make_unique<Box>(Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.5f, 0.5f, 0.5f), cubeMat));
        setFrameAngle(0.f);
    }

    Vec3 sampleLightPoint(std::mt19937& rng) const {
        std::uniform_real_distribution<float> uni(0.f, 1.f);
        float pick = uni(rng) * lightArea;
        int bar = 3;
        for (int i = 0; i < 4; ++i) {
            if (pick < barArea[i]) { bar = i; break; }
            pick -= barArea[i];
        }
        const float h = kFrameHalf, t = kFrameT;
        const float u0 = uni(rng), u1 = uni(rng);
        float x, y;
        switch (bar) {
            case 0: x = -h + 2.f * h * u0; y = -h + t * u1; break;
            case 1: x = -h + 2.f * h * u0; y = h - t + t * u1; break;
            case 2: x = -h + t * u0; y = -h + t + (2.f * h - 2.f * t) * u1; break;
            default: x = h - t + t * u0; y = -h + t + (2.f * h - 2.f * t) * u1; break;
        }
        return frameR.mul(Vec3(x, y, kFrameZ + 0.5f * t));
    }
};
