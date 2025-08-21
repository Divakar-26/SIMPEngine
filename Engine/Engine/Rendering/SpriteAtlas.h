#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <SDL3/SDL.h>
#include "Texture.h"
#include "Sprite.h"

namespace SIMPEngine
{

    struct AtlasFrame
    {
        SDL_FRect sourceRect;
    };

    class SpriteAtlas
    {
    public:
        SpriteAtlas() = default;

        bool Load(SDL_Renderer *renderer, const std::string &texturePath);

        void AddFrame(const std::string &name, const SDL_FRect &rect);
        Sprite GetSprite(const std::string &name) const;

        std::vector<Sprite> GetAnimationFrames(const std::vector<std::string> &names) const;

        std::shared_ptr<Texture> GetTexture() const { return m_Texture; }

    private:
        std::shared_ptr<Texture> m_Texture;
        std::unordered_map<std::string, AtlasFrame> m_Frames;
    };
}