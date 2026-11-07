#pragma once

#include <string>

namespace SIMPEngine
{
    struct SpriteSheetSlice
    {
        bool Found = false;
        int FrameWidth = 0;
        int FrameHeight = 0;
        int Columns = 1;
        int Rows = 1;
        int FrameCount = 1;
        std::string Method;
    };

    class SpriteSheetSlicer
    {
    public:
        static SpriteSheetSlice Detect(const std::string &path);
        static SpriteSheetSlice Detect(const unsigned char *rgba, int width, int height);
    };
}
