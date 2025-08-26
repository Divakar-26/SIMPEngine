#pragma once
#include <string>
#include <unordered_map>
#include <optional>

namespace SIMPEngine{
    class VFS
    {   
        public:
            static void Mount(const std::string & alias, const std::string & realPath); //this is where the virtual path will be mounted

            static bool UnMount(const std::string & alias);
            static std::optional<std::string> Resolve(const std::string & virtualPath);
            static const std::unordered_map<std::string , std::string> & Mounts();

            static std::unordered_map<std::string, std::string> s_Mounts; //all the virutal path will be stored here
    };
    
};