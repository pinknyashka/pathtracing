// One-shot PPM region statistics for the smoke check in AGENTS.md.
// Usage: smoke_stats <frame.ppm>
// Region boxes are tuned for the 320x320 orbit layout (camera behind the frame).
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

struct PPM {
    int w = 0, h = 0;
    std::vector<unsigned char> px;
};

static bool nextInt(std::istream& is, int& v) {
    int c;
    do {
        c = is.peek();
        if (c == '#') {
            std::string line;
            std::getline(is, line);
        }
    } while (c == '#');
    if (!(is >> v)) return false;
    return true;
}

static bool loadPPM(const char* path, PPM& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::string magic;
    if (!(f >> magic)) return false;
    if (magic != "P6") return false;
    int w, h, maxval;
    if (!nextInt(f, w) || !nextInt(f, h) || !nextInt(f, maxval)) return false;
    f.get();  // single whitespace after maxval
    out.w = w;
    out.h = h;
    out.px.resize((size_t)w * h * 3);
    f.read(reinterpret_cast<char*>(out.px.data()), (std::streamsize)out.px.size());
    if (f.gcount() != (std::streamsize)out.px.size()) return false;
    if (maxval != 255)
        for (auto& c : out.px) c = (unsigned char)((c * 255) / maxval);
    return true;
}

struct Stat {
    int n = 0, maxR = 0, lit = 0;
    double r = 0, g = 0, b = 0;
};

static Stat region(const PPM& p, int x0, int x1, int y0, int y1) {
    Stat s;
    if (x1 > p.w) x1 = p.w;
    if (y1 > p.h) y1 = p.h;
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x) {
            size_t i = ((size_t)y * p.w + x) * 3;
            int r = p.px[i], g = p.px[i + 1], b = p.px[i + 2];
            s.r += r; s.g += g; s.b += b;
            ++s.n;
            if (r > s.maxR) s.maxR = r;
            if (r >= 4) ++s.lit;
        }
    if (s.n) {
        s.r /= s.n;
        s.g /= s.n;
        s.b /= s.n;
    }
    return s;
}

static void print(const char* name, const Stat& s) {
    std::printf("%-28s n=%6d avgR=%7.2f avgG=%7.2f avgB=%7.2f maxR=%3d lit(r>=4)=%6d\n",
                name, s.n, s.r, s.g, s.b, s.maxR, s.lit);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <frame.ppm>\n", argv[0]);
        return 2;
    }
    PPM p;
    if (!loadPPM(argv[1], p)) {
        std::fprintf(stderr, "failed to load %s\n", argv[1]);
        return 1;
    }
    int brightRed = 0, dark = 0;
    for (size_t i = 0; i < p.px.size(); i += 3) {
        int r = p.px[i], g = p.px[i + 1], b = p.px[i + 2];
        if (r > 150 && g < 100 && b < 100) ++brightRed;
        if (r < 4 && g < 4 && b < 4) ++dark;
    }
    size_t total = p.px.size() / 3;
    std::printf("%s  %dx%d  pixels=%zu\n", argv[1], p.w, p.h, total);
    std::printf("brightRed(r>150,g<100,b<100) = %d  (%.1f%%)\n", brightRed, 100.0 * brightRed / total);
    std::printf("dark(r<4,g<4,b<4)             = %d  (%.1f%%)\n", dark, 100.0 * dark / total);
    print("cube front (x125..195 y148..212)", region(p, 125, 196, 148, 213));
    print("cube top   (x125..195 y128..146)", region(p, 125, 196, 128, 146));
    print("bg lower-left (x20..80 y260..310)", region(p, 20, 81, 260, 311));
    print("bg upper-right(x240..300 y20..80)", region(p, 240, 301, 20, 81));
    return 0;
}
