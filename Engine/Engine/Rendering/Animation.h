#pragma once

#include <Engine/Rendering/Sprite.h>

#include <SDL3/SDL.h>
#include <memory>

namespace SIMPEngine {

    class Animation {
    public:
        Animation() = default;
        Animation(std::shared_ptr<Texture> spritesheet,
                  int frameWidth, int frameHeight,
                  int frameCount, float frameDuration,
                  bool loop = true);

        void Update(float deltaTime);
        void Draw(float x, float y, float width, float height,
                  SDL_Color tint = {255, 255, 255, 255},
                  float rotation = 0.0f);
        
        int GetFrameNumber() {return m_FrameCount;}
        int GetFrameWidth() {return m_FrameWidth;}
        int GetFrameHeight() {return m_FrameHeight;}
        float GetFrameDuration() {return m_FrameDuration;}
        bool IsLooping() {return m_Loop;}
        std::shared_ptr<Texture> GetTexture() { return m_Texture;}

        void Reset();
        void SetLooping(bool loop);
        bool isFinished();

    private:
        std::shared_ptr<Texture> m_Texture = nullptr;

        int m_FrameWidth = 0;
        int m_FrameHeight = 0;
        int m_FrameCount = 0;

        float m_FrameDuration = 0.1f;
        bool m_Loop = true;
        bool m_Finished = false;

        float m_ElapsedTime = 0.0f;
        int m_CurrentFrame = 0;
    };
}
