#pragma once

#include <Engine/Scene/Component.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/ScriptableEntity.h>
#include <Engine/Rendering/TextureManager.h>

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Mossveil
{
    constexpr int LevelCount = 3;

    struct GameState
    {
        int Level = 1;
        int PendingLevel = 0;
        uint32_t Seed = 7;
        uint32_t PendingSeed = 0;
        int Wisps = 0;
        int TotalWisps = 0;
        int RunWisps = 0;
        bool HasDash = false;
        float Fade = 1.0f;
        bool PlayerDead = false;
        bool Exiting = false;
        float LevelTime = 0.0f;
        glm::vec2 Checkpoint{0.0f};
        glm::vec2 BoundsMin{0.0f};
        glm::vec2 BoundsMax{0.0f};
        std::string Message;
        float MessageTime = 0.0f;
    };

    inline GameState &State()
    {
        static GameState s_State;
        return s_State;
    }

    struct Box
    {
        glm::vec2 Min{0.0f};
        glm::vec2 Max{0.0f};

        bool Overlaps(const Box &o) const
        {
            return Min.x < o.Max.x && Max.x > o.Min.x && Min.y < o.Max.y && Max.y > o.Min.y;
        }
    };

    inline Box ColliderBox(const TransformComponent &transform, const CollisionComponent &collision)
    {
        glm::vec2 min = transform.position + glm::vec2(collision.offsetX, collision.offsetY);
        return {min, min + glm::vec2(collision.width, collision.height)};
    }

    inline float Approach(float value, float target, float step)
    {
        if (value < target)
            return std::min(value + step, target);
        return std::max(value - step, target);
    }

    inline std::shared_ptr<SIMPEngine::Texture> Tex(const std::string &path)
    {
        auto texture = SIMPEngine::TextureManager::Get().LoadTexture(path, path);
        if (texture)
        {
            texture->SetVFSPath(path);
            texture->SetFilter(SIMPEngine::TextureFilter::Linear);
        }
        return texture;
    }

    inline std::string PiecePath(const std::string &name)
    {
        return "assets://mossveil/pieces/" + name + ".png";
    }

    struct SheetInfo
    {
        const char *Name;
        int FrameWidth;
        int FrameHeight;
        int Frames;
        int Columns;
    };

    inline std::string SheetPath(const SheetInfo &sheet)
    {
        return std::string("assets://mossveil/") + sheet.Name + ".png";
    }

    inline SDL_FRect SheetFrame(const SheetInfo &sheet, int index)
    {
        index = std::clamp(index, 0, sheet.Frames - 1);
        return {static_cast<float>((index % sheet.Columns) * sheet.FrameWidth), static_cast<float>((index / sheet.Columns) * sheet.FrameHeight),
                static_cast<float>(sheet.FrameWidth), static_cast<float>(sheet.FrameHeight)};
    }

    namespace Sheets
    {
        inline constexpr SheetInfo Idle{"wizard_idle", 120, 128, 20, 10};
        inline constexpr SheetInfo Walk{"wizard_walk", 120, 128, 20, 10};
        inline constexpr SheetInfo Jump{"wizard_jump", 120, 128, 8, 8};
        inline constexpr SheetInfo Dash{"wizard_dash", 120, 128, 16, 10};

        inline constexpr SheetInfo PlantTall{"plant_tall", 89, 160, 45, 10};
        inline constexpr SheetInfo PlantCurl{"plant_curl", 195, 160, 45, 10};
        inline constexpr SheetInfo PlantTuft{"plant_tuft", 199, 160, 45, 10};
        inline constexpr SheetInfo PlantBulb{"plant_bulb", 50, 160, 30, 10};
        inline constexpr SheetInfo PlantSway{"plant_sway", 85, 160, 30, 10};
        inline constexpr SheetInfo PlantFern{"plant_fern", 67, 160, 30, 10};
        inline constexpr SheetInfo PlantReed{"plant_reed", 82, 160, 30, 10};
        inline constexpr SheetInfo PlantWind{"plant_wind", 162, 160, 30, 10};
        inline constexpr SheetInfo FlowerBlue{"flower_blue", 153, 160, 30, 10};
        inline constexpr SheetInfo FlowerCap{"flower_cap", 166, 160, 30, 10};
    }

    inline const char *LevelTitle(int level)
    {
        switch (level)
        {
        case 1:
            return "The Mossway";
        case 2:
            return "Thornroot Depths";
        default:
            return "The Shifting Deep";
        }
    }

    inline const char *LevelNumeral(int level)
    {
        switch (level)
        {
        case 1:
            return "I";
        case 2:
            return "II";
        default:
            return "III";
        }
    }

    inline SIMPEngine::Entity FindByTag(SIMPEngine::Scene &scene, const std::string &tag)
    {
        return scene.GetEntityByName(tag);
    }
}
