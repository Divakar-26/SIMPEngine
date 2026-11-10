#pragma once

#include <Engine/Layer/Layer.h>
#include <Engine/Core/TimeStep.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>

#include <SDL3/SDL_events.h>
#include <imgui.h>

struct SDL_Window;
struct SDL_Renderer;

namespace SIMPEngine
{
    class ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer();

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(class TimeStep ts) override;
        void OnRender() override;
        void OnEvent(Event &event) override;
        void OnSDLEvent(SDL_Event &e) override;
        void Begin();
        void End();

        bool viewportFocused = false;

        void SetBlockEvent(bool block) { m_BlockEvent = block; }

    private:
        void ApplyModernTheme();
        void LoadFonts();

        bool showLogWindow = false;

        SDL_Window *m_Window = nullptr;
        SDL_Renderer *m_Renderer = nullptr;

        SIMPEngine::Entity selectedEntity;

        bool m_BlockEvent = false;

        ImFont *m_FontRegular = nullptr;
        ImFont *m_FontBold = nullptr;
    };
}
