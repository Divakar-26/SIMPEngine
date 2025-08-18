#pragma once

#include "Layer/Layer.h" // Your base Layer class
#include "TimeStep.h"
#include <SDL3/SDL_events.h>
#include <imgui.h>
#include"Scene/Entity.h"
#include"Scene/Scene.h"

struct SDL_Window;
struct SDL_Renderer;

namespace SIMPEngine
{

    class ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer();

        void OnAttach() override;                  // Called when layer is pushed
        void OnDetach() override;                  // Called when layer is popped
        void OnUpdate(class TimeStep ts) override; // Called every frame to update ImGui
        void OnRender() override;                  // Called every frame to update ImGui
        void OnEvent(Event &event) override;
        void OnSDLEvent(SDL_Event &e);
        void Begin();
        void End();

        bool viewportFocused = false;

        void SetBlockEvent(bool block) {m_BlockEvent = block;}
    private:
        bool showLogWindow = false; // toggle for log window

        SDL_Window *m_Window = nullptr;
        SDL_Renderer *m_Renderer = nullptr;

        SIMPEngine::Entity selectedEntity;

        bool m_BlockEvent;

    };

}
