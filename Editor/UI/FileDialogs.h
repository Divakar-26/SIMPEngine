#pragma once

#include <optional>
#include <string>

namespace EditorUI
{
    void PickFolder(const std::string &id, const std::string &startFolder = {});
    std::optional<std::string> TakePickedFolder(const std::string &id);
    void OpenInFileManager(const std::string &path);
    bool LaunchProcess(const std::string &executable, const std::string &argument = {});
}
