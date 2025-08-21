# Create GameApp executable
add_executable(GameApp
    main.cpp
    EditorLayer.cpp
    Panels/ViewportPanel.cpp
    Panels/HierarchyPanel.cpp
)

# Link game with engine library
target_link_libraries(GameApp
    PRIVATE Engine imgui
)
