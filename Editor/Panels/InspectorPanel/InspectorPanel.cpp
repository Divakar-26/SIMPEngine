#include "InspectorPanel.h"
#include "ComponentMenu.h"
#include <Engine/Scene/ScriptableEntity.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <cmath>
#include <cstdio>
#include <cstring>

InspectorPanel::InspectorPanel(
    EditorContext *context)
{
    m_Context = context;
}

void InspectorPanel::OnImGuiRender()
{
    using namespace EditorUI;

    auto &m_SelectedEntity =
        m_Context->SelectedEntity;

    bool visible = ImGui::Begin("Inspector###Inspector");
    if (!visible)
    {
        ImGui::End();
        return;
    }

    if (!m_SelectedEntity || !m_Context->Scene->GetRegistry().valid(m_SelectedEntity.GetHandle()))
    {
        DrawSceneProperties();
        ImGui::End();
        return;
    }

    DrawHeader(m_SelectedEntity);

    DrawCoreComponents(m_SelectedEntity);
    DrawSpriteSection(m_SelectedEntity);
    DrawAnimationSection(m_SelectedEntity);
    DrawPhysicsSection(m_SelectedEntity);

    ImGui::Dummy(ImVec2(0, 4));
    DrawAddComponentButton(m_SelectedEntity);

    ImGui::End();
}

void InspectorPanel::DrawSceneProperties()
{
    using namespace EditorUI;
    SIMPEngine::Scene &scene = *m_Context->Scene;

    if (BeginSection("scene", "Scene"))
    {
        std::string name = scene.GetName();
        if (PropertyText("Name", name))
            scene.SetName(name);

        glm::vec4 bg = scene.GetBackgroundColor();
        float rgba[4] = {bg.r, bg.g, bg.b, bg.a};
        if (PropertyColor("Background", rgba))
            scene.SetBackgroundColor({rgba[0], rgba[1], rgba[2], rgba[3]});

        char count[32];
        std::snprintf(count, sizeof(count), "%zu", scene.GetRegistry().view<TagComponent>().size());
        PropertyValue("Entities", count);
        PropertyValue("File", m_Context->ScenePath.empty() ? "Not saved yet" : m_Context->ScenePath.c_str(), &Colors::TextDim);
    }
    EndSection();

    ImGui::Dummy(ImVec2(0, 6));
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
    ImGui::TextWrapped("Select an entity in the Scene tree or the viewport to edit it.");
    ImGui::PopStyleColor();
}

void InspectorPanel::DrawHeader(SIMPEngine::Entity &entity)
{
    using namespace EditorUI;

    Visual v = EntityVisual(entity);

    if (entity.HasComponent<TagComponent>())
    {
        std::string tag = entity.GetComponent<TagComponent>().Tag;

        ImGui::PushStyleColor(ImGuiCol_Text, v.Color());
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(v.Icon);
        ImGui::PopStyleColor();
        ImGui::SameLine(0, 6);

        ImGui::PushFont(GetFont(FontRole::Medium), 0.0f);
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputText("##Tag", &tag, ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::IsItemDeactivatedAfterEdit())
            EditorCommands::RenameEntity(*m_Context, entity, tag);
        ImGui::PopFont();
    }

    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
    ImGui::Text("%s  ·  id %u", v.Type, static_cast<unsigned>(entt::to_entity(entity.GetHandle())));
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 2));
}

void InspectorPanel::DrawCoreComponents(SIMPEngine::Entity &entity)
{
    using namespace EditorUI;
    EditorContext &ctx = *m_Context;

    DrawComponentSection<TransformComponent>(ctx, "Transform", entity, [](TransformComponent &tc)
                                             {
        if (PropertyVec2("Position", &tc.position.x, 1.0f, "%.1f"))
            tc.dirty = true;
        if (PropertyFloat("Rotation", &tc.rotation, 0.5f, 0.0f, 0.0f, "%.1f°"))
            tc.dirty = true;
        if (PropertyVec2("Scale", &tc.scale.x, 0.01f, "%.2f"))
            tc.dirty = true;
        if (PropertyFloat("Z Index", &tc.zIndex, 0.1f, -1000.0f, 1000.0f, "%.1f"))
            tc.dirty = true; }, false);

    DrawComponentSection<RenderComponent>(ctx, "Shape", entity, [&entity](RenderComponent &rc)
                                          {
        PropertyColor("Color", rc.color);

        float size[2] = {rc.width, rc.height};
        if (PropertyVec2("Size", size, 0.5f, "%.1f"))
        {
            rc.width = size[0];
            rc.height = size[1];
            if (entity.HasComponent<SpriteComponent>())
            {
                auto &sprite = entity.GetComponent<SpriteComponent>();
                sprite.width = rc.width;
                sprite.height = rc.height;
            }
        } });

    DrawComponentSection<TextComponent>(ctx, "Text", entity, [](TextComponent &tx)
                                        {
        PropertyText("Text", tx.Text, true);
        PropertyFloat("Size", &tx.Size, 0.5f, 1.0f, 1000.0f, "%.0f");
        PropertyColor("Color", tx.Color);
        static const char *aligns[] = {"Left", "Center", "Right"};
        PropertySegmented("Align", &tx.Align, aligns, 3);
        PropertyText("Font", tx.FontPath);
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                std::string path(static_cast<const char *>(payload->Data));
                std::string ext = EditorUI::LowerExtension(path);
                if (ext == ".ttf" || ext == ".otf")
                {
                    NotifyEdit();
                    tx.FontPath = path;
                }
            }
            ImGui::EndDragDropTarget();
        } });

    DrawComponentSection<VelocityComponent>(ctx, "Velocity", entity, [](VelocityComponent &vc)
                                            {
        float v[2] = {vc.vx, vc.vy};
        if (PropertyVec2("Velocity", v, 1.0f, "%.1f"))
        {
            vc.vx = v[0];
            vc.vy = v[1];
        } });

    DrawComponentSection<CameraComponent>(ctx, "Camera", entity, [](CameraComponent &cc)
                                          {
        PropertyBool("Primary", &cc.primary);
        float zoom = cc.Camera.GetZoom();
        if (PropertyFloat("Zoom", &zoom, 0.01f, 0.1f, 10.0f, "%.2f"))
            cc.Camera.SetZoom(zoom); });

    DrawComponentSection<CollisionComponent>(ctx, "Collision", entity, [](CollisionComponent &col)
                                             {
        float size[2] = {col.width, col.height};
        if (PropertyVec2("Size", size, 0.5f, "%.1f"))
        {
            col.width = size[0];
            col.height = size[1];
        }
        float offset[2] = {col.offsetX, col.offsetY};
        if (PropertyVec2("Offset", offset, 0.5f, "%.1f"))
        {
            col.offsetX = offset[0];
            col.offsetY = offset[1];
        } });

    DrawComponentSection<LifetimeComponent>(ctx, "Lifetime", entity, [](LifetimeComponent &lf)
                                            { PropertyFloat("Remaining", &lf.remaining, 0.1f, 0.0f, 10000.0f, "%.1f s"); });

    DrawComponentSection<TilemapComponent>(ctx, "Tilemap", entity, [](TilemapComponent &tm)
                                           {
        PropertyFloat("Tile Size", &tm.tileSize, 0.5f, 1.0f, 1024.0f, "%.0f");
        char chunks[32];
        std::snprintf(chunks, sizeof(chunks), "%zu", tm.chunks.size());
        PropertyValue("Chunks", chunks);
        PropertyValue("Tileset", tm.tileset ? "Assigned" : "None", tm.tileset ? nullptr : &Colors::TextFaint); });

    DrawComponentSection<ScriptComponent>(ctx, "Script", entity, [](ScriptComponent &sc)
                                          {
        PropertyValue("Type", "Native (C++)");
        PropertyValue("State", sc.Instance ? "Running" : "Not started", sc.Instance ? &Colors::Success : &Colors::TextDim); }, false);

    SIMPEngine::Scene *scene = m_Context->Scene;
    DrawComponentSection<HierarchyComponent>(ctx, "Hierarchy", entity, [scene](HierarchyComponent &hc)
                                             {
        auto &registry = scene->GetRegistry();
        const char *parentName = "None";
        if (hc.parent != entt::null && registry.valid(hc.parent) && registry.all_of<TagComponent>(hc.parent))
            parentName = registry.get<TagComponent>(hc.parent).Tag.c_str();
        PropertyValue("Parent", parentName, hc.parent != entt::null ? nullptr : &Colors::TextFaint);
        char children[32];
        std::snprintf(children, sizeof(children), "%zu", hc.children.size());
        PropertyValue("Children", children); }, false);
}

void InspectorPanel::DrawAddComponentButton(SIMPEngine::Entity &entity)
{
    using namespace EditorUI;

    if (Button("Add Component", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
    {
        m_ComponentFilter[0] = '\0';
        ImGui::OpenPopup("##addcomponent");
    }

    ImVec2 min = ImGui::GetItemRectMin();
    ImGui::SetNextWindowPos(ImVec2(min.x, ImGui::GetItemRectMax().y + 2.0f), ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetItemRectSize().x, 0.0f), ImGuiCond_Appearing);
    if (ImGui::BeginPopup("##addcomponent"))
    {
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();
        SearchBox("##filter", m_ComponentFilter, sizeof(m_ComponentFilter), "Search");
        DrawAddComponentItems(*m_Context, entity, m_ComponentFilter);
        ImGui::EndPopup();
    }
}
