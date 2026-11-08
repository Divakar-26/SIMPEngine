#pragma once

#include <Engine/Scripting/LuaHelpers.h>

#include <vector>

namespace SIMPEngine::LuaBindings
{
    const ComponentBinding &SpriteBinding();
    const ComponentBinding &CameraBinding();
    const ComponentBinding &LightBinding();
    const ComponentBinding &VelocityBinding();
    const ComponentBinding &CollisionBinding();
    const ComponentBinding &ShapeBinding();
    const ComponentBinding &TextBinding();
    const ComponentBinding &GlowBinding();
    const ComponentBinding &ParticlesBinding();
    const ComponentBinding &AudioSourceBinding();
    const ComponentBinding &ParallaxBinding();
    const ComponentBinding &BodyBinding();
    const ComponentBinding &TriggerBinding();
    const ComponentBinding &TimerBinding();
    const ComponentBinding &PathBinding();
    const ComponentBinding &HealthBinding();
    const ComponentBinding &TilemapBinding();
    const ComponentBinding &AnimationBinding();
    const ComponentBinding &VectorBinding();

    const std::vector<const ComponentBinding *> &ComponentBindings();
}
