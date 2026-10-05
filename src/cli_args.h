#pragma once
#include <cerrno>
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
    bool noAccumulate = false;
    bool sppSet = false;
    double time = -1.0;  // < 0: follow the wall clock
};

static void usage() {
    std::printf(
        "pathtracer [options]\n"
        "  --width N         frame width (default 960)\n"
        "  --height N        frame height (default 540)\n"
        "  --spp N           samples per pixel (default 8; window mode uses 4 unless set)\n"
        "  --frames N        frames to render, 0 = run until closed/Ctrl+C (default 0)\n"
        "  --seed N          RNG seed (default 12345)\n"
        "  --out DIR         write PPM frames to DIR instead of the window\n"
        "  --time T          freeze the animation at wall time T seconds (default: wall clock)\n"
        "  --accumulate      temporal exponential blend to smooth noise\n"
        "  --no-accumulate   disable the blend (window mode blends by default)\n"
        "  --help            this text\n");
}

// Parses a full, in-range base-10 integer (no trailing junk, no overflow).
static bool parseLong(const char* s, long& out) {
    if (!s || !*s) return false;
    errno = 0;
    char* end = nullptr;
    const long v = std::strtol(s, &end, 10);
    if (errno == ERANGE || end == s || *end != '\0') return false;
    out = v;
    return true;
}

static bool valueArg(const char* opt, const char* v, long& out) {
    if (!parseLong(v, out)) {
        std::fprintf(stderr, "invalid value for %s: %s\n", opt, v);
        return false;
    }
    return true;
}

// Parses a full, in-range base-10 number (no trailing junk, no overflow).
static bool parseDouble(const char* s, double& out) {
    if (!s || !*s) return false;
    errno = 0;
    char* end = nullptr;
    const double v = std::strtod(s, &end);
    if (errno == ERANGE || end == s || *end != '\0') return false;
    out = v;
    return true;
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
        long n = 0;
        if (s == "--width") { v = value(); if (!v) return false; if (!valueArg(s.c_str(), v, n)) return false; a.width = (int)n; }
        else if (s == "--height") { v = value(); if (!v) return false; if (!valueArg(s.c_str(), v, n)) return false; a.height = (int)n; }
        else if (s == "--spp") { v = value(); if (!v) return false; if (!valueArg(s.c_str(), v, n)) return false; a.spp = (int)n; a.sppSet = true; }
        else if (s == "--frames") { v = value(); if (!v) return false; if (!valueArg(s.c_str(), v, n)) return false; a.frames = (int)n; }
        else if (s == "--seed") {
            v = value(); if (!v) return false;
            if (!valueArg(s.c_str(), v, n)) return false;
            if (n < 0) { std::fprintf(stderr, "seed must be non-negative\n"); return false; }
            a.seed = (unsigned)n;
        }
        else if (s == "--out") { v = value(); if (!v) return false; a.out = v; }
        else if (s == "--time") {
            v = value(); if (!v) return false;
            if (!parseDouble(v, a.time)) {
                std::fprintf(stderr, "invalid value for --time: %s\n", v);
                return false;
            }
            if (a.time < 0.0) { std::fprintf(stderr, "time must be non-negative\n"); return false; }
        }
        else if (s == "--accumulate") a.accumulate = true;
        else if (s == "--no-accumulate") a.noAccumulate = true;
        else if (s == "--help" || s == "-h") { usage(); std::exit(0); }
        else { std::fprintf(stderr, "unknown argument: %s\n", s.c_str()); return false; }
    }
    if (a.accumulate && a.noAccumulate) { std::fprintf(stderr, "--accumulate and --no-accumulate conflict\n"); return false; }
    if (a.width < 4 || a.height < 4) { std::fprintf(stderr, "width/height too small\n"); return false; }
    if (a.spp < 1) a.spp = 1;
    if (a.frames < 0) a.frames = 0;
    return true;
}
