#include <Engine/Rendering/AnimationController.h>
#include <Engine/Core/Log.h>

namespace SIMPEngine{
    void AnimationController::AddClip(const std::string & name, std::shared_ptr<Animation> clip){
        m_Clips[name] = std::move(clip);
    }

    void AnimationController::Play(const std::string & name){
        auto it = m_Clips.find(name);
        if(it == m_Clips.end()){
            CORE_ERROR("AnimationController: clip '{}' is not found", name);
            return;
        }

        m_CurrentName = name;
        m_Current = it->second.get();
        m_Current->Reset();
    }

    void AnimationController::Update(float deltaTime){
        if(m_Current)
            m_Current->Update(deltaTime);
    }

    void AnimationController::Draw(float x, float y, float w, float h, SDL_Color tint, float rotation){
        if(m_Current)
            m_Current->Draw(x, y , w, h, tint, rotation);
    }

    std::vector<std::string> AnimationController::GetClipNames() const{
        std::vector<std::string> names;
        names.reserve(m_Clips.size());
        for(auto& [k, _] : m_Clips) names.push_back(k);
        return names;
    }
}