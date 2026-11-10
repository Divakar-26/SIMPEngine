#include <Engine/PCH.h>
#include <Engine/Application/Application.h>
#include <Engine/Core/FileSystem.h>
#include <Engine/Core/VFS.h>
#include <Engine/Core/Profiler/EngineProfiler.h>

#include "Events/ApplicationEvent.h"
#include "Events/EventDispatcher.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "Input/Input.h"
#include "Rendering/GLRenderingAPI.h"
#include <Engine/Rendering/Renderer.h>
#include <Engine/Rendering/SvgTextureCache.h>
#include <Engine/Rendering/TextureManager.h>
#include <Engine/Particles/ParticleEmitter.h>
#include <Engine/Scene/Systems/LightingSystem.h>
#include <Engine/Audio/AudioEngine.h>
#include <Engine/Scripting/LuaRuntime.h>
#include <Engine/Rendering/Bloom.h>
#include <Engine/Rendering/Font.h>
#include <Engine/Core/Version.h>
#include <Engine/Project/Project.h>

namespace SIMPEngine
{
    class KeyReleasedEvent;
    class KeyPressedEvent;
    class WindowCloseEvent;
    Application *Application::s_Instance = nullptr;

    Application::Application(const ApplicationSpecification &specification)
    {
        s_Instance = this;
        CORE_INFO("{} {}", EngineName, EngineVersion);
        Project::MountEngineResources();

        SDL_WindowFlags flags = 0;
        if (specification.Resizable)
            flags |= SDL_WINDOW_RESIZABLE;
        if (specification.Fullscreen)
            flags |= SDL_WINDOW_FULLSCREEN;
        if (specification.Maximized)
            flags |= SDL_WINDOW_MAXIMIZED;
        m_Window.SetVSync(specification.VSync);
        m_Window.Init(specification.Title.c_str(), specification.Width, specification.Height, flags);

        int w, h;
        SDL_GetWindowSize(m_Window.GetNativeWindow(), &w, &h);
        Renderer::Init(std::make_unique<GLRenderingAPI>(), w, h);
        AudioEngine::Init();

        auto imguiLayer = std::make_unique<ImGuiLayer>();

        m_ImGuiLayer = imguiLayer.get();

        PushOverlay(std::move(imguiLayer));

        CORE_INFO("Entt version {}", ENTT_VERSION);
    }

    Application::~Application()
    {
        ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
        m_LayerStack.Clear();
        LuaRuntime::Get().ShutDown();
        AudioEngine::Shutdown();
        Renderer::Flush();
        SvgTextureCache::Clear();
        Font::ClearCache();
        ParticleTextures::Release();
        LightingSystem::ReleaseTextures();
        BloomRenderer::ReleaseShared();
        TextureManager::Get().UnloadAll();
        Renderer::Shutdown();
        s_Instance = nullptr;
    }

    void Application::Run()
    {
        Input::ResetAllKeys();

        uint64_t lastFrameTime = SDL_GetPerformanceCounter();
        const double freq = static_cast<double>(SDL_GetPerformanceFrequency());

        while (m_Running)
        {
            PROFILE_FRAME_BEGIN();

            uint64_t currentFrameTime = SDL_GetPerformanceCounter();
            float deltaTime = static_cast<float>(static_cast<double>(currentFrameTime - lastFrameTime) / freq);
            lastFrameTime = currentFrameTime;
            deltaTime = std::min(deltaTime, 0.1f);

            if (!m_Queued.empty())
            {
                std::vector<std::function<void()>> queued;
                queued.swap(m_Queued);
                for (auto &action : queued)
                    action();
            }

            Input::BeginFrame();

            {
                PROFILE_SCOPE("Events");

                SDL_Event sdlEvent;
                while (SDL_PollEvent(&sdlEvent))
                {
                    if (m_ImGuiLayer)
                        m_ImGuiLayer->OnSDLEvent(sdlEvent);
                    SDLEventToEngine(sdlEvent);
                }
            }

            if (SDL_GetWindowFlags(m_Window.GetNativeWindow()) & SDL_WINDOW_MINIMIZED)
                SDL_Delay(16);

            {
                PROFILE_SCOPE("LayerUpdate");

                for (auto & layer : m_LayerStack)
                    layer->OnUpdate(TimeStep(deltaTime));
                AudioEngine::Update();
            }

            {
                {
                    PROFILE_SCOPE("Render/SetClearColor");
                    Renderer::SetClearColor(0.329f,
                                            0.329f,
                                            0.329f,
                                            1.0f);
                }
                {
                    PROFILE_SCOPE("Render/ClearCall");
                    Renderer::Clear();
                }
                {
                    PROFILE_SCOPE("Render/ImGuiBegin");
                    m_ImGuiLayer->Begin();
                }
                {
                    PROFILE_SCOPE("Render/Layers");
                    for (auto & layer : m_LayerStack)
                        layer->OnRender();
                }

                {
                    PROFILE_SCOPE("Render/ImGuiEnd");
                    m_ImGuiLayer->End();
                }
            }

            {
                PROFILE_SCOPE("Render/Swap");
                m_Window.OnUpdate();
            }
            PROFILE_FRAME_END();
        }
    }

    void Application::PushLayer(std::unique_ptr<Layer> layer)
    {
        m_LayerStack.PushLayer(std::move(layer));
    }

    void Application::PushOverlay(std::unique_ptr<Layer> overlay)
    {
        m_LayerStack.PushOverlay(std::move(overlay));
    }

    void Application::PopLayer(Layer *layer)
    {
        m_LayerStack.PopLayer(layer);
    }

    void Application::PopOverlay(Layer *overlay)
    {
        m_LayerStack.PopOverlay(overlay);
    }

    void Application::Queue(std::function<void()> action)
    {
        m_Queued.push_back(std::move(action));
    }

    void Application::SDLEventToEngine(SDL_Event &sdlEvent)
    {
        switch (sdlEvent.type)
        {
        case SDL_EVENT_QUIT:
        {
            WindowCloseEvent e;
            OnEvent(e);
            break;
        }
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            Input::ResetAllKeys();
            break;
        case SDL_EVENT_WINDOW_RESIZED:
        {
            WindowResizeEvent e(sdlEvent.window.data1, sdlEvent.window.data2);
            OnEvent(e);
            break;
        }
        case SDL_EVENT_KEY_DOWN:
        {
            bool repeat = (sdlEvent.key.repeat != 0);
            KeyPressedEvent e(static_cast<int>(sdlEvent.key.key), repeat);
            OnEvent(e);
            break;
        }
        case SDL_EVENT_KEY_UP:
        {
            KeyReleasedEvent e(static_cast<int>(sdlEvent.key.key));
            OnEvent(e);
            break;
        }
        case SDL_EVENT_TEXT_INPUT:
        {
            KeyTypedEvent e(sdlEvent.text.text[0]);
            OnEvent(e);
            break;
        }
        case SDL_EVENT_MOUSE_MOTION:
        {
            MouseMovedEvent e(sdlEvent.motion.x, sdlEvent.motion.y);
            OnEvent(e);
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            MouseButtonPressedEvent e(sdlEvent.button.button);
            OnEvent(e);
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            MouseButtonReleasedEvent e(sdlEvent.button.button);
            OnEvent(e);
            break;
        }
        case SDL_EVENT_MOUSE_WHEEL:
        {
            MouseScrolledEvent e(sdlEvent.wheel.x, sdlEvent.wheel.y);
            OnEvent(e);
            break;
        }
        default:
            break;
        }
    }

    void Application::OnEvent(Event &e)
    {
        m_ImGuiLayer->OnEvent(e);

        EventDispatcher dispatcher(e);

        if (e.GetEventType() == EventType::WindowClose)
        {
            for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
            {
                (*--it)->OnEvent(e);
                if (e.Handled)
                    return;
            }
            m_Running = false;
            return;
        }

        dispatcher.Dispatch<WindowResizeEvent>([](WindowResizeEvent &ev)
                                               {
                                                   Renderer::SetWindowSize(ev.GetWidth(), ev.GetHeight());
                                                   return false;
                                               });

        dispatcher.Dispatch<KeyPressedEvent>([](KeyPressedEvent &ev)
                                             {
                                                 Input::OnKeyPressed(ev.GetKeyCode());
                                                 return false; });

        dispatcher.Dispatch<KeyReleasedEvent>([](KeyReleasedEvent &ev)
                                              {
        Input::OnKeyReleased(ev.GetKeyCode());
        return false; });

        dispatcher.Dispatch<MouseButtonPressedEvent>([](MouseButtonPressedEvent &ev)
                                                     {
        Input::OnMouseButtonPressed(ev.GetMouseButton());
        return false; });

        dispatcher.Dispatch<MouseButtonReleasedEvent>([](MouseButtonReleasedEvent &ev)
                                                      {
        Input::OnMouseButtonReleased(ev.GetMouseButton());
        return false; });

        dispatcher.Dispatch<MouseMovedEvent>([](MouseMovedEvent &ev)
                                             {
        Input::OnMouseMoved(ev.GetX(), ev.GetY());
        return false; });

        dispatcher.Dispatch<MouseScrolledEvent>([](MouseScrolledEvent &ev)
                                                {
    Input::OnMouseWheel(ev.GetYOffset());
    return false; });

        if (!e.Handled)
        {
            for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
            {
                (*--it)->OnEvent(e);
                if (e.Handled)
                    break;
            }
        }
    }
}
