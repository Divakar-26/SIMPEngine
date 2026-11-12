#include "ButtonSystem.h"
#include "SystemUtil.h"

#include <Engine/Audio/AudioEngine.h>
#include <Engine/Input/Input.h>
#include <Engine/Scene/Scene.h>

#include <vector>

namespace SIMPEngine
{
    namespace
    {
        int ButtonCode(const ButtonComponent &button)
        {
            return button.button == 1 ? 3 : button.button == 2 ? 2 : 1;
        }

        void ApplyScale(TransformComponent &transform, ButtonComponent &button, int state)
        {
            if (state == button.state)
                return;
            if (button.state == 0)
                button.baseScale = transform.scale;
            const float factor = state == 2 ? button.pressScale : state == 1 ? button.hoverScale : 1.0f;
            transform.scale = button.baseScale * factor;
            transform.dirty = true;
            button.state = state;
        }
    }

    void ButtonSystem::Update(Scene &scene)
    {
        entt::registry &registry = scene.GetRegistry();
        auto view = registry.view<ButtonComponent, TransformComponent>();

        const bool inside = Input::IsMouseInViewport() && !scene.IsMouseOverUI();
        const glm::vec2 mouse = scene.ScreenToWorld(Input::GetMouseViewportPosition());

        entt::entity top = entt::null;
        float topZ = 0.0f;
        if (inside)
        {
            for (entt::entity handle : view)
            {
                const ButtonComponent &button = view.get<ButtonComponent>(handle);
                if (!button.enabled || !PointHitsEntity(registry, handle, mouse, button.padding))
                    continue;
                const float z = view.get<TransformComponent>(handle).zIndex;
                if (top == entt::null || z >= topZ)
                {
                    top = handle;
                    topZ = z;
                }
            }
        }

        struct Pending
        {
            entt::entity Entity;
            ScriptEvent Type;
        };
        std::vector<Pending> pending;

        for (entt::entity handle : view)
        {
            ButtonComponent &button = view.get<ButtonComponent>(handle);
            TransformComponent &transform = view.get<TransformComponent>(handle);

            const bool hovered = button.enabled && handle == top;
            const int code = ButtonCode(button);

            if (hovered && !button.hovering)
            {
                pending.push_back({handle, ScriptEvent::HoverEnter});
                if (!button.hoverSound.empty())
                    AudioEngine::Play(button.hoverSound, button.volume, 1.0f, false, AudioBus::Ui);
            }
            else if (!hovered && button.hovering)
                pending.push_back({handle, ScriptEvent::HoverExit});
            button.hovering = hovered;

            if (hovered && Input::IsMouseButtonJustPressed(code))
                button.pressed = true;
            if (button.pressed && (!button.enabled || !Input::IsMouseButtonPressed(code)))
            {
                button.pressed = false;
                if (hovered)
                {
                    pending.push_back({handle, ScriptEvent::Clicked});
                    if (!button.clickSound.empty())
                        AudioEngine::Play(button.clickSound, button.volume, 1.0f, false, AudioBus::Ui);
                }
            }

            ApplyScale(transform, button, hovered ? (button.pressed ? 2 : 1) : 0);
        }

        for (const Pending &event : pending)
            scene.SendEvent(event.Entity, event.Type);
    }
}
