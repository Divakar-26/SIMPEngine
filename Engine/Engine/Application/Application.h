#pragma once
#include <Engine/Application/FrameCapture.h>
#include <Engine/Core/Window.h>
#include <Engine/Events/Event.h>
#include <Engine/Layer/LayerStack.h>
#include <Engine/UI/ImGuiLayer.h>
#include <Engine/Layer/RenderingLayer.h>
#include <Engine/Math/Camera2D.h>
#include <Engine/Scene/SceneManager.h>

#include <functional>
#include <string>
#include <vector>

namespace SIMPEngine
{
    struct ApplicationSpecification
    {
        std::string Title = "SIMPEngine";
        std::string Name;
        int Width = 1920;
        int Height = 1080;
        bool Resizable = true;
        bool Fullscreen = false;
        bool Maximized = false;
        bool VSync = true;
    };

    class Application
    {
    public:
        Application(const ApplicationSpecification &specification = {});
        virtual ~Application();

        void Run();
        void OnEvent(Event &e);
        void SDLEventToEngine(SDL_Event &event);

        void PushLayer(std::unique_ptr<Layer> layer);
        void PushOverlay(std::unique_ptr<Layer> overlay);
        void PopLayer(Layer *layer);
        void PopOverlay(Layer *overlay);
        void Queue(std::function<void()> action);
        void Close() { m_Running = false; }

        Window &GetWindow() { return m_Window; }

        static Application &Get() { return *s_Instance; }
        static Application *TryGet() { return s_Instance; }

        void SetFullscreen(bool fullscreen);
        bool IsFullscreen();
        void SetFullscreenOverride(std::function<void(bool)> set, std::function<bool()> get);

        ImGuiLayer *GetImGuiLayer() { return m_ImGuiLayer; }

    private:
        Window m_Window;
        bool m_Running = true;

        LayerStack m_LayerStack;
        std::vector<std::function<void()>> m_Queued;
        FrameCapture m_Capture;
        ImGuiLayer *m_ImGuiLayer = nullptr;

        template <typename Fn>
        bool Guarded(const char *stage, Layer &layer, Fn &&fn);
        void ReportFault(const char *stage, const std::string &layer, const char *what);

        std::function<void(bool)> m_SetFullscreen;
        std::function<bool()> m_IsFullscreen;
        std::string m_LastFault;
        int m_FaultRepeats = 0;

        static Application *s_Instance;
    };

    Application *CreateApplication();
}
