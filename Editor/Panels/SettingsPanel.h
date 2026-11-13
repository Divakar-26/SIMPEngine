#pragma once

#include <EditorPanel.h>

#include "Android/AndroidTools.h"

#include <functional>

class SettingsPanel : public EditorPanel
{
public:
    void OnImGuiRender() override;

    void Open(int category = -1)
    {
        m_Open = true;
        if (category >= 0)
            m_Category = category;
    }
    void SetAndroid(AndroidSession *session) { m_Android = session; }
    bool ConsumeAndroidRescan()
    {
        bool pending = m_AndroidRescan;
        m_AndroidRescan = false;
        return pending;
    }
    void SetResetLayout(std::function<void()> callback) { m_ResetLayout = std::move(callback); }
    bool ConsumeApplyRequest()
    {
        bool pending = m_ApplyRequested;
        m_ApplyRequested = false;
        return pending;
    }

private:
    void DrawGeneral(bool &apply, bool &dirty);
    void DrawAppearance(bool &apply, bool &dirty);
    void DrawViewport(bool &apply, bool &dirty);
    void DrawEditing(bool &apply, bool &dirty);
    void DrawPlay(bool &apply, bool &dirty);
    void DrawScripts(bool &apply, bool &dirty);
    void DrawAndroid(bool &apply, bool &dirty);
    void DrawMaintenance(bool &apply, bool &dirty);

    bool m_Open = false;
    bool m_ApplyRequested = false;
    int m_Category = 0;
    bool m_ConfirmReset = false;
    bool m_AndroidRescan = false;
    AndroidSession *m_Android = nullptr;
    std::function<void()> m_ResetLayout;
};
