#pragma once

#include <cstdint>

namespace Mossveil
{
    class Builder;

    void BuildMossway(Builder &b);
    void BuildThornroot(Builder &b);
    void BuildShiftingDeep(Builder &b, uint32_t seed);
}
