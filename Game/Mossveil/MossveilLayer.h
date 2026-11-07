#pragma once

#include "Builder.h"

#include <Engine/Layer/Layer.h>

namespace Mossveil
{
    class MossveilLayer : public SIMPEngine::Layer
    {
    public:
        explicit MossveilLayer(SIMPEngine::Scene *scene)
            : Layer("Mossveil"), m_Scene(scene)
        {
        }

        void OnUpdate(SIMPEngine::TimeStep) override
        {
            GameState &state = State();
            if (!m_Scene || state.PendingLevel == 0)
                return;

            int level = state.PendingLevel;
            uint32_t seed = state.PendingSeed ? state.PendingSeed : state.Seed;
            state.PendingLevel = 0;
            state.PendingSeed = 0;
            BuildLevel(*m_Scene, level, seed);
        }

    private:
        SIMPEngine::Scene *m_Scene = nullptr;
    };
}
