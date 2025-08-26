#include "VFS.h"
#include "FileSystem.h"
#include <filesystem>

using namespace std;
namespace SIMPEngine{
    unordered_map<string ,string> VFS::s_Mounts;

    void VFS::Mount(const string & alias, const string & realPath){
        s_Mounts[alias] = FileSystem::Normalise(realPath);
    }

    bool VFS::UnMount(const string& alias){
        return s_Mounts.erase(alias) > 0;
    }

    optional<string> VFS::Resolve(const string & vp){
        auto pos = vp.find("://");
        if(pos==string::npos) return vp;
        
        string alias = vp.substr(0,pos);
        string rel = vp.substr(pos+3);

        auto it = s_Mounts.find(alias);
        if(it==s_Mounts.end()) return std::nullopt;

        return FileSystem::Normalise(FileSystem::Join(it->second, rel));
    }

    const unordered_map<string, string> & VFS::Mounts() {return s_Mounts;}
}