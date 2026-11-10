#pragma once

#include "RecentProjects.h"

#include <Engine/Layer/Layer.h>
#include <Engine/Project/Project.h>

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

class ProjectHubLayer : public SIMPEngine::Layer
{
public:
    using OpenCallback = std::function<bool(const std::filesystem::path &, std::string &)>;

    explicit ProjectHubLayer(OpenCallback onOpen);

    void OnAttach() override;
    void OnRender() override;

    void ShowError(const std::string &message);

private:
    enum class Page
    {
        Projects,
        Learn
    };

    struct Guide
    {
        std::string Title;
        std::string Summary;
        std::filesystem::path File;
    };

    void DrawSidebar(float width, float height);
    void DrawProjects();
    void DrawProjectRow(int index, const RecentProject &project, bool missing);
    void DrawLearn();
    void DrawNewProjectModal();
    void DrawOpenModal();
    void DrawErrorModal();
    void Open(const std::filesystem::path &path);
    void FindGuides();

    OpenCallback m_OnOpen;
    RecentProjects m_Recent;
    Page m_Page = Page::Projects;
    int m_Selected = -1;
    char m_Filter[128] = "";

    bool m_ShowNew = false;
    std::string m_NewName = "My Game";
    std::string m_NewLocation;
    int m_NewTemplate = 1;

    bool m_ShowOpen = false;
    std::string m_OpenPath;

    bool m_ShowError = false;
    std::string m_Error;

    std::vector<Guide> m_Guides;
};
