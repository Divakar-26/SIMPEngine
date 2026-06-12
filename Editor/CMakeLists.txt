# Create Editor library
add_library(Editor STATIC
    EditorLayer.cpp
    Panels/HierarchyPanel.cpp
    Panels/ViewportPanel.cpp
    Panels/InspectorPanel.cpp
    Panels/ContentBrowserPanel.cpp
)

target_include_directories(Editor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Panels
    # ${CMAKE_CURRENT_SOURCE_DIR}/icons
    ${CMAKE_SOURCE_DIR}/Engine 
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
