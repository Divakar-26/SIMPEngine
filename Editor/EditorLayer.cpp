#include "EditorLayer.h"
#include "imgui.h"
#include "Core/Log.h"
#include "ImGuiSink.h"
#include "Core/FileSystem.h"

EditorLayer::EditorLayer(SIMPEngine::RenderingLayer *renderingLayer)
    : Layer("EditorLayer"), m_ViewportPanel(renderingLayer), m_HieararchyPanel(renderingLayer), serializer(&renderingLayer->GetScene())
{
    this->m_RenderingLayer = renderingLayer;

    m_AssetManager = std::make_unique<SIMPEngine::AssetManager>();
    m_AssetManager->Init("assets", "assets/registry.asset");

    m_ContentBrowser = std::make_unique<ContentBrowserPanel>("assets://", m_AssetManager.get());

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
    // Create Dockspace

    m_RenderingLayer->OnRender();

    static bool dockspaceOpen = true;
    static bool opt_fullscreen = true;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // Dockspace

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    ImGui::End();

    auto it = m_HieararchyPanel.GetSelectedEntity();
    m_ViewportPanel.OnRender(*it);

    ShowLogs();

    m_HieararchyPanel.OnRender();
    m_InspectorPanel.SetSelectedEntity(*it);
    m_InspectorPanel.OnRender();
    m_ContentBrowser->OnImGuiRender();

    ImGui::ShowDemoWindow();
}

void EditorLayer::ShowLogs()
{
    if (ImGui::BeginMainMenuBar())

    {
        if (ImGui::BeginMenu("View"))

        {
            if (ImGui::MenuItem("Log", nullptr, &showLogs))
            {
            }

            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Save Scene"))
        {
            serializer.Serialize("assets/scene.simpscene");
        }

        if (ImGui::MenuItem("Load Scene"))
        {
            serializer.Deserialize("assets/scene.simpscene");
        }

        ImGui::EndMenu();
    }

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
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::End();
    }
}