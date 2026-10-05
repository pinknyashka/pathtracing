#pragma once
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "../math/vec3.h"

#ifdef PATHTRACER_HAS_SDL
#include <SDL.h>
#endif

struct Display {
    static void toneMap(const Vec3& c, std::uint8_t& r, std::uint8_t& g, std::uint8_t& b) {
        auto f = [](float x) -> std::uint8_t {
            x = std::max(0.f, x);
            x = x / (1.f + x);
            int v = (int)(x * 255.f + 0.5f);
            return (std::uint8_t)(v > 255 ? 255 : v);
        };
        r = f(c.x); g = f(c.y); b = f(c.z);
    }

    static void writePPM(const std::string& path, int w, int h, const std::vector<Vec3>& px) {
        std::ofstream f(path, std::ios::binary);
        if (!f) return;
        f << "P6\n" << w << " " << h << "\n255\n";
        std::vector<char> row((size_t)w * 3);
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                std::uint8_t r, g, b;
                toneMap(px[(size_t)y * w + x], r, g, b);
                row[(size_t)x * 3 + 0] = (char)r;
                row[(size_t)x * 3 + 1] = (char)g;
                row[(size_t)x * 3 + 2] = (char)b;
            }
            f.write(row.data(), (std::streamsize)row.size());
        }
    }

#ifdef PATHTRACER_HAS_SDL
    bool openSDL(int w, int h) {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) return false;
        window = SDL_CreateWindow("pathtracer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, 0);
        if (!window) return false;
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) renderer = SDL_CreateRenderer(window, -1, 0);
        if (!renderer) return false;
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
        if (!texture) return false;
        open = true;
        return true;
    }

    void present(const std::vector<Vec3>& px, int w, int h) {
        std::vector<std::uint32_t> buf((size_t)w * h);
        for (size_t i = 0; i < (size_t)w * h; ++i) {
            std::uint8_t r, g, b;
            toneMap(px[i], r, g, b);
            buf[i] = 0xFF000000u | ((std::uint32_t)r << 16) | ((std::uint32_t)g << 8) | b;
        }
        SDL_UpdateTexture(texture, nullptr, buf.data(), w * 4);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    void close() {
        if (texture) SDL_DestroyTexture(texture);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
        open = false;
    }

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    bool open = false;
#endif
};
