#include <Engine/Rendering/TextureManager.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>
#include <Engine/Core/FileSystem.h>

#include <Engine/Rendering/GL.h>

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
            if (m_Placeholders.count(id) == 0)
                return it->second;
            std::string retryPath = filePath;
            if (auto resolved = VFS::Resolve(filePath))
                retryPath = *resolved;
            if (!FileSystem::Exists(retryPath))
                return it->second;
            m_Placeholders.erase(id);
            m_TextureMap.erase(it);
        }

        std::string realPath = filePath;
        if (auto resolved = VFS::Resolve(filePath))
            realPath = *resolved;
        else
            CORE_WARN("Failed to resolve VFS path '{}'", filePath);

        std::shared_ptr<Texture> texture = std::make_shared<Texture>();
        if (!texture->LoadFromFile(realPath.c_str()))
        {
            LOG_ERROR("Failed to load texture: {} (using a placeholder)", filePath);
            if (!glGenTextures)
                return nullptr;
            constexpr int Size = 16;
            unsigned char pixels[Size * Size * 4];
            for (int y = 0; y < Size; ++y)
                for (int x = 0; x < Size; ++x)
                {
                    const bool bright = ((x / 4) + (y / 4)) % 2 == 0;
                    unsigned char *p = pixels + (y * Size + x) * 4;
                    p[0] = bright ? 255 : 48;
                    p[1] = 0;
                    p[2] = bright ? 255 : 48;
                    p[3] = 255;
                }
            texture = std::make_shared<Texture>();
            texture->UploadToGPU(pixels, Size, Size, 4);
            texture->SetVFSPath(filePath);
            m_TextureMap[id] = texture;
            m_Placeholders.insert(id);
            return texture;
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
        m_Placeholders.clear();
        LOG_INFO("Unloaded all textures");
    }

    TextureManager::~TextureManager()
    {
        UnloadAll();
    }
}
