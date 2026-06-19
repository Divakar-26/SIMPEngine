#include "EditorLayer.h"
#include "imgui.h"
#include <Engine/Core/Log.h>
#include "ImGuiSink.h"
#include <Engine/Core/FileSystem.h>

EditorLayer::EditorLayer(SIMPEngine::RenderingLayer *renderingLayer)
    : Layer("EditorLayer"), m_ViewportPanel(renderingLayer, &m_EditorContext), m_HieararchyPanel(&m_EditorContext), serializer(&renderingLayer->GetScene()), m_InspectorPanel(&m_EditorContext)
{
    this->m_RenderingLayer = renderingLayer;

    m_EditorContext.Scene = &renderingLayer->GetScene();
    m_EditorContext.Viewport = &m_ViewportPanel;

    m_AssetManager = std::make_unique<SIMPEngine::AssetManager>();
    m_AssetManager->Init("assets", "assets/registry.asset");

    m_ContentBrowser = std::make_unique<ContentBrowserPanel>("assets://", m_AssetManager.get());

    m_AssetPicker = std::make_unique<AssetPickerPanel>("assets://", m_AssetManager.get());
    m_InspectorPanel.SetAssetPicker(m_AssetPicker.get());

    m_AnimationMakerPanel =
        std::make_unique<AnimationMakerPanel>();
    m_InspectorPanel.SetAnimationMaker(
        m_AnimationMakerPanel.get());

    SIMPEngine::FileSystem::CreateDirectories("assets");
}

void EditorLayer::OnAttach()
{
    ImGuiStyle &style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Tab] = ImVec4(0.2f, 0.2f, 0.25f, 1.0f);

    m_ViewportPanel.OnAttach();

    m_RenderingLayer->OnAttach();
}

void EditorLayer::OnDetach()
{
    m_RenderingLayer->OnDetach();
}

void EditorLayer::OnUpdate(class SIMPEngine::TimeStep ts)
{
    if (m_ViewportPanel.iSFocusedAndHovered())
        m_RenderingLayer->OnUpdate(ts.GetSeconds());
}
void EditorLayer::OnEvent(SIMPEngine::Event &e)
{
    m_RenderingLayer->OnEvent(e);
}

void EditorLayer::OnRender()
{
    m_RenderingLayer->OnRender();

    static bool dockspaceOpen = true;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpaceWindow", &dockspaceOpen, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save Scene"))
                serializer.Serialize("assets/scene.simpscene");
            if (ImGui::MenuItem("Load Scene"))
                serializer.Deserialize("assets/scene.simpscene");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Log", nullptr, &showLogs);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug"))
        {
            if (ImGui::MenuItem("Open Asset Picker"))
            {
                m_AssetPicker->Open(
                    "Select Texture",
                    "assets://",
                    [](const std::string &path)
                    {
                        CORE_INFO("Picked {}", path);
                    });
            }

            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    m_ViewportPanel.OnImGuiRender();
    m_HieararchyPanel.OnImGuiRender();
    m_InspectorPanel.OnImGuiRender();
    m_ContentBrowser->OnImGuiRender();
    m_AssetPicker->OnImGuiRender();
    m_AnimationMakerPanel->OnImGuiRender();

    if (showLogs)
        ShowLogs();

    ImGui::End();
}

void EditorLayer::ShowLogs()
{
    if (showLogs)
    {
        ImGui::Begin("Log", &showLogs);

        auto sink = SIMPEngine::Log::GetImGuiSink();
        if (sink)
        {
            for (auto &line : sink->Logs)
            {
                ImVec4 color;
                if (line.find("[error]") != std::string::npos)
                    color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                else if (line.find("[info]") != std::string::npos)
                    color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                else
                    color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                ImGui::TextColored(color, "%s", line.c_str());
            }
            if (ImGui::GetScrollMaxY() > 0)
                ImGui::SetScrollHereY(1.0f);
        }
        ImGui::End();
    }
}