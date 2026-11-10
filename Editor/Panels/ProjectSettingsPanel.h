#pragma once

#include <EditorPanel.h>

#include <string>
#include <vector>

class ProjectSettingsPanel : public EditorPanel
{
public:
    void OnImGuiRender() override;

    void Open();
    void SetCurrentScene(const std::string &vpath) { m_CurrentScene = vpath; }

private:
    void RefreshScenes();

    bool m_Open = false;
    bool m_Dirty = false;
    std::string m_CurrentScene;
    std::vector<std::string> m_Scenes;
    std::string m_Error;
};
