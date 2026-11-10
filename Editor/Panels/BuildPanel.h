#pragma once

#include <EditorPanel.h>
#include <Export/GameBuilder.h>

#include <functional>
#include <string>

class BuildPanel : public EditorPanel
{
public:
    void OnImGuiRender() override;

    void Open();
    void SetBeforeBuild(std::function<bool()> callback) { m_BeforeBuild = std::move(callback); }
    void SetOpenSettings(std::function<void()> callback) { m_OpenSettings = std::move(callback); }
    bool IsBuilding() const { return m_Builder.GetState() == GameBuilder::State::Running; }

private:
    void StartBuild(bool runAfter);

    bool m_Open = false;
    bool m_RunAfter = false;
    bool m_Launched = false;
    std::string m_Output;
    std::string m_Message;
    GameBuilder m_Builder;
    std::function<bool()> m_BeforeBuild;
    std::function<void()> m_OpenSettings;
};
