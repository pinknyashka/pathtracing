#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include "cli_args.h"
#include "math/vec3.h"
#include "render/display.h"
#include "render/tracer.h"
#include "scene/camera.h"
#include "scene/scene.h"

int main(int argc, char** argv) {
    Args args;
    if (!parseArgs(argc, argv, args)) {
        usage();
        return 2;
    }

    const Scene scene;

    Display display;
    bool haveWindow = false;
    const bool wantPpm = !args.out.empty();
#ifdef PATHTRACER_HAS_SDL
    if (!wantPpm && display.openSDL(args.width, args.height)) haveWindow = true;
#endif
    const bool ppm = !haveWindow;
    const std::string ppmDir = wantPpm ? args.out : "frames";
    if (ppm) {
        std::error_code ec;
        std::filesystem::create_directories(ppmDir, ec);
        std::printf("pathtracer: no window available, writing PPM frames to %s/\n", ppmDir.c_str());
        if (args.frames == 0) {
            std::printf("pathtracer: PPM mode with --frames 0 runs until Ctrl+C (use --frames N for a fixed count)\n");
        }
    }

    const float aspect = (float)args.width / (float)args.height;
    const float fovY = 50.f * kPi / 180.f;
    const float orbitPeriod = 12.f;

    std::vector<Vec3> framePx((size_t)args.width * args.height);
    std::vector<Vec3> accum((size_t)args.width * args.height, Vec3(0, 0, 0));

    const auto tStart = std::chrono::steady_clock::now();
    long long n = 0;
    bool quit = false;

    while (!quit) {
        const auto t0 = std::chrono::steady_clock::now();
        const double tSec = std::chrono::duration<double>(t0 - tStart).count();
        const float angle = (float)(2.0 * kPi * (tSec / orbitPeriod));
        const Camera cam = Camera::orbit(angle, 5.f, 2.f, fovY, aspect);

        for (auto& p : framePx) p = Vec3(0, 0, 0);

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 16)
#endif
        for (int y = 0; y < args.height; ++y) {
            Engine rowEngine(scene, args.seed + (unsigned)(y * 7919 + (n % 1000003) * 104729));
            for (int x = 0; x < args.width; ++x) {
                Vec3 c(0, 0, 0);
                for (int s = 0; s < args.spp; ++s) {
                    c = c + rowEngine.pixelColor(cam, x, y, args.width, args.height);
                }
                c = c * (1.f / (float)args.spp);
                const size_t i = (size_t)(args.height - 1 - y) * args.width + x;
                if (args.accumulate) {
                    c = accum[i] * 0.6f + c * 0.4f;
                    accum[i] = c;
                }
                framePx[i] = c;
            }
        }

#ifdef PATHTRACER_HAS_SDL
        if (haveWindow) {
            display.present(framePx, args.width, args.height);
            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) quit = true;
            }
        } else
#endif
        {
            char name[512];
            std::snprintf(name, sizeof(name), "%s/frame_%04lld.ppm", ppmDir.c_str(), (long long)n + 1);
            display.writePPM(name, args.width, args.height, framePx);
        }

        const double ms =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        std::fprintf(stderr, "frame %lld: %dx%d @ %d spp  %.1f ms (%.1f fps)\n",
                     (long long)n + 1, args.width, args.height, args.spp, ms, ms > 0 ? 1000.0 / ms : 0.0);
        ++n;
        if (args.frames > 0 && (long long)args.frames <= n) break;
    }

#ifdef PATHTRACER_HAS_SDL
    if (haveWindow) display.close();
#endif
    return 0;
}
