#pragma once
#include <string>
#include <cstdint>

namespace SIMPEngine
{
    using AssetHandle = uint64_t;

    inline AssetHandle HashPath(const std::string &p)
    {
        uint64_t h = 1469598103934665603ull;
        for (unsigned char c : p)
        {
            h ^= static_cast<uint64_t>(c);
            h *= 1099511628211ull;
        }
        return h ? h : 1;
    }

    enum class AssetType
    {
        Unknown = 0,
        Texture2D,
        Scene,
        Audio,
        Prefab
    };

    struct AssetMetadata
    {
        AssetHandle handle = 0;
        AssetType type = AssetType::Unknown;
        std::string virtualPath;
        std::string sourcePath;

        bool imported = false;
    };

    struct AssetRef
    {
        AssetHandle handle = 0;
        AssetType type = AssetType::Unknown;
        bool isValid() const
        {
            return handle != 0 && type != AssetType::Unknown;
        }
    };
}
