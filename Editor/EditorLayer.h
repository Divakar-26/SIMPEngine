#pragma once
#include <Engine/Layer/Layer.h>
#include <Engine/Core/TimeStep.h>
#include <Engine/Layer/RenderingLayer.h>
#include <Engine/Events/Event.h>
#include <Engine/Assets/AssetManager.h>
#include <Engine/Scene/SceneSerializer.h>
#include "Panels/ViewportPanel.h"
#include "Panels/GameViewPanel.h"
#include "Panels/HierarchyPanel.h"
#include "Panels/InspectorPanel/InspectorPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/AssetPickerPanel.h"
#include "Panels/ConsolePanel.h"
#include "Panels/SettingsPanel.h"
#include "Panels/TilePalettePanel.h"
#include "Panels/ParticlePickerPanel.h"
#include "Panels/ScriptEditorPanel.h"
#include "Panels/TweenLabPanel.h"
#include "Panels/EffectsPanel.h"
#include "Panels/DocsPanel.h"
#include "Panels/ProjectSettingsPanel.h"
#include "Panels/BuildPanel.h"
#include "Panels/InspectorPanel/AnimationMakerPanel.h"
#include "EditorContext.h"
#include "EditorHistory.h"
#include "EditorRecovery.h"
#include "Android/AndroidTools.h"

#include <optional>

#include <functional>
#include <string>

class EditorLayer : public SIMPEngine::Layer
{
public:
    EditorLayer(SIMPEngine::RenderingLayer *renderingLayer);
    ~EditorLayer() override = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(class SIMPEngine::TimeStep ts) override;
    void OnEvent(SIMPEngine::Event &e) override;
    void OnRender() override;

    void SetCloseProjectCallback(std::function<void()> callback) { m_OnCloseProject = std::move(callback); }
    void OpenStartScene(const std::string &path);

private:
    enum class PendingAction
    {
        None,
        NewScene,
        OpenScene,
        CloseProject,
        Quit
    };

    void DrawMenuBar();
    void DrawTransport();
    void DrawStatusBar();
    void DrawDockspace();
    void DrawSaveAsModal();
    void DrawConfirmModal();
    void DrawShortcutsWindow();
    void DrawRecoveryModal();
    void CheckRecovery(const std::string &scenePath);
    void TrackWindow(float dt);
    void RestoreWindow();
    void WriteRecovery();
    void HandleRuntimeFaults();
    void SetGameFullscreen(bool fullscreen);
    void Flash(const std::string &message, bool error = false);
    void OpenNewParticleEffect();
    void BuildDefaultLayout(unsigned int dockspaceId);
    void HandleShortcuts();
    void UpdateWindowTitle();

    void RequestAction(PendingAction action, const std::string &path = {});
    void RunAction(PendingAction action, const std::string &path);
    void NewScene();
    void OpenScene(const std::string &path);
    void BrowseOpenScene();
    bool SaveScene();
    void SaveSceneAs(const std::string &path);

    void Play();
    void Stop();
    void TogglePause();
    bool SaveBeforeLaunch();
    void RunStandalone();
    void RunOnDevices();
    void DrawAndroidControls(float h);
    void UpdateAndroid();

    EditorContext m_EditorContext;
    EditorHistory m_History;

    ViewportPanel m_ViewportPanel;
    GameViewPanel m_GameViewPanel;
    HierarchyPanel m_HieararchyPanel;
    InspectorPanel m_InspectorPanel;
    TilePalettePanel m_TilePalettePanel;
    ConsolePanel m_ConsolePanel;
    SettingsPanel m_SettingsPanel;
    ScriptEditorPanel m_ScriptEditorPanel;
    TweenLabPanel m_TweenLabPanel;
    EffectsPanel m_EffectsPanel;
    DocsPanel m_DocsPanel;
    ProjectSettingsPanel m_ProjectSettingsPanel;
    BuildPanel m_BuildPanel;
    AndroidSession m_Android;
    AndroidSession::State m_AndroidLastState = AndroidSession::State::Idle;
    std::function<void()> m_OnCloseProject;
    SIMPEngine::RenderingLayer *m_RenderingLayer;

    std::unique_ptr<ContentBrowserPanel> m_ContentBrowser;
    std::unique_ptr<SIMPEngine::AssetManager> m_AssetManager;
    std::unique_ptr<AssetPickerPanel> m_AssetPicker;
    std::unique_ptr<ParticlePickerPanel> m_ParticlePicker;
    std::unique_ptr<AnimationMakerPanel> m_AnimationMakerPanel;

    SIMPEngine::SceneSerializer serializer;

    SceneSnapshot m_PlaySnapshot;
    bool m_DirtyBeforePlay = false;

    PendingAction m_PendingAction = PendingAction::None;
    std::string m_PendingPath;
    bool m_ShowConfirm = false;
    std::string m_TabBeforePlay;
    bool m_ShowSaveAs = false;
    bool m_ShowShortcuts = false;
    std::string m_SaveAsName;
    PendingAction m_AfterSave = PendingAction::None;

    bool m_ShowProfiler = false;
    bool m_ResetLayout = false;
    float m_WindowSettle = 0.0f;

public:
    void RequestLayoutReset() { m_ResetLayout = true; }

private:
    bool m_LayoutChecked = false;
    int m_FocusDefaultTabs = 0;
    float m_StatusFlash = 0.0f;
    std::string m_StatusMessage;
    bool m_StatusError = false;

    float m_AutosaveTimer = 0.0f;
    std::optional<RecoveryFile> m_Recovery;
    bool m_ShowRecovery = false;
    bool m_RecoveryUndecided = false;
    std::string m_CrashReport;
    bool m_WindowWasFullscreen = false;
    std::string m_WindowTitle;
};
