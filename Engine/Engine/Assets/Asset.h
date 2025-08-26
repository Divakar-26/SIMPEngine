#pragma once
#include <string>
#include <cstdint>

namespace SIMPEngine
{
    using AssetHandle = uint64_t;

    inline AssetHandle HashPath(const std::string &p) //takes the virtual path (assets://texture/a.png) and produces a stabel 64-bit handle
    {
        uint64_t h = 1469598103934665603ull;
        for (unsigned char c : p)
        {
            h ^= c;
            h *= 109951162821ull;
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

    //this stores everything that asset manager needs to knwo about an asset, its type, virtual path, actual path.
    struct AssetMetadata
    {
        AssetHandle handle = 0;
        AssetType type = AssetType::Unknown;
        std::string virtualPath;
        std::string sourcePath;

        bool imported = false;
    };

    //lightweight refrence to an asset
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