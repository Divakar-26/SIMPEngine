add_library(Editor STATIC
    EditorLayer.cpp
    EditorCommands.cpp
    EditorHistory.cpp
    EditorRecovery.cpp
    EditorCamera.cpp
    TileEditor.cpp
    Panels/HierarchyPanel.cpp
    Panels/ViewportPanel.cpp
    Panels/InspectorPanel/InspectorPanel.cpp
    Panels/ContentBrowserPanel.cpp
    Panels/AssetPickerPanel.cpp
    Panels/InspectorPanel/SpriteSection.cpp
    Panels/InspectorPanel/AnimationSection.cpp
    Panels/InspectorPanel/AnimationMakerPanel.cpp
    Panels/InspectorPanel/PhysicsSection.cpp
    Panels/ConsolePanel.cpp
    Panels/GameViewPanel.cpp
    Panels/SettingsPanel.cpp
    Panels/TilePalettePanel.cpp
    Panels/InspectorPanel/TilemapSection.cpp
    Panels/InspectorPanel/ParticleSection.cpp
    Panels/InspectorPanel/LightSection.cpp
    Panels/InspectorPanel/AudioSection.cpp
    Panels/InspectorPanel/LogicSection.cpp
    Panels/InspectorPanel/VectorSection.cpp
    Panels/ParticlePickerPanel.cpp
    Panels/ScriptEditorPanel.cpp
    Panels/TweenLabPanel.cpp
    UI/Theme.cpp
    UI/Widgets.cpp
    ScriptEditor/LuaLexer.cpp
    ScriptEditor/CodeTheme.cpp
    ScriptEditor/LuaCompletion.cpp
    ScriptEditor/CodeEditor.cpp
    Export/GameBuilder.cpp
    UI/FileDialogs.cpp
    Panels/ProjectSettingsPanel.cpp
    Panels/BuildPanel.cpp
    ProjectHub/RecentProjects.cpp
    ProjectHub/ProjectHubLayer.cpp
)

target_compile_definitions(Editor PRIVATE ENGINE_PROFILER_ENABLE_IMGUI)

target_include_directories(Editor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Panels
    ${CMAKE_SOURCE_DIR}/Engine
)

target_link_libraries(Editor
    PUBLIC Engine
    PUBLIC imgui
    PUBLIC SDL3::SDL3
    PUBLIC SDL3_image::SDL3_image
    PUBLIC spdlog::spdlog
)
