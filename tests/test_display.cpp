#include "check.h"

#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "math/vec3.h"
#include "render/display.h"

// Minimal P6 reader for the round-trip check: parses "P6\n<w> <h>\n<maxval>\n"
// and returns all bytes after the header as the pixel data.
static bool readP6(const std::string& path, int& w, int& h, int& maxval,
                   std::vector<std::uint8_t>& data) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::string all((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (all.size() < 2 || all[0] != 'P' || all[1] != '6') return false;
    size_t i = 2;
    auto nextInt = [&](long& v) -> bool {
        while (i < all.size() && std::isspace((unsigned char)all[i])) ++i;
        v = 0;
        bool any = false;
        while (i < all.size() && std::isdigit((unsigned char)all[i])) {
            v = v * 10 + (all[i] - '0');
            ++i;
            any = true;
        }
        return any;
    };
    long lw = 0, lh = 0, lm = 0;
    if (!nextInt(lw) || !nextInt(lh) || !nextInt(lm)) return false;
    // A PPM header ends with a single whitespace character; data follows it.
    if (i >= all.size() || !std::isspace((unsigned char)all[i])) return false;
    ++i;
    w = (int)lw;
    h = (int)lh;
    maxval = (int)lm;
    data.assign(all.begin() + (std::ptrdiff_t)i, all.end());
    return true;
}

int main() {
    // 1. toneMap: Reinhard basics (clamp, midpoint, saturation).
    {
        std::uint8_t r, g, b;

        Display::toneMap(Vec3(0, 0, 0), r, g, b);
        CHECK(r == 0 && g == 0 && b == 0);

        Display::toneMap(Vec3(-5, 0, 0), r, g, b);
        CHECK(r == 0);  // negative components clamp to black

        Display::toneMap(Vec3(1, 1, 1), r, g, b);
        CHECK(r == 128 && g == 128 && b == 128);  // 1/(1+1) = 0.5 -> 127.5 -> 128

        Display::toneMap(Vec3(1e6f, 2, 3), r, g, b);
        CHECK(r == 255);            // saturates at white
        CHECK(g < 255 && b < 255);  // finite, below saturation
    }

    // 2. toneMap: the red channel is non-decreasing in input intensity.
    {
        const float a[] = {0.1f, 0.5f, 1.f, 2.f, 5.f};
        std::uint8_t prev = 0, r, g, b;
        for (float v : a) {
            Display::toneMap(Vec3(v, 0, 0), r, g, b);
            CHECK(r >= prev);
            prev = r;
        }
    }

    // 3. writePPM: the file round-trips byte-for-byte as the tone-mapped output.
    {
        // 4x3 row-major buffer of known HDR values.
        const std::vector<Vec3> px = {
            Vec3(0, 0, 0),       Vec3(0.5f, 0, 0),   Vec3(1, 1, 1),       Vec3(4, 0.2f, 0.1f),
            Vec3(2, 0, 0),       Vec3(0, 3, 0),       Vec3(0, 0, 5),       Vec3(0.125f, 0.125f, 0.125f),
            Vec3(10, 10, 10),    Vec3(0, 0, 0),       Vec3(0.2f, 0, 0),    Vec3(5, 0, 0),
        };
        const std::string path = "test_display_roundtrip.ppm";

        CHECK(Display::writePPM(path, 4, 3, px));

        int w = 0, h = 0, maxval = 0;
        std::vector<std::uint8_t> data;
        CHECK(readP6(path, w, h, maxval, data));
        CHECK(w == 4 && h == 3 && maxval == 255);
        CHECK(data.size() == (size_t)4 * 3 * 3);

        for (size_t i = 0; i < px.size(); ++i) {
            std::uint8_t er, eg, eb;
            Display::toneMap(px[i], er, eg, eb);
            CHECK(data[i * 3 + 0] == er);
            CHECK(data[i * 3 + 1] == eg);
            CHECK(data[i * 3 + 2] == eb);
        }

        // Pin a few hand-computed bytes so a broken toneMap cannot hide behind
        // the self-consistent comparison above:
        // px[0] = (0,0,0) -> 0; px[2] = (1,1,1) -> 128;
        // px[8] = (10,10,10) -> 10/11*255 = 231.8 -> 232.
        CHECK(data[0 * 3 + 0] == 0 && data[0 * 3 + 1] == 0 && data[0 * 3 + 2] == 0);
        CHECK(data[2 * 3 + 0] == 128 && data[2 * 3 + 1] == 128 && data[2 * 3 + 2] == 128);
        CHECK(data[8 * 3 + 0] == 232 && data[8 * 3 + 1] == 232 && data[8 * 3 + 2] == 232);

        std::filesystem::remove(path);

        // A nonexistent directory must fail cleanly, not crash.
        CHECK(!Display::writePPM("no_such_dir_xyz/file.ppm", 4, 3, px));
    }

    return checkFinish();
}
