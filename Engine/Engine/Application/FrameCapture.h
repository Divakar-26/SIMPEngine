#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace SIMPEngine
{
    class Application;

    class FrameCapture
    {
    public:
        ~FrameCapture();

        bool Begin(int width, int height);
        bool IsActive() const { return m_Pipe != nullptr; }
        float FrameTime() const { return 1.0f / static_cast<float>(m_Fps); }
        void ApplyInput(Application &app);
        bool Capture();
        void End();

    private:
        struct InputEvent
        {
            uint64_t Frame = 0;
            bool Down = true;
            int Key = 0;
        };

        void LoadInput(const std::string &path);

        std::FILE *m_Pipe = nullptr;
        int m_Width = 0;
        int m_Height = 0;
        int m_Fps = 30;
        uint64_t m_Frame = 0;
        uint64_t m_MaxFrames = 0;
        size_t m_NextInput = 0;
        std::vector<InputEvent> m_Input;
        std::vector<unsigned char> m_Pixels;
    };
}
