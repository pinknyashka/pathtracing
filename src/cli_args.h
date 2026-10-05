#pragma once
#include <cstdio>
#include <cstdlib>
#include <string>

struct Args {
    int width = 960;
    int height = 540;
    int spp = 8;
    int frames = 0;
    unsigned seed = 12345;
    std::string out;
    bool accumulate = false;
};

static void usage() {
    std::printf(
        "pathtracer [options]\n"
        "  --width N     frame width (default 960)\n"
        "  --height N    frame height (default 540)\n"
        "  --spp N       samples per pixel (default 8)\n"
        "  --frames N    frames to render, 0 = run until closed/Ctrl+C (default 0)\n"
        "  --seed N      RNG seed (default 12345)\n"
        "  --out DIR     write PPM frames to DIR instead of the window\n"
        "  --accumulate  temporal exponential blend to smooth noise\n"
        "  --help        this text\n");
}

// Returns false on unknown/malformed arguments. --help prints usage and exits 0.
static bool parseArgs(int argc, char** argv, Args& a) {
    for (int i = 1; i < argc; ++i) {
        const std::string s = argv[i];
        auto value = [&]() -> const char* {
            if (i + 1 >= argc) return nullptr;
            return argv[++i];
        };
        const char* v;
        if (s == "--width") { v = value(); if (!v) return false; a.width = std::atoi(v); }
        else if (s == "--height") { v = value(); if (!v) return false; a.height = std::atoi(v); }
        else if (s == "--spp") { v = value(); if (!v) return false; a.spp = std::atoi(v); }
        else if (s == "--frames") { v = value(); if (!v) return false; a.frames = std::atoi(v); }
        else if (s == "--seed") { v = value(); if (!v) return false; a.seed = (unsigned)std::atoi(v); }
        else if (s == "--out") { v = value(); if (!v) return false; a.out = v; }
        else if (s == "--accumulate") a.accumulate = true;
        else if (s == "--help" || s == "-h") { usage(); std::exit(0); }
        else { std::fprintf(stderr, "unknown argument: %s\n", s.c_str()); return false; }
    }
    if (a.width < 4 || a.height < 4) { std::fprintf(stderr, "width/height too small\n"); return false; }
    if (a.spp < 1) a.spp = 1;
    if (a.frames < 0) a.frames = 0;
    return true;
}
