#include "AssetRegistry.h"
#include "Core/FileSystem.h"
#include <sstream>
#include <iomanip>

using namespace std;
namespace SIMPEngine
{
    bool AssetRegistry::Load(const string &path)
    {
        m_Map.clear();
        auto txt = FileSystem::ReadText(path);
        if (!txt)
            return false;

        istringstream ss(*txt);
        string line;
        while (getline(ss, line))
        {
            if (line.empty() || line[0] == '#')
                continue;
            AssetMetadata md;
            string typeStr, importedStrl;
            stringstream ls(line);
            getline(ls, (line = string()), '\n');
        }

        ss.clear();
        ss.str(*txt);
        while (getline(ss, line))
        {
            if (line.empty() || line[0] == '#')
                continue;
            string handleStr, typeStr, vp, rp, imp;
            stringstream ls(line);
            if (!getline(ls, handleStr, '|'))
                continue;
            if (!getline(ls, typeStr, '|'))
                continue;
            if (!getline(ls, vp, '|'))
                continue;
            if (!getline(ls, rp, '|'))
                continue;
            if (!getline(ls, imp, '|'))
                imp = "0";

            AssetMetadata md;
            md.handle = std::stoull(handleStr);
            md.type = static_cast<AssetType>(std::stoul(typeStr));
            md.virtualPath = vp;
            md.sourcePath = rp;
            md.imported = (imp == "1");
            m_Map[md.handle] = md;
        }
        return true;
    }

    bool AssetRegistry::Save(const string &path) const
    {
        ostringstream ss;
        ss << "# handle|type|virtualPath|realPath|imported\n";
        for (auto &[h, md] : m_Map)
        {
            ss << h << '|' << (unsigned)md.type << '|' << md.virtualPath << '|' << md.sourcePath << '|' << (md.imported ? 1 : 0) << '\n';
        }
        return FileSystem::WriteText(path, ss.str());
    }

    AssetMetadata *AssetRegistry::Get(AssetHandle h)
    {
        auto it = m_Map.find(h);
        return it == m_Map.end() ? nullptr : &it->second;
    }

    const AssetMetadata *AssetRegistry::Get(AssetHandle h) const
    {
        auto it = m_Map.find(h);
        return it == m_Map.end() ? nullptr : &it->second;
    }

    AssetMetadata &AssetRegistry::Upsert(const AssetMetadata &md)
    {
        auto it = m_Map.find(md.handle);
        if (it == m_Map.end())
            it = m_Map.emplace(md.handle, md).first;
        else
            it->second = md;
        return it->second;
    }

    bool AssetRegistry::Remove(AssetHandle h) { return m_Map.erase(h) > 0; }

    AssetHandle AssetRegistry::GetOrCreateForVirtualPath(const string &vpath, AssetType type, const string &realPath)
    {
        AssetHandle h = HashPath(vpath);
        auto it = m_Map.find(h);
        if (it == m_Map.end())
        {
            AssetMetadata md;
            md.handle = h;
            md.type = type;
            md.virtualPath = vpath;
            md.sourcePath = realPath;
            md.imported = false;
            m_Map.emplace(h, md);
        }
        return h;
    }

}