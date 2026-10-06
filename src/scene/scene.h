#pragma once
#include <algorithm>
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
    // frame = neon glow (pure emission). M6: the ring's red channel is 8x
    // brighter (4.0 -> 32.0) so its cast on the cube reads as a distinct red
    // glow. M7: the cube is now WHITE GLOSSY PLASTIC (pearl-level F0 = 0.15,
    // sheen roughness 0.30, was 0.04/0.40) -- a reflective surface that
    // mirrors the scene: the ring's cast concentrates into a strong red
    // reflection band across the top face (tone-mapped R-G ~ 30 levels, was
    // ~10) while the faces stay bright white. The ring red is 3x brighter
    // (32.0 -> 96.0, G/B unchanged -- the hue stays a deep saturated red and
    // the frame still tone-maps to a clamped bright red) so that reflection
    // band reads clearly. (True point mirrors of the ring on the cube are
    // geometrically impossible here -- every face's mirror ray points into
    // the void, the ring girdles the cube at its equator -- so the ring's
    // "reflection" is this sheen band; see docs/plan-reflections.md.)
    Material cubeMat = Material::glossyWhite();
    Material frameMat = Material::neon({96.0f, 0.18f, 0.12f});

    // M5: ambient fill light -- a strong, neutral, vertical-gradient light that
    // illuminates the scene's surfaces so the cube reads as a BRIGHT WHITE
    // object. It is a light, not a visible sky: the tracer estimates its
    // contribution by NEE (cosine-sampled, low-variance; also lights the
    // plastic's sheen), while the camera's escape rays return black, because in
    // this empty scene nothing but the cube (and the glowing frame) reflects
    // light -- the void stays black. The gentle zenith->nadir grade gives the
    // cube form shading (bright top, darker base). Neutral on purpose, so the
    // cube reads white and the neon ring stays the only saturated (red) source.
    // M6: scaled to ~0.75x so the 8x-stronger ring light is visible on the
    // cube (bright white top face with a clear red cast) instead of washing
    // it out.
    Vec3 envZenith  = {3.4f, 3.4f, 3.4f}; // dir.y = +1 (straight up)
    Vec3 envHorizon = {2.6f, 2.6f, 2.6f}; // dir.y =  0
    Vec3 envNadir   = {1.9f, 1.9f, 1.9f}; // dir.y = -1 (straight down)

    Vec3 envRadiance(const Vec3& dir) const {
        const float y = std::clamp(dir.y, -1.f, 1.f);
        if (y >= 0.f) return envHorizon + (envZenith - envHorizon) * y;
        return envHorizon + (envNadir - envHorizon) * (-y);
    }

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
