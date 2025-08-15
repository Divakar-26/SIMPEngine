#pragma once
#include "Rendering/Texture.h"
#include"Rendering/Sprite.h"
#include <memory>

struct SpriteComponent {
    std::shared_ptr<SIMPEngine::Sprite> sprite; // your Sprite class
    float width;
    float height;
};


