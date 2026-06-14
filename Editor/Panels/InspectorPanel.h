#pragma once
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Component.h>
#include <../EditorContext.h>
#include <../EditorCommands.h>
#include <EditorPanel.h>
#include <filesystem>
#include "AssetPickerPanel.h"

#include <Engine/Core/Log.h>
#include <Engine/Core/FileSystem.h>
#include <Engine/Core/VFS.h>
#include <algorithm>
#include <cctype>
#include <filesystem>

class AssetPickerPanel;

template <typename T, typename UIFunc>
static void DrawComponent(const char *name, SIMPEngine::Entity entity, UIFunc uiFunc)
{
    if (entity.HasComponent<T>())
    {
        auto &component = entity.GetComponent<T>();

        if (ImGui::TreeNodeEx((void *)typeid(T).hash_code(),
                              ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap,
                              name))
        {
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Remove Component"))
                    entity.RemoveComponent<T>();
                ImGui::EndPopup();
            }

            uiFunc(component);
            ImGui::TreePop();
        }
    }
}

template<typename T, typename UIFunc>
static void DrawComponentCard(
    const char* name,
    SIMPEngine::Entity entity,
    UIFunc uiFunc)
{
    if (!entity.HasComponent<T>())
        return;

    auto& component = entity.GetComponent<T>();

    ImGui::PushStyleVar(
        ImGuiStyleVar_FramePadding,
        ImVec2(4, 2));

    ImGui::PushStyleColor(
        ImGuiCol_Header,
        ImVec4(0.22f, 0.27f, 0.36f, 1.0f));

    ImGui::PushStyleColor(
        ImGuiCol_HeaderHovered,
        ImVec4(0.28f, 0.34f, 0.46f, 1.0f));

    ImGui::PushStyleColor(
        ImGuiCol_HeaderActive,
        ImVec4(0.35f, 0.45f, 0.65f, 1.0f));

    bool open = ImGui::CollapsingHeader(
        name,
        ImGuiTreeNodeFlags_DefaultOpen);

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();

    if (!open)
        return;

    ImGui::PushID(name);

    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Remove Component"))
            entity.RemoveComponent<T>();

        ImGui::EndPopup();
    }

    uiFunc(component);

    ImGui::Dummy(ImVec2(0, 2));

    ImGui::PopID();
}

class InspectorPanel : public EditorPanel
{
public:
    InspectorPanel(EditorContext *m_Context);

    void OnImGuiRender() override;

    void DrawSpriteSection(SIMPEngine::Entity &m_SelectedEntity);

    void SetAssetPicker(AssetPickerPanel *picker)
    {
        m_AssetPicker = picker;
    }

private:
    EditorContext *m_Context;

    bool m_ShowTexturePicker = false;
    std::string m_SelectedTexturePath;

    AssetPickerPanel *m_AssetPicker = nullptr;
};
