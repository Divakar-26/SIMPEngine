#include "TimerSystem.h"

#include <Engine/Scene/Scene.h>

#include <algorithm>
#include <cmath>

namespace SIMPEngine
{
    void TimerSystem::Update(Scene &scene, float dt)
    {
        entt::registry &registry = scene.GetRegistry();
        m_Fired.clear();

        auto view = registry.view<TimerComponent>();
        for (entt::entity handle : view)
        {
            TimerComponent &timer = view.get<TimerComponent>(handle);
            if (!timer.running)
                continue;

            const float interval = std::max(timer.interval, 0.001f);
            timer.elapsed += dt;
            if (timer.elapsed < interval)
                continue;

            if (timer.repeat)
            {
                timer.elapsed = std::fmod(timer.elapsed, interval);
            }
            else
            {
                timer.running = false;
                timer.elapsed = 0.0f;
            }
            m_Fired.push_back(handle);
        }

        for (entt::entity handle : m_Fired)
            scene.SendEvent(handle, ScriptEvent::Timer);
    }
}
