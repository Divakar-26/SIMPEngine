#include "FileDialogs.h"

#include <Engine/Application/Application.h>
#include <Engine/Core/Log.h>

#include <SDL3/SDL.h>

#include <map>
#include <mutex>
#include <vector>

namespace EditorUI
{
    namespace
    {
        std::mutex s_Mutex;
        std::map<std::string, std::string> s_Picked;
        std::vector<SDL_Process *> s_Processes;

        void FolderPicked(void *userdata, const char *const *files, int)
        {
            std::string *id = static_cast<std::string *>(userdata);
            if (files && files[0])
            {
                std::lock_guard<std::mutex> lock(s_Mutex);
                s_Picked[*id] = files[0];
            }
            else if (!files)
                CORE_WARN("Folder dialog failed: {}", SDL_GetError());
            delete id;
        }
    }

    void PickFolder(const std::string &id, const std::string &startFolder)
    {
        SDL_Window *window = SIMPEngine::Application::Get().GetWindow().GetNativeWindow();
        SDL_ShowOpenFolderDialog(FolderPicked, new std::string(id), window, startFolder.empty() ? nullptr : startFolder.c_str(), false);
    }

    std::optional<std::string> TakePickedFolder(const std::string &id)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        auto it = s_Picked.find(id);
        if (it == s_Picked.end())
            return std::nullopt;
        std::string folder = it->second;
        s_Picked.erase(it);
        return folder;
    }

    void OpenInFileManager(const std::string &path)
    {
        const std::string url = "file://" + path;
        if (!SDL_OpenURL(url.c_str()))
            CORE_WARN("Could not open {}: {}", path, SDL_GetError());
    }

    bool LaunchProcess(const std::string &executable, const std::string &argument)
    {
        for (auto it = s_Processes.begin(); it != s_Processes.end();)
        {
            if (SDL_WaitProcess(*it, false, nullptr))
            {
                SDL_DestroyProcess(*it);
                it = s_Processes.erase(it);
            }
            else
                ++it;
        }

        const char *args[3] = {executable.c_str(), argument.empty() ? nullptr : argument.c_str(), nullptr};
        SDL_Process *process = SDL_CreateProcess(args, false);
        if (!process)
        {
            CORE_ERROR("Could not start {}: {}", executable, SDL_GetError());
            return false;
        }
        s_Processes.push_back(process);
        return true;
    }
}
