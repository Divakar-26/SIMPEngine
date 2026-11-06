#pragma once
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Component.h>

#include "../../EditorContext.h"
#include "../../EditorCommands.h"
#include "UI/Visuals.h"
#include "UI/Widgets.h"
#include <EditorPanel.h>
#include "AssetPickerPanel.h"
#include "AnimationMakerPanel.h"

#include <Engine/Core/Log.h>
#include <Engine/Core/FileSystem.h>
#include <Engine/Core/VFS.h>
#include <algorithm>
#include <cctype>
#include <filesystem>

class AssetPickerPanel;
class AnimationMakerPanel;

template <typename T, typename UIFunc>
static void DrawComponentSection(
    EditorContext &ctx,
    const char *title,
    SIMPEngine::Entity entity,
    UIFunc uiFunc,
    bool removable = true)
{
    if (!entity.HasComponent<T>())
        return;

    bool remove = false;
    if (EditorUI::BeginSection(title, title, removable ? &remove : nullptr))
        uiFunc(entity.GetComponent<T>());
    EditorUI::EndSection();

    if (remove)
    {
        EditorCommands::RecordEdit(ctx);
        entity.RemoveComponent<T>();
        ctx.Scene->MarkPhysicsDirty();
    }
}

class InspectorPanel : public EditorPanel
{
public:
    static constexpr const char *WindowId = "###Inspector";

    InspectorPanel(EditorContext *m_Context);

    void OnImGuiRender() override;

    void DrawSpriteSection(SIMPEngine::Entity &m_SelectedEntity);
    void DrawTilemapSection(SIMPEngine::Entity &m_SelectedEntity);
    void DrawAnimationSection(SIMPEngine::Entity &m_SelectedEntity);
    void DrawPhysicsSection(SIMPEngine::Entity &m_SelectedEntity);

    void SetAssetPicker(AssetPickerPanel *picker)
    {
        m_AssetPicker = picker;
    }

    void SetAnimationMaker(
        AnimationMakerPanel *maker)
    {
        m_AnimationMaker = maker;
    }

private:
    void DrawHeader(SIMPEngine::Entity &entity);
    void DrawSceneProperties();
    void DrawCoreComponents(SIMPEngine::Entity &entity);
    void DrawAddComponentButton(SIMPEngine::Entity &entity);

    EditorContext *m_Context;

    char m_ComponentFilter[64] = "";

    AssetPickerPanel *m_AssetPicker = nullptr;
    AnimationMakerPanel* m_AnimationMaker = nullptr;
};
