#pragma once

namespace SIMPEngine
{
    class EngineDimensions
    {
    public:
        static int GetWidth() { return s_Width; }
        static int GetHeight() { return s_Height; }

        static void SetDimensions(int width, int height)
        {
            s_Width = width;
            s_Height = height;
        }

    private:
        static int s_Width;
        static int s_Height;
    };
}
