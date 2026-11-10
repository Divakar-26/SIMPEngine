#include "TestFramework.h"

#include <Engine/Core/Log.h>

#include <spdlog/spdlog.h>

#include <cstring>

int main(int argc, char **argv)
{
    SIMPEngine::Log::Init();
    spdlog::set_level(spdlog::level::off);

    const char *filter = argc > 1 ? argv[1] : nullptr;
    int ran = 0;
    int failedCases = 0;
    for (const SIMPTest::Case &test : SIMPTest::Registry())
    {
        if (filter && !std::strstr(test.Name, filter))
            continue;
        const int before = SIMPTest::Failures();
        test.Run();
        const bool ok = SIMPTest::Failures() == before;
        std::printf("%s %s\n", ok ? "  ok  " : "  FAIL", test.Name);
        failedCases += ok ? 0 : 1;
        ++ran;
    }
    std::printf("\n%d tests, %d failed, %d checks failed\n", ran, failedCases, SIMPTest::Failures());
    return SIMPTest::Failures() == 0 && ran > 0 ? 0 : 1;
}
