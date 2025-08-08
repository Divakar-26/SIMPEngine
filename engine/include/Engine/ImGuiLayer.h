#pragma once

#include "Engine/Layer.h" // Your base Layer class
#include<SDL3/SDL_events.h>
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

        void OnAttach() override;            // Called when layer is pushed
        void OnDetach() override;            // Called when layer is popped
        void OnUpdate() override;            // Called every frame to update ImGui
        void OnEvent(Event &event) override; // Optional: if you want to handle events

        void OnSDLEvent(SDL_Event &e);
    private:
        void Begin();
        void End();

        SDL_Window *m_Window = nullptr;
        SDL_Renderer *m_Renderer = nullptr;
    };

}
