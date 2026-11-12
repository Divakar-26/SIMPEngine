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
    const ComponentBinding &CharacterControllerBinding();
    const ComponentBinding &TilemapBinding();
    const ComponentBinding &AnimationBinding();
    const ComponentBinding &VectorBinding();
    const ComponentBinding &DamageBinding();
    const ComponentBinding &OscillatorBinding();
    const ComponentBinding &SpawnerBinding();
    const ComponentBinding &FollowBinding();
    const ComponentBinding &PickupBinding();
    const ComponentBinding &CheckpointBinding();
    const ComponentBinding &RespawnBinding();
    const ComponentBinding &MusicBinding();
    const ComponentBinding &PickableBinding();
    const ComponentBinding &ButtonBinding();
    const ComponentBinding &TeleporterBinding();
    const ComponentBinding &CameraZoneBinding();
    const ComponentBinding &WorldBoundsBinding();
    const ComponentBinding &CanvasBinding();
    const ComponentBinding &UIRectBinding();
    const ComponentBinding &UIPanelBinding();
    const ComponentBinding &UILabelBinding();
    const ComponentBinding &UIImageBinding();
    const ComponentBinding &UIButtonBinding();
    const ComponentBinding &UILayoutBinding();
    const ComponentBinding &ShaderCanvasBinding();

    const std::vector<const ComponentBinding *> &ComponentBindings();
}
