#include "check.h"

#include <initializer_list>
#include <vector>

#include "cli_args.h"

// Runs parseArgs on the given arguments (argv[0] is supplied automatically).
static bool runArgs(std::initializer_list<const char*> args, Args& a) {
    std::vector<char*> argv;
    argv.push_back((char*)"pathtracer");
    for (const char* s : args) argv.push_back((char*)s);
    return parseArgs((int)argv.size(), argv.data(), a);
}

int main() {
    // 1. No arguments: everything stays at its default.
    {
        Args a;
        CHECK(runArgs({}, a));
        CHECK(a.width == 960);
        CHECK(a.height == 540);
        CHECK(a.spp == 8);
        CHECK(a.frames == 0);
        CHECK(a.seed == 12345);
        CHECK(a.out.empty());
        CHECK(!a.accumulate);
    }

    // 2. All value options at once.
    {
        Args a;
        CHECK(runArgs({"--width", "320", "--height", "240", "--spp", "64",
                       "--frames", "5", "--seed", "42", "--out", "tmp"}, a));
        CHECK(a.width == 320);
        CHECK(a.height == 240);
        CHECK(a.spp == 64);
        CHECK(a.frames == 5);
        CHECK(a.seed == 42);
        CHECK(a.out == "tmp");
        CHECK(!a.accumulate);
    }

    // 3. --accumulate flag, in both orders relative to value options.
    {
        Args a;
        CHECK(runArgs({"--width", "320", "--accumulate"}, a));
        CHECK(a.accumulate);
        CHECK(a.width == 320);
        Args b;
        CHECK(runArgs({"--accumulate", "--height", "240"}, b));
        CHECK(b.accumulate);
        CHECK(b.height == 240);
    }

    // 4. Repeated option: last value wins.
    {
        Args a;
        CHECK(runArgs({"--width", "100", "--width", "50"}, a));
        CHECK(a.width == 50);
        CHECK(a.height == 540);
    }

    // 5. Missing value for a value-taking option.
    {
        Args a;
        CHECK(!runArgs({"--width"}, a));
        CHECK(!runArgs({"--out"}, a));
        CHECK(!runArgs({"--height", "240", "--spp"}, a));
    }

    // 6. Unknown argument.
    {
        Args a;
        CHECK(!runArgs({"--bogus"}, a));
    }

    // 7. width/height below the minimum of 4.
    {
        Args a;
        CHECK(!runArgs({"--width", "2"}, a));
        Args b;
        CHECK(!runArgs({"--height", "3"}, b));
        Args c;
        CHECK(runArgs({"--width", "4", "--height", "4"}, c));  // the minimum itself is accepted
    }

    // 8. Clamps: spp < 1 -> 1, frames < 0 -> 0.
    {
        Args a;
        CHECK(runArgs({"--spp", "0"}, a));
        CHECK(a.spp == 1);
        Args b;
        CHECK(runArgs({"--frames", "-3"}, b));
        CHECK(b.frames == 0);
        Args c;
        CHECK(runArgs({"--spp", "-7"}, c));
        CHECK(c.spp == 1);
    }

    // 9. Non-numeric value: atoi -> 0 -> width below the minimum.
    {
        Args a;
        CHECK(!runArgs({"--width", "abc"}, a));
    }

    // 10. Valid options followed by an invalid one.
    {
        Args a;
        CHECK(!runArgs({"--width", "320", "--bogus"}, a));
    }

    return checkFinish();
}
