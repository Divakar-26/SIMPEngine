#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>
#include <Engine/Math/EntityUtility.h>
#include <Engine/Particles/ParticleEmitter.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *ParticlesMeta = "SIMP.Particles";

        int ParticlesPlay(lua_State *L)
        {
            CheckComponent<ParticleEmitterComponent>(L, ParticlesMeta)->playing = true;
            return 0;
        }

        int ParticlesStop(lua_State *L)
        {
            CheckComponent<ParticleEmitterComponent>(L, ParticlesMeta)->playing = false;
            return 0;
        }

        int ParticlesIsPlaying(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<ParticleEmitterComponent>(L, ParticlesMeta)->playing);
            return 1;
        }

        int ParticlesRestart(lua_State *L)
        {
            ParticleEmitterComponent *emitter = CheckComponent<ParticleEmitterComponent>(L, ParticlesMeta);
            ParticleEmitter::Restart(emitter->state);
            emitter->playing = true;
            return 0;
        }

        int ParticlesBurst(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<ParticleEmitterComponent>(L, ParticlesMeta);
            const int count = static_cast<int>(luaL_checkinteger(L, 2));
            TransformComponent *transform = entity->GetScene()->GetRegistry().try_get<TransformComponent>(entity->GetHandle());
            if (!transform || count <= 0)
                return 0;

            ParticleEmitterComponent &emitter = entity->GetComponent<ParticleEmitterComponent>();
            ParticleEmitter::Emit(emitter.effect, emitter.state, ParticleSystem::FrameOf(*transform), count);
            return 0;
        }

        int ParticlesGetRate(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<ParticleEmitterComponent>(L, ParticlesMeta)->effect.Rate);
            return 1;
        }

        int ParticlesSetRate(lua_State *L)
        {
            ParticleEmitterComponent *emitter = CheckComponent<ParticleEmitterComponent>(L, ParticlesMeta);
            emitter->effect.Rate = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        const luaL_Reg ParticlesMethods[] = {
            {"Play", ParticlesPlay},
            {"Stop", ParticlesStop},
            {"IsPlaying", ParticlesIsPlaying},
            {"Restart", ParticlesRestart},
            {"Burst", ParticlesBurst},
            {"GetRate", ParticlesGetRate},
            {"SetRate", ParticlesSetRate},
            {nullptr, nullptr}};
    }

    const ComponentBinding &ParticlesBinding()
    {
        static const ComponentBinding binding{"Particles", ParticlesMeta, ParticlesMethods, &HasOf<ParticleEmitterComponent>, &AddOf<ParticleEmitterComponent>, &RemoveOf<ParticleEmitterComponent>};
        return binding;
    }
}
