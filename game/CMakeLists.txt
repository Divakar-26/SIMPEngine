# Create GameApp executable
add_executable(GameApp
    main.cpp
    EditorLayer.cpp
    Panels/ViewportPanel.cpp
)

# Link game with engine library
target_link_libraries(GameApp
    PRIVATE Engine imgui
)
