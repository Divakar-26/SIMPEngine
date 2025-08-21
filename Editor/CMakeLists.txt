add_executable(Editor
    EditorLayer.cpp
    Panels/ViewportPanel.cpp
)

target_link_libraries(Editor
    PRIVATE Engine imgui
)
