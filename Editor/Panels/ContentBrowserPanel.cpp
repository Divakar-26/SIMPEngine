// ContentBrowserPanel.cpp
#include "ContentBrowserPanel.h"
#include "Core/VFS.h"
#include "Core/FileSystem.h"


std::string ContentBrowserPanel::ToVirtualChild(const std::string& parent, const std::string& filename){
    if(parent.ends_with('/')) return parent + filename;
    return parent + "/" + filename;
}

void ContentBrowserPanel::OnImGuiRender(){
    ImGui::Begin("Content Browser");

    // left tree
    ImGui::BeginChild("##tree", ImVec2(220, 0), true);
    DrawDirectory(m_Root);
    ImGui::EndChild();

    ImGui::SameLine();

    // right grid
    ImGui::BeginChild("##grid", ImVec2(0,0), true);
    auto real = SIMPEngine::VFS::Resolve(m_CurrentDir);
    if(real){
        auto entries = SIMPEngine::FileSystem::List(*real);
        float cell = 96.0f; int columns = (int)(ImGui::GetContentRegionAvail().x / cell); if(columns<1) columns=1;
        ImGui::Columns(columns, 0, false);
        for (auto& e : entries){
            std::string filename = std::filesystem::u8path(e.path).filename().string();
            DrawEntry(ToVirtualChild(m_CurrentDir, filename), e.isDirectory);
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
    }
    ImGui::EndChild();

    ImGui::End();
}

void ContentBrowserPanel::DrawDirectory(const std::string& vdir){
    auto real = SIMPEngine::VFS::Resolve(vdir); if(!real) return;
    auto label = std::filesystem::u8path(*real).filename().string();
    if(label.empty()) label = vdir; 

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;
    bool opened = ImGui::TreeNodeEx(label.c_str(), flags);
    if (ImGui::IsItemClicked()){ m_CurrentDir = vdir; }

    if(opened){
        for (auto& e : SIMPEngine::FileSystem::List(*real)){
            if(e.isDirectory){
                std::string childV = ToVirtualChild(vdir, std::filesystem::u8path(e.path).filename().string());
                DrawDirectory(childV);
            }
        }
        ImGui::TreePop();
    }
}

void ContentBrowserPanel::DrawEntry(const std::string& vpath, bool isDir){
    auto real = SIMPEngine::VFS::Resolve(vpath); if(!real) return;
    std::string name = std::filesystem::u8path(*real).filename().string();

    ImGui::PushID(vpath.c_str());
    ImGui::BeginGroup();
    ImGui::Button(isDir ? "📁" : "📄", ImVec2(72,72));

    if(ImGui::BeginDragDropSource()){
        // if file, offer asset handle payload
        SIMPEngine::AssetType type = SIMPEngine::AssetType::Unknown;
        if(!isDir){
            // naive type guess
            auto ext = std::filesystem::u8path(*real).extension().string();
            if(ext==".png"||ext==".jpg") type = SIMPEngine::AssetType::Texture2D;
            if(ext==".scene") type = SIMPEngine::AssetType::Scene;
            auto h = m_AM->ImportIfNeeded(vpath, type);
            ImGui::SetDragDropPayload("ASSET_HANDLE", &h, sizeof(h));
        }
        ImGui::TextUnformatted(name.c_str());
        ImGui::EndDragDropSource();
    }

    if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)){
        if(isDir) m_CurrentDir = vpath;
        // could open viewer for files
    }

    ImGui::TextWrapped("%s", name.c_str());
    ImGui::EndGroup();

    // context menu
    if(ImGui::BeginPopupContextItem("ctx")){
        if(isDir && ImGui::MenuItem("New Folder")){
            auto newV = ToVirtualChild(vpath, "New Folder");
            auto newR = SIMPEngine::VFS::Resolve(newV);
            if(newR) SIMPEngine::FileSystem::CreateDirectories(*newR);
        }
        if(ImGui::MenuItem("Rename")){
            // left to implement: inline rename state
        }
        if(ImGui::MenuItem("Delete")){
            SIMPEngine::FileSystem::Remove(*real);
        }
        ImGui::EndPopup();
    }

    ImGui::PopID();
}

