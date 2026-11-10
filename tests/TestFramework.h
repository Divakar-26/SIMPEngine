#pragma once

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace SIMPTest
{
    struct Case
    {
        const char *Name;
        void (*Run)();
    };

    inline std::vector<Case> &Registry()
    {
        static std::vector<Case> cases;
        return cases;
    }

    inline int &Failures()
    {
        static int failures = 0;
        return failures;
    }

    struct Registrar
    {
        Registrar(const char *name, void (*run)()) { Registry().push_back({name, run}); }
    };

    inline void Fail(const char *file, int line, const std::string &what)
    {
        ++Failures();
        std::printf("    FAIL %s:%d  %s\n", file, line, what.c_str());
    }
}

#define SIMP_TEST_CONCAT_INNER(a, b) a##b
#define SIMP_TEST_CONCAT(a, b) SIMP_TEST_CONCAT_INNER(a, b)
#define TEST_CASE(name)                                                                              \
    static void SIMP_TEST_CONCAT(SimpTest_, __LINE__)();                                             \
    static SIMPTest::Registrar SIMP_TEST_CONCAT(SimpTestReg_, __LINE__)(name, &SIMP_TEST_CONCAT(SimpTest_, __LINE__)); \
    static void SIMP_TEST_CONCAT(SimpTest_, __LINE__)()

#define CHECK(expr)                                         \
    do                                                      \
    {                                                       \
        if (!(expr))                                        \
            SIMPTest::Fail(__FILE__, __LINE__, #expr);      \
    } while (0)

#define CHECK_NEAR(a, b, eps)                                                                                         \
    do                                                                                                                \
    {                                                                                                                 \
        const double simpA = static_cast<double>(a);                                                                  \
        const double simpB = static_cast<double>(b);                                                                  \
        if (std::abs(simpA - simpB) > static_cast<double>(eps))                                                       \
            SIMPTest::Fail(__FILE__, __LINE__, std::string(#a " ~ " #b "  (") + std::to_string(simpA) + " vs " + std::to_string(simpB) + ")"); \
    } while (0)
