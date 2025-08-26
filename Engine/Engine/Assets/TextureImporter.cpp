#include "TextureImporter.h"
#include "Core/VFS.h"
#include "Core/FileSystem.h"
#include "Core/Log.h"

namespace SIMPEngine {
bool TextureImporter::Import(const AssetMetadata& meta, void*& outObject){
    auto real = VFS::Resolve(meta.virtualPath);
    if(!real || !FileSystem::Exists(*real)) return false;
    auto tex = TextureManager::Get().LoadTexture(meta.virtualPath.c_str(), (*real).c_str(), Renderer::GetSDLRenderer());
    if(!tex) return false;
    outObject = tex.get(); // or store a strong ref somewhere
    return true;
}
}