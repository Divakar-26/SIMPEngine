# Create GameApp executable
add_executable(GameApp
    main.cpp
)

option(NO_EDITOR "Build Game Without Editor" ON)
if(NOT NO_EDITOR)
    target_link_libraries(GameApp
        PRIVATE Engine Editor imgui
    )
else()
    target_link_libraries(GameApp
        PRIVATE Engine imgui
    )
    target_compile_definitions(GameApp PRIVATE NO_EDITOR)
endif()