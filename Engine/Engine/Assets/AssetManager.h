#pragma once
#include "AssetRegistry.h"
#include <unordered_map>

namespace SIMPEngine {
class AssetManager {
public:
    void Init(const std::string& rootAssetsDir, const std::string& registryPath);
    void Shutdown();

    AssetHandle ImportIfNeeded(const std::string& vpath, AssetType type);
    void* Get(AssetHandle handle); // raw object (texture*, etc)
    template<typename T> T* GetAs(AssetHandle h){ return static_cast<T*>(Get(h)); }

    const AssetRegistry& Registry() const { return m_Registry; }
    AssetRegistry& Registry() { return m_Registry; }

private:
    std::string m_Root;
    std::string m_RegistryPath;
    AssetRegistry m_Registry;

    struct CacheItem { void* object=nullptr; int refcount=0; };
    std::unordered_map<AssetHandle, CacheItem> m_Cache;

    void* LoadInternal(const AssetMetadata& md);
};
}
