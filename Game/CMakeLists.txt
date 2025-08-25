# Create GameApp executable
add_executable(GameApp
    main.cpp
    # EditorLayer.cpp
    # Panels/ViewportPanel.cpp
    # Panels/HierarchyPanel.cpp
)

option(NO_EDITOR "Build Game Without Editor" OFF)
if(NOT NO_EDITOR)
    target_link_libraries(GameApp
    PRIVATE Engine Editor imgui 
    )
else()
    target_link_libraries(GameApp
    PRIVATE Engine Editor imgui
    )
    target_compile_definitions(GameApp PRIVATE NO_EDITOR)
endif()

# Link game with engine library

