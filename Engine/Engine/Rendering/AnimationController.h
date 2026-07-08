#pragma once
#include <Engine/Rendering/Animation.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>


namespace SIMPEngine
{
    class AnimationController{
        public:

            void AddClip(const std::string& name, std::shared_ptr<Animation> clip);
            void Play(const std::string & name);
            void TryPlay(const std::string& name);
            void Update(float deltaTime);

            void Draw(float x, float y, float w, float h, SDL_Color tint = {255,255,255,255}, float rotation = 0.0f);

            const std::string& GetCurrentClipName() const{return m_CurrentName;}
            Animation* GetCurrentClip() const {return m_Current;}

            std::vector<std::string> GetClipNames() const;

        private:
            std::unordered_map<std::string, std::shared_ptr<Animation>> m_Clips;

            std::string m_CurrentName;
            Animation * m_Current = nullptr;
    }; 
} // namespace SIMPEngine
