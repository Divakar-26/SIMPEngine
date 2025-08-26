#pragma once
#include "Asset.h"
#include  <unordered_map>
#include <string>

namespace SIMPEngine{
    class AssetRegistry{
        public:
        bool Load(const std::string & path);
        bool Save(const std::string & path) const;

        bool Has(AssetHandle h) const{
            return m_Map.count(h)>0;
        }
        
        AssetMetadata * Get(AssetHandle h);
        const AssetMetadata * Get(AssetHandle h) const;

        AssetMetadata& Upsert(const AssetMetadata& md);
        bool Remove(AssetHandle h);

        AssetHandle GetOrCreateForVirtualPath(const std::string & vpath, AssetType type, const std::string & realPath);
        const std::unordered_map<AssetHandle,AssetMetadata>& All() const{return m_Map;}

        private:
            std::unordered_map<AssetHandle, AssetMetadata> m_Map;
    };
}