#include "AssetManager.h"
#include "Core/VFS.h"
#include "Core/FileSystem.h"
#include "Importers.h"
#include "Core/Log.h"

namespace SIMPEngine {

void AssetManager::Init(const std::string& rootAssetsDir, const std::string& registryPath){
    m_Root = rootAssetsDir;
    m_RegistryPath = registryPath;
    VFS::Mount("assets", rootAssetsDir);
    FileSystem::CreateDirectories(rootAssetsDir);
    m_Registry.Load(registryPath);
}

void AssetManager::Shutdown(){ m_Registry.Save(m_RegistryPath); m_Cache.clear(); }

AssetHandle AssetManager::ImportIfNeeded(const std::string& vpath, AssetType type) {
    auto real = VFS::Resolve(vpath);
    if (!real) return 0;

    // Registry keeps both paths
    auto handle = m_Registry.GetOrCreateForVirtualPath(vpath, type, *real);

    CORE_INFO("Importing asset vpath={} real={}", vpath, *real);

    m_Registry.Save(m_RegistryPath);
    return handle;
}

void* AssetManager::Get(AssetHandle handle){
    if(!handle) return nullptr;
    auto it = m_Cache.find(handle);
    if(it!=m_Cache.end()) return it->second.object;

    auto* md = m_Registry.Get(handle);
    if(!md) return nullptr;
    auto obj = LoadInternal(*md);
    if(obj) m_Cache[handle] = {obj, 1};
    return obj;
}
void* AssetManager::LoadInternal(const AssetMetadata& md){
    auto* imp = ImporterRegistry::Find(md.type);
    if(!imp) return nullptr;
    void* obj=nullptr;
    if(!imp->Import(md, obj)) return nullptr;
    return obj;
}
}
