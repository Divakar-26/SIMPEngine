#include "FrameCapture.h"

#include <Engine/Application/Application.h>
#include <Engine/Core/Log.h>
#include <Engine/Events/KeyEvent.h>

#include <glad/glad.h>
#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#define SIMP_POPEN _popen
#define SIMP_PCLOSE _pclose
#else
#define SIMP_POPEN popen
#define SIMP_PCLOSE pclose
#endif

namespace SIMPEngine
{
    FrameCapture::~FrameCapture()
    {
        End();
    }

    bool FrameCapture::Begin(int width, int height)
    {
        const char *output = std::getenv("SIMP_CAPTURE");
        if (!output || !*output)
            return false;

        if (const char *fps = std::getenv("SIMP_CAPTURE_FPS"))
            m_Fps = std::max(1, std::atoi(fps));
        if (const char *frames = std::getenv("SIMP_CAPTURE_FRAMES"))
            m_MaxFrames = static_cast<uint64_t>(std::max(0, std::atoi(frames)));
        if (const char *input = std::getenv("SIMP_CAPTURE_INPUT"))
            LoadInput(input);

        m_Width = width;
        m_Height = height;
        m_Pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);

        std::ostringstream command;
        command << "ffmpeg -y -loglevel error -f rawvideo -pix_fmt rgba -s " << width << "x" << height << " -r " << m_Fps
                << " -i - -vf vflip -c:v libx264 -preset slow -crf 16 -pix_fmt yuv420p \"" << output << "\"";
        m_Pipe = SIMP_POPEN(command.str().c_str(), "w");
        if (!m_Pipe)
        {
            CORE_ERROR("Frame capture: could not start ffmpeg");
            return false;
        }
        CORE_INFO("Frame capture: {}x{} at {} fps into {}", width, height, m_Fps, output);
        return true;
    }

    void FrameCapture::LoadInput(const std::string &path)
    {
        std::ifstream file(path);
        if (!file)
        {
            CORE_WARN("Frame capture: no input script at {}", path);
            return;
        }
        std::string line;
        while (std::getline(file, line))
        {
            std::istringstream in(line);
            InputEvent event;
            std::string action, key;
            if (!(in >> event.Frame >> action >> key))
                continue;
            event.Down = action == "down";
            event.Key = static_cast<int>(SDL_GetKeyFromName(key.c_str()));
            if (event.Key == 0)
            {
                CORE_WARN("Frame capture: unknown key '{}'", key);
                continue;
            }
            m_Input.push_back(event);
        }
        std::stable_sort(m_Input.begin(), m_Input.end(), [](const InputEvent &a, const InputEvent &b) { return a.Frame < b.Frame; });
    }

    void FrameCapture::ApplyInput(Application &app)
    {
        while (m_NextInput < m_Input.size() && m_Input[m_NextInput].Frame <= m_Frame)
        {
            const InputEvent &event = m_Input[m_NextInput++];
            if (event.Down)
            {
                KeyPressedEvent e(event.Key, false);
                app.OnEvent(e);
            }
            else
            {
                KeyReleasedEvent e(event.Key);
                app.OnEvent(e);
            }
        }
    }

    bool FrameCapture::Capture()
    {
        if (!m_Pipe)
            return true;
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glReadBuffer(GL_BACK);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, m_Width, m_Height, GL_RGBA, GL_UNSIGNED_BYTE, m_Pixels.data());
        std::fwrite(m_Pixels.data(), 1, m_Pixels.size(), m_Pipe);
        ++m_Frame;
        return m_MaxFrames == 0 || m_Frame < m_MaxFrames;
    }

    void FrameCapture::End()
    {
        if (!m_Pipe)
            return;
        SIMP_PCLOSE(m_Pipe);
        m_Pipe = nullptr;
        CORE_INFO("Frame capture: wrote {} frames", m_Frame);
    }
}
