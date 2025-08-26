
#pragma once
#include "Importers.h"
#include "Rendering/Renderer.h"
#include "Rendering/TextureManager.h"

namespace SIMPEngine {
class TextureImporter : public IAssetImporter {
public:
    bool Supports(AssetType type) const override { return type==AssetType::Texture2D; }
    bool Import(const AssetMetadata& meta, void*& outObject) override;
};
}