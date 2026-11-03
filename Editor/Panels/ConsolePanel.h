#pragma once

#include <EditorPanel.h>

class ConsolePanel : public EditorPanel
{
public:
    static constexpr const char *WindowId = "###Console";

    void OnImGuiRender() override;

    bool IsVisible() const { return m_Visible; }
    bool *VisibleFlag() { return &m_Visible; }

private:
    bool m_Visible = true;
    bool m_ShowTrace = false;
    bool m_ShowInfo = true;
    bool m_ShowWarn = true;
    bool m_ShowError = true;
    bool m_AutoScroll = true;
    char m_Filter[128] = "";
};
