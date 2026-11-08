add_executable(GameApp
    main.cpp
    Mossveil/Builder.cpp
    Mossveil/Levels.cpp
)

set_target_properties(GameApp PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Game")

option(NO_EDITOR "Build Game Without Editor" OFF)
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

add_executable(ScriptDemo
    ScriptDemo/main.cpp
)

set_target_properties(ScriptDemo PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Game")

if(NOT NO_EDITOR)
    target_link_libraries(ScriptDemo
        PRIVATE Engine Editor imgui
    )
else()
    target_link_libraries(ScriptDemo
        PRIVATE Engine imgui
    )
    target_compile_definitions(ScriptDemo PRIVATE NO_EDITOR)
endif()
