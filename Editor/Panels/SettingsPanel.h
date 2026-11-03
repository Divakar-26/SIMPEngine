#pragma once

#include <EditorPanel.h>

class SettingsPanel : public EditorPanel
{
public:
    void OnImGuiRender() override;

    void Open() { m_Open = true; }
    bool ConsumeApplyRequest()
    {
        bool pending = m_ApplyRequested;
        m_ApplyRequested = false;
        return pending;
    }

private:
    bool m_Open = false;
    bool m_ApplyRequested = false;
};
