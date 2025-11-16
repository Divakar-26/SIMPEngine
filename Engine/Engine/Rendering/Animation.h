#pragma once

#include <Engine/Rendering/Sprite.h>

    
#include<SDL3/SDL.h>
#include<vector>
#include<memory>


namespace SIMPEngine{
    class Animation{
        public:
            Animation(const std::vector<Sprite>& frames, float frameDuration, bool loop = true);

            void Update(float deltaTime);

            void Draw(float x, float y, float width, float height, SDL_Color hint = {255,255,255,255}, float rotation = 0.0f);

            void Reset();
            void SetLooping(bool loop);
            bool isFinished();
            
        private:

        std::shared_ptr<Texture> m_Texture;
        std::vector<Sprite> m_Frames;

        float m_FrameDuration;
        bool m_Loop;
        bool m_Finished;

        float m_ElapsedTime;
        size_t m_CurrentFrame;
    };
}