#include <Engine/PCH.h>
#include <Engine/Application/Application.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>
#include <Engine/Input/Input.h>
#include <Engine/Layer/Layer.h>
#include <Engine/Motion/MotionHost.h>
#include <Engine/Rendering/Renderer.h>

#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cstdio>
#include <string>

namespace
{
    std::string g_Script;
    std::string g_Assets;

    class PreviewLayer : public SIMPEngine::Layer
    {
    public:
        PreviewLayer() : Layer("Preview") {}

        void OnAttach() override
        {
            m_Host.Load(g_Script, false);
            m_Duration = m_Host.Duration();
        }

        void OnUpdate(SIMPEngine::TimeStep ts) override
        {
            using SIMPEngine::Input;
            const float dt = std::min(ts.GetSeconds(), 0.1f);

            m_PollTimer += dt;
            if (m_PollTimer >= 0.25f)
            {
                m_PollTimer = 0.0f;
                if (m_Host.PollReload())
                {
                    m_Host.Seek(m_Host.Time());
                    m_Notice = m_Host.Error().empty() ? "reloaded" : "reload failed";
                    m_NoticeTime = 2.0f;
                }
            }
            if (m_NoticeTime > 0.0f)
                m_NoticeTime -= dt;

            if (Input::IsKeyJustPressed(SDLK_SPACE))
                m_Playing = !m_Playing;
            if (Input::IsKeyJustPressed(SDLK_L))
                m_Loop = !m_Loop;
            const bool shift = Input::IsKeyPressed(SDLK_LSHIFT) || Input::IsKeyPressed(SDLK_RSHIFT);
            const float jump = shift ? 10.0f : 2.0f;
            if (Input::IsKeyJustPressed(SDLK_RIGHT))
                m_Host.Seek(m_Host.Time() + jump);
            if (Input::IsKeyJustPressed(SDLK_LEFT))
                m_Host.Seek(m_Host.Time() - jump);
            if (Input::IsKeyJustPressed(SDLK_PERIOD))
                m_Host.Seek(m_Host.Time() + 1.0f / 30.0f);
            if (Input::IsKeyJustPressed(SDLK_COMMA))
                m_Host.Seek(m_Host.Time() - 1.0f / 30.0f);
            if (Input::IsKeyJustPressed(SDLK_R) || Input::IsKeyJustPressed(SDLK_HOME))
                m_Host.Seek(0.0f);
            if (Input::IsKeyJustPressed(SDLK_F5))
            {
                m_Host.Reload();
                m_Host.Seek(m_Host.Time());
            }

            if (m_Playing)
                m_Host.Step(dt);
            m_Duration = m_Host.Duration();
            if (m_Duration > 0.0f && m_Host.Time() >= m_Duration)
            {
                if (m_Loop)
                    m_Host.Seek(0.0f);
                else
                    m_Playing = false;
            }
        }

        void OnRender() override
        {
            const glm::vec2 size = SIMPEngine::Renderer::GetViewportSize();
            const int w = static_cast<int>(size.x), h = static_cast<int>(size.y);
            m_Host.Render(w, h);

            SIMPEngine::Renderer::BeginScene(glm::mat4(1.0f), glm::ortho(0.0f, size.x, 0.0f, size.y, -1.0f, 1.0f));
            SIMPEngine::Renderer::DrawQuad(size.x * 0.5f, 17.0f, size.x, 34.0f, 0.0f, {0, 0, 0, 170}, true, 10.0f);
            if (m_Duration > 0.0f)
            {
                const float p = std::clamp(m_Host.Time() / m_Duration, 0.0f, 1.0f);
                SIMPEngine::Renderer::DrawQuad(size.x * 0.5f, 34.0f, size.x, 3.0f, 0.0f, {255, 255, 255, 40}, true, 11.0f);
                SIMPEngine::Renderer::DrawQuad(size.x * p * 0.5f, 34.0f, size.x * p, 3.0f, 0.0f, {255, 206, 96, 255}, true, 12.0f);
            }
            char line[256];
            if (m_Duration > 0.0f)
                std::snprintf(line, sizeof(line), "%s %.1f / %.1f s%s     space play  arrows seek  . , frame  r restart  l loop  F5 reload",
                              m_Playing ? "playing" : "paused", m_Host.Time(), m_Duration, m_Loop ? "  (loop)" : "");
            else
                std::snprintf(line, sizeof(line), "%s %.1f s     space play  arrows seek  r restart  F5 reload", m_Playing ? "playing" : "paused", m_Host.Time());
            SIMPEngine::Renderer::DrawText(line, 12.0f, 27.0f, 16.0f, {235, 238, 248, 255}, SIMPEngine::TextAlign::Left, 0.0f, 13.0f);
            if (m_NoticeTime > 0.0f)
                SIMPEngine::Renderer::DrawText(m_Notice, size.x - 12.0f, 27.0f, 16.0f, {120, 230, 160, 255}, SIMPEngine::TextAlign::Right, 0.0f, 13.0f);

            if (!m_Host.Error().empty())
            {
                const std::string &error = m_Host.Error();
                int lines = 1;
                for (char c : error)
                    lines += c == '\n';
                lines = std::min(lines, 7);
                const float height = 22.0f * static_cast<float>(lines) + 12.0f;
                SIMPEngine::Renderer::DrawQuad(size.x * 0.5f, size.y - height * 0.5f, size.x, height, 0.0f, {120, 20, 30, 230}, true, 10.0f);
                float y = size.y - 8.0f;
                size_t start = 0;
                for (int i = 0; i < lines && start <= error.size(); ++i)
                {
                    size_t end = error.find('\n', start);
                    if (end == std::string::npos)
                        end = error.size();
                    SIMPEngine::Renderer::DrawText(error.substr(start, end - start), 12.0f, y, 16.0f, {255, 220, 220, 255}, SIMPEngine::TextAlign::Left, 0.0f, 13.0f);
                    y -= 22.0f;
                    start = end + 1;
                }
            }
            SIMPEngine::Renderer::EndScene();
        }

    private:
        SIMPEngine::MotionHost m_Host;
        bool m_Playing = true;
        bool m_Loop = true;
        float m_PollTimer = 0.0f;
        float m_Duration = 0.0f;
        float m_NoticeTime = 0.0f;
        std::string m_Notice;
    };

    class Preview : public SIMPEngine::Application
    {
    public:
        Preview() : Application(Specification())
        {
            PushOverlay(std::make_unique<PreviewLayer>());
        }

        static SIMPEngine::ApplicationSpecification Specification()
        {
            SIMPEngine::ApplicationSpecification spec;
            spec.Title = "SIMP Preview";
            spec.Name = "SIMPPreview";
            spec.Width = 1280;
            spec.Height = 720;
            return spec;
        }
    };
}

SIMPEngine::Application *SIMPEngine::CreateApplication()
{
    return new Preview();
}

int main(int argc, char **argv)
{
    SIMPEngine::Log::Init();
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--assets" && i + 1 < argc)
            g_Assets = argv[++i];
        else if (g_Script.empty())
            g_Script = arg;
    }
    if (g_Script.empty())
    {
        std::fprintf(stderr, "SIMPPreview <script.lua> [--assets DIR]\n  space play/pause, arrows seek (shift = 10 s), . and , step a frame, r restart, l loop, F5 reload.\n  The script reloads by itself whenever a .lua file next to it is saved.\n");
        return 1;
    }
    SIMPEngine::VFS::Mount("assets", g_Assets.empty() ? "../assets" : g_Assets);

    SIMPEngine::Application *app = SIMPEngine::CreateApplication();
    app->Run();
    delete app;
    return 0;
}
