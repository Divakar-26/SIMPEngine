#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include <functional>
#include <string>
#include <vector>

namespace SIMPEngine
{
    class Scene;
    class MotionBuilder;

    struct MotionOptions
    {
        glm::vec2 Position{0.0f};
        float Size = 0.0f;
        glm::vec4 Color{-1.0f};
        glm::vec4 Accent{-1.0f};
        float Duration = 0.0f;
        float Delay = 0.0f;
        float Width = 0.0f;
        float Z = 0.0f;
        float Hold = -1.0f;
        std::vector<glm::vec2> Points;
        std::vector<float> Values;
        std::string Text;
        entt::entity Parent = entt::null;
    };

    enum class MotionInput
    {
        None,
        Points,
        Values,
        Text
    };

    struct MotionPreset
    {
        std::string Name;
        std::string Category;
        std::string Description;
        float Duration = 1.0f;
        MotionInput Input = MotionInput::None;
        std::function<void(MotionBuilder &)> Build;
    };

    namespace MotionPresets
    {
        const std::vector<MotionPreset> &All();
        const MotionPreset *Find(const std::string &name);
        std::vector<std::string> Categories();

        entt::entity Play(Scene &scene, const MotionPreset &preset, const MotionOptions &options = {}, float *length = nullptr);
        std::string Example(const MotionPreset &preset);
        std::vector<float> SampleFunction(const std::function<float(float)> &fn, float from, float to, int count);
    }
}
