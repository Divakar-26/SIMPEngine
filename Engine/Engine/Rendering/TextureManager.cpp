#include <Engine/Rendering/TextureManager.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>

namespace SIMPEngine
{
    TextureManager &TextureManager::Get()
    {
        static TextureManager instance;
        return instance;
    }

    std::shared_ptr<Texture> TextureManager::LoadTexture(const std::string &id, const std::string &filePath)
    {
        auto it = m_TextureMap.find(id);
        if (it != m_TextureMap.end())
        {
            return it->second;
        }

        std::string realPath = filePath;
        if (auto resolved = VFS::Resolve(filePath))
            realPath = *resolved;
        else
            CORE_WARN("Failed to resolve VFS path '{}'", filePath);

        std::shared_ptr<Texture> texture = std::make_shared<Texture>();
        if (!texture->LoadFromFile(realPath.c_str()))
        {
            LOG_ERROR("Failed to load texture: {}", filePath);
            return nullptr;
        }

        m_TextureMap[id] = texture;
        m_TextureMap[id]->SetVFSPath(filePath);
        LOG_INFO("Loaded texture: {}", id);

        return texture;
    }

    std::shared_ptr<Texture> TextureManager::GetTexture(const std::string &id)
    {
        auto it = m_TextureMap.find(id);
        if (it != m_TextureMap.end())
        {
            return it->second;
        }
        LOG_WARN("Texture not found: {}", id);
        return nullptr;
    }

    void TextureManager::UnloadTexture(const std::string &id)
    {
        m_TextureMap.erase(id);
        LOG_INFO("Unloaded texture: {}", id);
    }

    void TextureManager::UnloadAll()
    {
        m_TextureMap.clear();
        LOG_INFO("Unloaded all textures");
    }

    TextureManager::~TextureManager()
    {
        UnloadAll();
    }
}
