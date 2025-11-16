#pragma once
#include <Engine/Assets/Asset.h>

#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace SIMPEngine
{
    struct TextureAsset {void * sdlTexture = nullptr; int w = 0, h = 0;};
    struct SceneAsset {};

    class IAssetImporter{
        public:
            virtual ~IAssetImporter() = default;
            virtual bool Supports(AssetType type) const = 0;
            virtual bool Import(const AssetMetadata & meta, void *&outObject) = 0;
    };

    class ImporterRegistry{
        public:
            static void Register(std::unique_ptr<IAssetImporter> importer);
            static IAssetImporter * Find(AssetType type);
        private:
            static std::vector<std::unique_ptr<IAssetImporter>> & List();
    };
}