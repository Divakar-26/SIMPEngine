#include "LuaComponents.h"

namespace SIMPEngine::LuaBindings
{
    const std::vector<const ComponentBinding *> &ComponentBindings()
    {
        static const std::vector<const ComponentBinding *> bindings = {
            &SpriteBinding(),
            &CameraBinding(),
            &LightBinding(),
            &VelocityBinding(),
            &CollisionBinding(),
            &ShapeBinding(),
            &TextBinding(),
            &GlowBinding(),
            &ParticlesBinding(),
            &AudioSourceBinding(),
            &ParallaxBinding(),
            &BodyBinding(),
            &TriggerBinding(),
            &TimerBinding(),
            &PathBinding(),
            &HealthBinding(),
            &TilemapBinding(),
            &AnimationBinding(),
        };
        return bindings;
    }
}
