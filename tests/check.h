#pragma once
#include <cmath>
#include <cstdio>

inline int g_checks_failed = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);   \
            ++g_checks_failed;                                            \
        }                                                                 \
    } while (0)

#define CHECK_NEAR(a, b, eps)                                                        \
    do {                                                                             \
        const double _a = (a), _b = (b);                                             \
        if (std::fabs(_a - _b) > (double)(eps)) {                                    \
            std::printf("FAIL %s:%d: %s = %g, expected %s = %g (eps %g)\n",          \
                        __FILE__, __LINE__, #a, _a, #b, _b, (double)(eps));          \
            ++g_checks_failed;                                                       \
        }                                                                            \
    } while (0)

inline int checkFinish() {
    if (g_checks_failed > 0) {
        std::printf("%d check(s) FAILED\n", g_checks_failed);
        return 1;
    }
    std::printf("all checks passed\n");
    return 0;
}
