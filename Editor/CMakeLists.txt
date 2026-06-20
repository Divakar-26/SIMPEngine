# Create Editor library
add_library(Editor STATIC
    EditorLayer.cpp
    EditorCommands.cpp
    Panels/HierarchyPanel.cpp
    Panels/ViewportUtility.cpp
    Panels/ViewportPanel.cpp
    Panels/InspectorPanel/InspectorPanel.cpp
    Panels/ContentBrowserPanel.cpp
    Panels/AssetPickerPanel.cpp
    Panels/InspectorPanel/SpriteSection.cpp
    Panels/InspectorPanel/AnimationSection.cpp
    Panels/InspectorPanel/AnimationMakerPanel.cpp
    Panels/InspectorPanel/PhysicsSection.cpp
)

target_compile_definitions(Editor PRIVATE ENGINE_PROFILER_ENABLE_IMGUI)

target_include_directories(Editor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Panels
    # ${CMAKE_CURRENT_SOURCE_DIR}/icons
    ${CMAKE_SOURCE_DIR}/Engine 
    ${CMAKE_SOURCE_DIR}/Editor
)

target_sources(imgui PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/ImGuizmo/ImGuizmo.cpp
)


target_link_libraries(Editor
    PUBLIC Engine
    PUBLIC imgui
    PUBLIC SDL3::SDL3
    PUBLIC SDL3_image::SDL3_image
    PUBLIC spdlog::spdlog
)
