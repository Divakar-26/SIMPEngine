#include "EditorLayer.h"
#include "imgui.h"
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/FileSystem.h>
#include <Engine/Core/VFS.h>
#include <Engine/Input/Input.h>
#include "UI/Theme.h"
#include "UI/Widgets.h"

#include <Engine/Core/Profiler/EngineProfiler.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>

namespace
{
    constexpr const char *SceneFolder = "assets://scenes";
    const std::vector<std::string> SceneExtensions = {".yaml", ".yml", ".simpscene"};

    std::string DisplayName(const std::string &path)
    {
        if (path.empty())
            return "Untitled";
        return std::filesystem::path(path).filename().string();
    }
}

EditorLayer::EditorLayer(SIMPEngine::RenderingLayer *renderingLayer)
    : Layer("EditorLayer"), m_History(&m_EditorContext), m_ViewportPanel(renderingLayer, &m_EditorContext), m_GameViewPanel(&m_EditorContext), m_HieararchyPanel(&m_EditorContext), m_InspectorPanel(&m_EditorContext), serializer(&renderingLayer->GetScene())
{
    this->m_RenderingLayer = renderingLayer;

    m_EditorContext.Scene = &renderingLayer->GetScene();
    m_EditorContext.Viewport = &m_ViewportPanel;
    m_EditorContext.History = &m_History;
    m_EditorContext.State = SceneState::Edit;

    m_AssetManager = std::make_unique<SIMPEngine::AssetManager>();
    m_AssetManager->Init("assets", "assets/registry.asset");

    m_ContentBrowser = std::make_unique<ContentBrowserPanel>("assets://", m_AssetManager.get());

    m_AssetPicker = std::make_unique<AssetPickerPanel>("assets://", m_AssetManager.get());
    m_InspectorPanel.SetAssetPicker(m_AssetPicker.get());

    m_AnimationMakerPanel =
        std::make_unique<AnimationMakerPanel>();
    m_AnimationMakerPanel->SetContext(&m_EditorContext);
    m_InspectorPanel.SetAnimationMaker(
        m_AnimationMakerPanel.get());

    SIMPEngine::FileSystem::CreateDirectories("assets");
}

void EditorLayer::OnAttach()
{
    EditorUI::RegisterSettingsHandler();
    EditorUI::LoadFonts();
    EditorUI::ApplyTheme(EditorUI::Settings().Theme);
    EditorUI::SetEditHook([this]
                          { m_History.BeginEdit(); });

    m_ViewportPanel.OnAttach();

    m_RenderingLayer->OnAttach();
    m_RenderingLayer->SetRenderToBackbuffer(false);
    m_RenderingLayer->SetSimulationEnabled(false);
    SIMPEngine::Input::SetEnabled(false);

    m_EditorContext.Scene->UpdateTransforms();
    m_History.Clear();
}

void EditorLayer::OnDetach()
{
    EditorUI::SetEditHook(nullptr);
    m_RenderingLayer->OnDetach();
}

void EditorLayer::OnUpdate(class SIMPEngine::TimeStep ts)
{
    if (m_SettingsPanel.ConsumeApplyRequest())
        EditorUI::ApplySettings();

    m_RenderingLayer->SetSimulationEnabled(m_EditorContext.State == SceneState::Play);

    if (m_EditorContext.State == SceneState::Pause && m_EditorContext.StepRequested)
        m_RenderingLayer->Step(1.0f / 60.0f);
    m_EditorContext.StepRequested = false;

    m_StatusFlash = std::max(0.0f, m_StatusFlash - ts.GetSeconds());
    UpdateWindowTitle();
}

void EditorLayer::OnEvent(SIMPEngine::Event &e)
{
    m_RenderingLayer->OnEvent(e);
}

void EditorLayer::UpdateWindowTitle()
{
    std::string title = DisplayName(m_EditorContext.ScenePath) + (m_EditorContext.Dirty ? " *" : "") + " - SIMPEngine";
    if (title == m_WindowTitle)
        return;
    m_WindowTitle = title;
    SDL_SetWindowTitle(SIMPEngine::Application::Get().GetWindow().GetNativeWindow(), title.c_str());
}

void EditorLayer::Play()
{
    if (m_EditorContext.State != SceneState::Edit)
        return;

    m_PlaySnapshot = m_History.Capture();
    m_DirtyBeforePlay = m_EditorContext.Dirty;

    SIMPEngine::Scene &scene = *m_EditorContext.Scene;
    scene.SyncPhysicsFromTransforms();
    scene.MarkPhysicsDirty();

    m_EditorContext.State = SceneState::Play;
    m_EditorContext.FocusGameView = true;
}

void EditorLayer::Stop()
{
    if (m_EditorContext.State == SceneState::Edit)
        return;

    m_EditorContext.State = SceneState::Edit;
    m_RenderingLayer->SetSimulationEnabled(false);
    SIMPEngine::Input::SetEnabled(false);

    m_History.Restore(m_PlaySnapshot);
    m_History.RefreshStable();
    m_EditorContext.Dirty = m_DirtyBeforePlay;
    m_EditorContext.FocusSceneView = true;
}

void EditorLayer::TogglePause()
{
    if (m_EditorContext.State == SceneState::Play)
        m_EditorContext.State = SceneState::Pause;
    else if (m_EditorContext.State == SceneState::Pause)
        m_EditorContext.State = SceneState::Play;
}

void EditorLayer::RequestAction(PendingAction action, const std::string &path)
{
    if (m_EditorContext.State != SceneState::Edit)
        Stop();

    if (m_EditorContext.Dirty)
    {
        m_PendingAction = action;
        m_PendingPath = path;
        m_ShowConfirm = true;
        return;
    }
    RunAction(action, path);
}

void EditorLayer::RunAction(PendingAction action, const std::string &path)
{
    if (action == PendingAction::NewScene)
        NewScene();
    else if (action == PendingAction::OpenScene)
    {
        if (path.empty())
            BrowseOpenScene();
        else
            OpenScene(path);
    }
}

void EditorLayer::NewScene()
{
    SIMPEngine::Scene &scene = *m_EditorContext.Scene;
    m_EditorContext.SelectedEntity = {};
    m_EditorContext.HoveredEntity = {};
    scene.Clear();
    scene.SetName("Untitled");
    scene.SetBackgroundColor({0.329f, 0.329f, 0.329f, 1.0f});
    m_EditorContext.ScenePath.clear();
    m_EditorContext.Dirty = false;
    m_History.Clear();
}

void EditorLayer::OpenScene(const std::string &path)
{
    SIMPEngine::Scene &scene = *m_EditorContext.Scene;
    m_EditorContext.SelectedEntity = {};
    m_EditorContext.HoveredEntity = {};
    scene.Clear();
    if (!serializer.Deserialize(path))
    {
        CORE_ERROR("Could not open scene {}", path);
        m_StatusMessage = "Could not open " + DisplayName(path);
    }
    else
    {
        m_EditorContext.ScenePath = path;
        m_StatusMessage = "Opened " + DisplayName(path);
    }
    m_StatusFlash = 3.0f;
    m_EditorContext.Dirty = false;
    m_History.Clear();
    m_ViewportPanel.FrameScene();
}

void EditorLayer::BrowseOpenScene()
{
    m_AssetPicker->Open(
        "Open Scene",
        SceneFolder,
        [this](const std::string &vpath)
        { OpenScene(vpath); },
        SceneExtensions);
}

bool EditorLayer::SaveScene()
{
    if (m_EditorContext.State != SceneState::Edit)
    {
        m_StatusMessage = "Stop the game before saving";
        m_StatusFlash = 3.0f;
        return false;
    }

    if (m_EditorContext.ScenePath.empty())
    {
        std::string name = m_EditorContext.Scene->GetName();
        m_SaveAsName = (name.empty() || name == "untitled") ? "scene.yaml" : name + ".yaml";
        m_ShowSaveAs = true;
        return false;
    }

    SaveSceneAs(m_EditorContext.ScenePath);
    return true;
}

void EditorLayer::SaveSceneAs(const std::string &path)
{
    if (serializer.Serialize(path))
    {
        m_EditorContext.ScenePath = path;
        m_EditorContext.Dirty = false;
        m_StatusMessage = "Saved " + DisplayName(path);
    }
    else
    {
        m_StatusMessage = "Save failed";
    }
    m_StatusFlash = 3.0f;
}

void EditorLayer::HandleShortcuts()
{
    ImGuiIO &io = ImGui::GetIO();
    bool typing = io.WantTextInput;
    EditorContext &ctx = m_EditorContext;

    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, ImGuiInputFlags_RouteGlobal))
        SaveScene();
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S, ImGuiInputFlags_RouteGlobal))
    {
        m_SaveAsName = DisplayName(ctx.ScenePath.empty() ? "scene.yaml" : ctx.ScenePath);
        m_ShowSaveAs = true;
    }
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O, ImGuiInputFlags_RouteGlobal))
        RequestAction(PendingAction::OpenScene);
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N, ImGuiInputFlags_RouteGlobal))
        RequestAction(PendingAction::NewScene);
    if (ImGui::Shortcut(ImGuiKey_F5, ImGuiInputFlags_RouteGlobal))
    {
        if (ctx.State == SceneState::Edit)
            Play();
        else
            Stop();
    }
    if (ImGui::Shortcut(ImGuiKey_F6, ImGuiInputFlags_RouteGlobal))
        TogglePause();
    if (ImGui::Shortcut(ImGuiKey_F7, ImGuiInputFlags_RouteGlobal) && ctx.State == SceneState::Pause)
        ctx.StepRequested = true;

    if (typing || ImGui::IsAnyItemActive())
        return;

    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z, ImGuiInputFlags_RouteGlobal))
        m_History.Undo();
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, ImGuiInputFlags_RouteGlobal) ||
        ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Y, ImGuiInputFlags_RouteGlobal))
        m_History.Redo();

    bool hasSelection = ctx.SelectedEntity && ctx.Scene->GetRegistry().valid(ctx.SelectedEntity.GetHandle());
    if (hasSelection && !m_GameViewPanel.IsFocused())
    {
        if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_D, ImGuiInputFlags_RouteGlobal))
            EditorCommands::DuplicateSelection(ctx);
        if (ImGui::Shortcut(ImGuiKey_Delete, ImGuiInputFlags_RouteGlobal))
            EditorCommands::DeleteSelection(ctx);
        if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
            ctx.ClearSelection();
    }
}

void EditorLayer::OnRender()
{
    HandleShortcuts();
    DrawMenuBar();
    DrawStatusBar();
    DrawDockspace();

    m_ViewportPanel.OnImGuiRender();
    m_GameViewPanel.OnImGuiRender();
    m_HieararchyPanel.OnImGuiRender();
    m_InspectorPanel.OnImGuiRender();
    m_ContentBrowser->OnImGuiRender();
    m_ConsolePanel.OnImGuiRender();
    m_AssetPicker->OnImGuiRender();
    m_AnimationMakerPanel->OnImGuiRender();
    m_SettingsPanel.OnImGuiRender();

    if (m_ShowProfiler)
        EngineProfiler::Profiler::Get().DrawImGui("Profiler###Profiler", &m_ShowProfiler);

    DrawSaveAsModal();
    DrawConfirmModal();
    DrawShortcutsWindow();

    if (m_FocusDefaultTabs > 0 && --m_FocusDefaultTabs == 0)
    {
        ImGui::SetWindowFocus("FileSystem###ContentBrowser");
        ImGui::SetWindowFocus("Viewport###Viewport");
    }

    m_History.EndFrame(ImGui::IsAnyItemActive() || m_ViewportPanel.IsInteracting());
}

void EditorLayer::DrawTransport()
{
    using namespace EditorUI;

    float h = ImGui::GetFrameHeight();
    float groupW = h * 3.0f + 8.0f;
    float x = std::floor((ImGui::GetWindowWidth() - groupW) * 0.5f);
    ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), x));

    bool editing = m_EditorContext.State == SceneState::Edit;
    bool paused = m_EditorContext.State == SceneState::Pause;

    if (IconButton("##play", editing ? ICON_LC_PLAY : ICON_LC_SQUARE, editing ? "Play (F5)" : "Stop (F5)", !editing, h))
    {
        if (editing)
            Play();
        else
            Stop();
    }
    ImGui::SameLine(0, 4);
    ImGui::BeginDisabled(editing);
    if (IconButton("##pause", ICON_LC_PAUSE, "Pause (F6)", paused, h))
        TogglePause();
    ImGui::SameLine(0, 4);
    ImGui::BeginDisabled(!paused);
    if (IconButton("##step", ICON_LC_STEP_FORWARD, "Step frame (F7)", false, h))
        m_EditorContext.StepRequested = true;
    ImGui::EndDisabled();
    ImGui::EndDisabled();
}

void EditorLayer::DrawMenuBar()
{
    using namespace EditorUI;
    EditorContext &ctx = m_EditorContext;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, Colors::Canvas);

    if (ImGui::BeginMainMenuBar())
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 5));

        if (ImGui::BeginMenu("Scene"))
        {
            if (ImGui::MenuItem("New Scene", "Ctrl+N"))
                RequestAction(PendingAction::NewScene);
            if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
                RequestAction(PendingAction::OpenScene);
            ImGui::Separator();
            if (ImGui::MenuItem("Save", "Ctrl+S"))
                SaveScene();
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
            {
                m_SaveAsName = DisplayName(ctx.ScenePath.empty() ? "scene.yaml" : ctx.ScenePath);
                m_ShowSaveAs = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            bool hasSelection = ctx.SelectedEntity && ctx.Scene->GetRegistry().valid(ctx.SelectedEntity.GetHandle());
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, m_History.CanUndo() && ctx.IsEditing()))
                m_History.Undo();
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, m_History.CanRedo() && ctx.IsEditing()))
                m_History.Redo();
            ImGui::Separator();
            if (ImGui::MenuItem("New Entity"))
            {
                ctx.SelectedEntity = EditorCommands::CreateEntity(ctx, "Entity");
                ctx.RenameRequest = ctx.SelectedEntity.GetHandle();
            }
            if (ImGui::MenuItem("Rename", "F2", false, hasSelection))
                ctx.RenameRequest = ctx.SelectedEntity.GetHandle();
            if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, hasSelection))
                EditorCommands::DuplicateSelection(ctx);
            if (ImGui::MenuItem("Delete", "Del", false, hasSelection))
                EditorCommands::DeleteSelection(ctx);
            ImGui::Separator();
            if (ImGui::MenuItem("Editor Settings..."))
                m_SettingsPanel.Open();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Console", nullptr, m_ConsolePanel.IsVisible()))
                *m_ConsolePanel.VisibleFlag() = !m_ConsolePanel.IsVisible();
            ImGui::MenuItem("Profiler", nullptr, &m_ShowProfiler);
            if (ImGui::MenuItem("Render Stats", nullptr, Settings().ShowStats))
            {
                Settings().ShowStats = !Settings().ShowStats;
                ImGui::MarkIniSettingsDirty();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Frame Scene"))
                m_ViewportPanel.FrameScene();
            if (ImGui::MenuItem("Reset Layout"))
                m_ResetLayout = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Run"))
        {
            bool editing = ctx.State == SceneState::Edit;
            if (ImGui::MenuItem(editing ? "Play" : "Stop", "F5"))
            {
                if (editing)
                    Play();
                else
                    Stop();
            }
            if (ImGui::MenuItem(ctx.State == SceneState::Pause ? "Resume" : "Pause", "F6", false, !editing))
                TogglePause();
            if (ImGui::MenuItem("Step Frame", "F7", false, ctx.State == SceneState::Pause))
                ctx.StepRequested = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("Keyboard Shortcuts"))
                m_ShowShortcuts = true;
            ImGui::EndMenu();
        }

        ImGui::PopStyleVar(2);

        DrawTransport();

        std::string title = DisplayName(ctx.ScenePath);
        if (ctx.Dirty)
            title += "  (unsaved)";
        float w = ImGui::CalcTextSize(title.c_str()).x;
        ImGui::SameLine(ImGui::GetWindowWidth() - w - 14.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDim);
        ImGui::TextUnformatted(title.c_str());
        ImGui::PopStyleColor();

        ImGui::EndMainMenuBar();
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void EditorLayer::DrawStatusBar()
{
    using namespace EditorUI;
    EditorContext &ctx = m_EditorContext;

    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, Colors::Canvas);
    bool open = ImGui::BeginViewportSideBar("##EditorStatusBar", viewport, ImGuiDir_Down, Metrics::StatusBarHeight, flags);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    if (open)
    {
        ImDrawList *dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        float fs = ImGui::GetFontSize();
        float y = std::floor(wp.y + (Metrics::StatusBarHeight - fs) * 0.5f);
        float x = wp.x + 10.0f;

        auto segment = [&](const char *text, const ImVec4 &color)
        {
            dl->AddText(ImVec2(x, y), U32(color), text);
            x += ImGui::CalcTextSize(text).x + 18.0f;
        };

        const char *state = ctx.State == SceneState::Play ? "Playing" : ctx.State == SceneState::Pause ? "Paused" : "Editing";
        const ImVec4 &stateColor = ctx.State == SceneState::Play ? Colors::Success : ctx.State == SceneState::Pause ? Colors::Warning : Colors::TextDim;
        segment(state, stateColor);

        char buf[160];
        std::snprintf(buf, sizeof(buf), "%zu entities", ctx.Scene->GetRegistry().view<TagComponent>().size());
        segment(buf, Colors::TextDim);

        if (ctx.SelectedEntity && ctx.Scene->GetRegistry().valid(ctx.SelectedEntity.GetHandle()) && ctx.SelectedEntity.HasComponent<TagComponent>())
            segment(ctx.SelectedEntity.GetComponent<TagComponent>().Tag.c_str(), Colors::Text);

        if (m_StatusFlash > 0.0f && !m_StatusMessage.empty())
            segment(m_StatusMessage.c_str(), WithAlpha(Colors::Text, std::min(1.0f, m_StatusFlash)));

        auto &profiler = EngineProfiler::Profiler::Get();
        std::snprintf(buf, sizeof(buf), "%.2f ms   %.0f fps", profiler.GetCurrentFrameTimeMs(), profiler.GetCurrentFPS());
        float w = ImGui::CalcTextSize(buf).x;
        dl->AddText(ImVec2(wp.x + ImGui::GetWindowWidth() - w - 10.0f, y), U32(Colors::TextFaint), buf);
    }
    ImGui::End();
}

void EditorLayer::DrawSaveAsModal()
{
    using namespace EditorUI;

    if (!BeginModal("Save Scene As", &m_ShowSaveAs, ImVec2(420, 0)))
        return;

    PushEditSuppression();
    if (ImGui::IsWindowAppearing())
        ImGui::SetKeyboardFocusHere();

    PropertyLabel("Folder");
    ImGui::TextUnformatted(SceneFolder);
    PropertyLabel("File name");
    bool submit = ImGui::InputText("##name", &m_SaveAsName, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

    std::string name = m_SaveAsName;
    if (!name.empty() && std::filesystem::path(name).extension().empty())
        name += ".yaml";
    std::string path = std::string(SceneFolder) + "/" + name;

    bool exists = false;
    if (auto real = SIMPEngine::VFS::Resolve(path))
        exists = SIMPEngine::FileSystem::Exists(*real);
    if (exists)
    {
        PropertyLabel("");
        ImGui::TextColored(Colors::Warning, "This file exists and will be replaced.");
    }

    ImGui::Dummy(ImVec2(0, 4));
    float bw = 90.0f;
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - bw * 2.0f - spacing);
    if (Button("Cancel", ButtonKind::Secondary, ImVec2(bw, 0)))
    {
        m_ShowSaveAs = false;
        m_AfterSave = PendingAction::None;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(name.empty());
    if (Button("Save", ButtonKind::Primary, ImVec2(bw, 0)) || (submit && !name.empty()))
    {
        SaveSceneAs(path);
        m_ShowSaveAs = false;
        ImGui::CloseCurrentPopup();
        if (m_AfterSave != PendingAction::None && !m_EditorContext.Dirty)
            RunAction(m_AfterSave, m_PendingPath);
        m_AfterSave = PendingAction::None;
    }
    ImGui::EndDisabled();

    PopEditSuppression();
    EndModal();
}

void EditorLayer::DrawConfirmModal()
{
    using namespace EditorUI;

    if (!BeginModal("Unsaved Changes", &m_ShowConfirm, ImVec2(420, 0)))
        return;

    ImGui::TextWrapped("Save changes to %s before continuing?", DisplayName(m_EditorContext.ScenePath).c_str());
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDim);
    ImGui::TextWrapped("Your changes will be lost if you don't save them.");
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 6));

    float bw = 96.0f;
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - bw * 3.0f - spacing * 2.0f);

    PendingAction action = m_PendingAction;
    if (Button("Don't Save", ButtonKind::Ghost, ImVec2(bw, 0)))
    {
        m_ShowConfirm = false;
        ImGui::CloseCurrentPopup();
        RunAction(action, m_PendingPath);
    }
    ImGui::SameLine();
    if (Button("Cancel", ButtonKind::Secondary, ImVec2(bw, 0)))
    {
        m_ShowConfirm = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (Button("Save", ButtonKind::Primary, ImVec2(bw, 0)))
    {
        m_ShowConfirm = false;
        ImGui::CloseCurrentPopup();
        if (SaveScene())
            RunAction(action, m_PendingPath);
        else
            m_AfterSave = action;
    }

    EndModal();
}

void EditorLayer::DrawShortcutsWindow()
{
    using namespace EditorUI;

    if (!m_ShowShortcuts)
        return;

    ImGui::SetNextWindowSize(ImVec2(380, 0), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::Begin("Keyboard Shortcuts", &m_ShowShortcuts, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
    {
        static const char *rows[][2] = {
            {"Play / Stop", "F5"},
            {"Pause / Resume", "F6"},
            {"Step frame", "F7"},
            {"Select / Move / Rotate / Scale", "Q  W  E  R"},
            {"Snap while dragging", "Hold Ctrl"},
            {"Frame selection", "F"},
            {"Pan view", "Middle or right drag, Space + drag"},
            {"Zoom view", "Mouse wheel"},
            {"Undo / Redo", "Ctrl+Z  /  Ctrl+Y"},
            {"Duplicate", "Ctrl+D"},
            {"Rename", "F2"},
            {"Delete", "Del"},
            {"New / Open / Save", "Ctrl+N  /  Ctrl+O  /  Ctrl+S"},
        };
        if (ImGui::BeginTable("##keys", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
        {
            for (auto &r : rows)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(r[0]);
                ImGui::TableNextColumn();
                ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDim);
                ImGui::TextUnformatted(r[1]);
                ImGui::PopStyleColor();
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void EditorLayer::DrawDockspace()
{
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGuiID dockspaceId = ImHashStr("SIMPEditorDockspace.v2");

    if (!m_LayoutChecked)
    {
        m_LayoutChecked = true;
        ImGuiDockNode *node = ImGui::DockBuilderGetNode(dockspaceId);
        if (!node || node->IsLeafNode())
            m_ResetLayout = true;
    }

    if (m_ResetLayout)
    {
        BuildDefaultLayout(dockspaceId);
        m_ResetLayout = false;
        m_FocusDefaultTabs = 2;
    }

    ImGui::DockSpaceOverViewport(dockspaceId, viewport, ImGuiDockNodeFlags_None);
}

void EditorLayer::BuildDefaultLayout(unsigned int dockspaceId)
{
    ImGuiViewport *viewport = ImGui::GetMainViewport();

    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

    ImGuiID center = dockspaceId;
    ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.17f, nullptr, &center);
    ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.24f, nullptr, &center);
    ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.28f, nullptr, &center);

    ImGui::DockBuilderDockWindow(HierarchyPanel::WindowId, left);
    ImGui::DockBuilderDockWindow(InspectorPanel::WindowId, right);
    ImGui::DockBuilderDockWindow(ContentBrowserPanel::WindowId, bottom);
    ImGui::DockBuilderDockWindow(ConsolePanel::WindowId, bottom);
    ImGui::DockBuilderDockWindow("###Profiler", bottom);
    ImGui::DockBuilderDockWindow(ViewportPanel::WindowId, center);
    ImGui::DockBuilderDockWindow(GameViewPanel::WindowId, center);

    ImGui::DockBuilderFinish(dockspaceId);

    if (ImGuiDockNode *node = ImGui::DockBuilderGetNode(bottom))
        node->SelectedTabId = ImHashStr("#TAB", 0, ImHashStr(ContentBrowserPanel::WindowId));
    if (ImGuiDockNode *node = ImGui::DockBuilderGetNode(center))
        node->SelectedTabId = ImHashStr("#TAB", 0, ImHashStr(ViewportPanel::WindowId));
}
