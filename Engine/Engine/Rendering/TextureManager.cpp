#include <Engine/Rendering/TextureManager.h>
#include <Engine/Core/Log.h>

namespace SIMPEngine {

    TextureManager& TextureManager::Get() {
        static TextureManager instance;
        return instance;
    }

    std::shared_ptr<Texture> TextureManager::LoadTexture(const std::string& id, const std::string& filePath, SDL_Renderer * renderer) {
        auto it = m_TextureMap.find(id);
        if (it != m_TextureMap.end()) {
            return it->second; 
        }

        std::shared_ptr<Texture> texture = std::make_shared<Texture>();
        if (!texture->LoadFromFile(renderer, filePath.c_str())) {
            LOG_ERROR("Failed to load texture: {}", filePath);
            return nullptr;
        }

        m_TextureMap[id] = texture;
        LOG_INFO("Loaded texture: {}", id);
        SDL_SetTextureScaleMode(texture->GetSDLTexture(), SDL_SCALEMODE_NEAREST);

        return texture;
    }

    std::shared_ptr<Texture> TextureManager::GetTexture(const std::string& id) {
        auto it = m_TextureMap.find(id);
        if (it != m_TextureMap.end()) {
            return it->second;
        }
        LOG_WARN("Texture not found: {}", id);
        return nullptr;
    }

    void TextureManager::UnloadTexture(const std::string& id) {
        m_TextureMap.erase(id);
        LOG_INFO("Unloaded texture: {}", id);
    }

    void TextureManager::UnloadAll() {
        m_TextureMap.clear();
        LOG_INFO("Unloaded all textures");
    }

    TextureManager::~TextureManager() {
        UnloadAll();
    }

}
