#pragma once
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Component.h>
#include <../EditorContext.h>
#include <../EditorCommands.h>
#include <EditorPanel.h>
#include <filesystem>
#include "AssetPickerPanel.h"

class AssetPickerPanel;

class InspectorPanel : public EditorPanel
{
public:
    InspectorPanel(EditorContext *m_Context);

    void OnImGuiRender() override;

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
