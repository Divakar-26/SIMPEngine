#include <Engine/Rendering/SpriteAtlas.h>
#include <Engine/Core/Log.h>

namespace SIMPEngine
{
    bool SpriteAtlas::Load(const std::string &texturePath)
    {
        m_Texture = std::make_shared<Texture>();
        if (!m_Texture->LoadFromFile(texturePath.c_str()))
        {
            CORE_ERROR("Failed to load texture for atlas: {}", texturePath);
            return false;
        }
        // SDL_SetTextureScaleMode(m_Texture->GetSDLTexture(), SDL_SCALEMODE_NEAREST);
        return true;
    }

    void SpriteAtlas::AddFrame(const std::string &name, const SDL_FRect &rect)
    {
        m_Frames[name] = {rect};
    }

    Sprite SpriteAtlas::GetSprite(const std::string &name) const
    {
        auto it = m_Frames.find(name);
        if (it == m_Frames.end())
        {
            CORE_ERROR("Frame '{}' not found in atlas!", name);
            return Sprite();
        }

        return Sprite(m_Texture, it->second.sourceRect);
    }

    std::vector<Sprite> SpriteAtlas::GetAnimationFrames(const std::vector<std::string> &names) const
    {
        std::vector<Sprite> frames;
        frames.reserve(names.size());

        for (const auto &name : names)
        {
            auto it = m_Frames.find(name);
            if (it != m_Frames.end())
            {
                frames.emplace_back(m_Texture, it->second.sourceRect);
            }
            else
            {
                CORE_ERROR("Frame '{}' not found for animation!", name);
            }
        }
        return frames;
    }
}