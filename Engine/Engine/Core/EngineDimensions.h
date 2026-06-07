#pragma once

namespace SIMPEngine
{
    /**
     * Centralized engine dimensions management
     * Single source of truth for window/viewport dimensions
     */
    class EngineDimensions
    {
    public:
        // Get current engine dimensions
        static int GetWidth() { return s_Width; }
        static int GetHeight() { return s_Height; }
        
        // Internal use - set by Window class
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
