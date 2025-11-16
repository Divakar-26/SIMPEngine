#pragma once
#include <Engine/Rendering/Texture.h>

#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>
#include <memory>

namespace SIMPEngine
{

    class TextureManager
    {
    public:
        static TextureManager &Get();

        std::shared_ptr<Texture> LoadTexture(const std::string &id, const std::string &filePath, SDL_Renderer *renderer);

        std::shared_ptr<Texture> GetTexture(const std::string &id);

        void UnloadTexture(const std::string &id);

        void UnloadAll();

    private:
        TextureManager() = default;
        ~TextureManager();

        std::unordered_map<std::string, std::shared_ptr<Texture>> m_TextureMap;
    };

}
