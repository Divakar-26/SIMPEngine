#pragma once

#include <EditorPanel.h>

#include "Docs/MarkdownView.h"

#include <string>
#include <utility>
#include <vector>

class DocsPanel : public EditorPanel
{
public:
    static constexpr const char *WindowId = "###Docs";

    void OnImGuiRender() override;

    bool IsVisible() const { return m_Visible; }
    bool *VisibleFlag() { return &m_Visible; }
    void Open(const std::string &file = {}, const std::string &anchor = {});

private:
    struct Entry
    {
        std::string File;
        std::string Title;
        std::string Path;
        Markdown::Document Document;
        std::vector<std::pair<std::string, std::string>> Lines;
    };

    struct Location
    {
        int Doc = 0;
        std::string Anchor;
    };

    struct Hit
    {
        int Doc = 0;
        std::string Anchor;
        std::string Heading;
        std::string Snippet;
    };

    void Load();
    void Show(int doc, const std::string &anchor, bool record);
    void Navigate(const std::string &url);
    void Search();
    void DrawSidebar(float width);
    void DrawGuides();
    void DrawApi();

    bool m_Visible = false;
    bool m_Loaded = false;
    std::string m_Folder;
    std::vector<Entry> m_Entries;
    int m_Current = -1;
    MarkdownView m_View;
    std::vector<Location> m_History;
    int m_Position = -1;
    char m_Query[96] = {};
    std::string m_LastQuery;
    std::vector<Hit> m_Hits;
    int m_Tab = 0;
    std::string m_Pending;
    std::string m_PendingAnchor;
    bool m_HasPending = false;

    char m_ApiQuery[96] = {};
    std::vector<std::pair<std::string, std::string>> m_Api;
    int m_ApiSelected = -1;
};
