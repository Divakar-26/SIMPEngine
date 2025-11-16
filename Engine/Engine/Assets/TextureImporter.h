#pragma once
#include <Engine/Assets/Importers.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Rendering/TextureManager.h>

namespace SIMPEngine {
class TextureImporter : public IAssetImporter {
public:
    bool Supports(AssetType type) const override { return type==AssetType::Texture2D; }
    bool Import(const AssetMetadata& meta, void*& outObject) override;
};
}