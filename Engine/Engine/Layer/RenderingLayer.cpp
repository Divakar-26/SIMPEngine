#include <Engine/Layer/RenderingLayer.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/TilemapUtils.h>
#include <Engine/Scene/SceneSerializer.h>
#include <Engine/Scene/Loaders/TiledLoader.h>
#include <Engine/Core/EngineDimensions.h>
#include <Engine/PCH.h>

#include <cstdlib>
#include <ctime>

#include "Events/ApplicationEvent.h"
#include "Events/EventDispatcher.h"
#include "Events/MouseEvent.h"
#include "Events/KeyEvent.h"
#include <Engine/Application/Application.h>
#include "Input/Input.h"
#include "Input/SIMP_Keys.h"

namespace SIMPEngine
{
    class WindowResizeEvent;

    void RenderingLayer::OnAttach()
    {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
    }

    void RenderingLayer::OnDetach()
    {
    }

    void RenderingLayer::OnUpdate(TimeStep ts)
    {
        PROFILE_SCOPE("RenderingLayer/Update");

        auto scene = m_SceneManager->GetActiveScene();

        if (!scene)
            return;

        if (!m_ReloadScenePath.empty() && Input::IsKeyJustPressed(SIMPK_F1))
        {
            SceneSerializer s(scene.get());
            scene->Clear();
            s.Deserialize(m_ReloadScenePath);
        }

        if (m_SimulationEnabled)
            UpdateScene(*scene, ts.GetSeconds());
    }

    void RenderingLayer::Step(float dt)
    {
        auto scene = m_SceneManager->GetActiveScene();
        if (scene)
            UpdateScene(*scene, dt);
    }

    void RenderingLayer::UpdateScene(Scene &scene, float dt)
    {
        try
        {
            scene.OnUpdate(dt);
        }
        catch (const std::exception &e)
        {
            RecordFault("update", e.what());
        }
        catch (...)
        {
            RecordFault("update", "unknown exception");
        }
    }

    void RenderingLayer::RecordFault(const char *stage, const char *what)
    {
        std::string message = std::string("Scene ") + stage + ": " + what;
        if (message != m_LastFault)
            CORE_ERROR("{}", message);
        m_LastFault = message;
        m_Fault = std::move(message);
    }

    void RenderingLayer::OnRender()
    {
        PROFILE_SCOPE("RenderingLayer/Render");
        if (!m_RenderToBackbuffer)
            return;

        auto scene = m_SceneManager->GetActiveScene();
        if (!scene)
            return;

        auto &camera = scene->GetActiveCamera();
        auto [cw, ch] = camera.GetViewportSize();
        if (cw != Renderer::m_WindowWidth || ch != Renderer::m_WindowHeight)
            camera.SetViewportSize(static_cast<float>(Renderer::m_WindowWidth), static_cast<float>(Renderer::m_WindowHeight));

        const glm::vec4 &bg = scene->GetBackgroundColor();
        Renderer::SetClearColor(bg.r, bg.g, bg.b, bg.a);
        Renderer::Clear();

        try
        {
            scene->OnRender();
            Renderer::Flush();
        }
        catch (const std::exception &e)
        {
            Renderer::Recover();
            RecordFault("render", e.what());
        }
    }

    void RenderingLayer::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseButtonPressedEvent>([](MouseButtonPressedEvent &)
                                                     { return false; });

        dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent &ev)
                                             {
            if (!m_RenderToBackbuffer || ev.IsRepeat())
                return false;
            const bool altEnter = ev.GetKeyCode() == SIMPK_RETURN && (SDL_GetModState() & SDL_KMOD_ALT);
            if (ev.GetKeyCode() != SIMPK_F11 && !altEnter)
                return false;
            Application &app = Application::Get();
            app.SetFullscreen(!app.IsFullscreen());
            return true; });

        dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent &ev)
                                               {
        int width = ev.GetWidth();
        int height = ev.GetHeight();

        Renderer::GetAPI()->ResizeViewport(width, height);
        EngineDimensions::SetDimensions(width, height);

        if (m_RenderToBackbuffer)
            if (auto scene = m_SceneManager->GetActiveScene())
                scene->GetActiveCamera().SetViewportSize(static_cast<float>(width), static_cast<float>(height));

        CORE_INFO("Viewport resized: {}x{}", width, height);
        return false; });
    }
}
