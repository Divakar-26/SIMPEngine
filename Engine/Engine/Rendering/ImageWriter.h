#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace SIMPEngine::ImageWriter
{
    bool EncodePng(int width, int height, const uint8_t *rgba, std::string &out);
    bool WritePng(const std::string &path, int width, int height, const std::vector<uint8_t> &rgba);
}
