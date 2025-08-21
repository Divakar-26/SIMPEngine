#include "Animation.h"

namespace SIMPEngine
{
    Animation::Animation(const std::vector<Sprite> &frames, float frameDuration, bool loop) :  m_Frames(frames), m_FrameDuration(frameDuration), m_Loop(loop), m_Finished(false), m_ElapsedTime(0.0f), m_CurrentFrame(0) {}

    void Animation::Update(float deltaTime)
    {
        if (m_Finished || m_Frames.empty())
        {
            return;
        }

        m_ElapsedTime += deltaTime;

        while (m_ElapsedTime >= m_FrameDuration)
        {
            m_ElapsedTime -= m_FrameDuration;
            m_CurrentFrame++;

            if (m_CurrentFrame >= m_Frames.size())
            {
                if (m_Loop)
                {
                    m_CurrentFrame = 0;
                }
                else
                {
                    m_CurrentFrame = m_Frames.size() - 1;
                    m_Finished = true;
                    break;
                }
            }
        }
    }

    void Animation::Draw(float x, float y, float width, float height, SDL_Color tint, float rotation)
    {
        if (m_Frames.empty())
            return;

        m_Frames[m_CurrentFrame].Draw(x, y, width, height, tint, rotation);
    }

    void Animation::Reset()
    {
        m_CurrentFrame = 0;
        m_ElapsedTime = 0.0f;
        m_Finished = false;
    }

    void Animation::SetLooping(bool loop){
        m_Loop = loop;
    }

    bool Animation::isFinished(){
        return m_Finished;
    }
} // namespace SIMPEngine
