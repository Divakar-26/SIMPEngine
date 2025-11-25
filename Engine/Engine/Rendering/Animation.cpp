#include <Engine/Rendering/Animation.h>

namespace SIMPEngine
{

    Animation::Animation(std::shared_ptr<Texture> spritesheet,
                         int frameWidth, int frameHeight,
                         int frameCount, float frameDuration,
                         bool loop)
        : m_Texture(spritesheet),
          m_FrameWidth(frameWidth),
          m_FrameHeight(frameHeight),
          m_FrameCount(frameCount),
          m_FrameDuration(frameDuration),
          m_Loop(loop),
          m_Finished(false),
          m_ElapsedTime(0.0f),
          m_CurrentFrame(0) {}

    void Animation::Update(float deltaTime)
    {
        if (m_Finished || !m_Texture || m_FrameCount <= 1)
            return;

        m_ElapsedTime += deltaTime;
        while (m_ElapsedTime >= m_FrameDuration)
        {
            m_ElapsedTime -= m_FrameDuration;
            m_CurrentFrame++;
            if (m_CurrentFrame >= m_FrameCount)
            {
                if (m_Loop)
                {
                    m_CurrentFrame = 0;
                }
                else
                {
                    m_CurrentFrame = m_FrameCount - 1;
                    m_Finished = true;
                    break;
                }
            }
        }
    }

    void Animation::Draw(float x, float y, float width, float height,
                         SDL_Color tint, float rotation)
    {
        if (!m_Texture)
            return;

        int texW = m_Texture->GetWidth();
        int framesPerRow = texW / m_FrameWidth;

        int frameX = m_CurrentFrame % framesPerRow;
        int frameY = m_CurrentFrame / framesPerRow;

        SDL_FRect src;
        src.x = float(frameX * m_FrameWidth);
        src.y = float(frameY * m_FrameHeight);
        src.w = float(m_FrameWidth);
        src.h = float(m_FrameHeight);

        Sprite tempSprite(m_Texture, src);
        tempSprite.Draw(x, y, width, height, tint, rotation);
    }

    void Animation::Reset()
    {
        m_CurrentFrame = 0;
        m_ElapsedTime = 0.0f;
        m_Finished = false;
    }

    void Animation::SetLooping(bool loop)
    {
        m_Loop = loop;
    }

    bool Animation::isFinished()
    {
        return m_Finished;
    }
}
