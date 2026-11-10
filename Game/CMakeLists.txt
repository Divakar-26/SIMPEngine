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

add_executable(ArrowOut
    ArrowOut/main.cpp
)

set_target_properties(ArrowOut PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Game")

if(NOT NO_EDITOR)
    target_link_libraries(ArrowOut
        PRIVATE Engine Editor imgui
    )
else()
    target_link_libraries(ArrowOut
        PRIVATE Engine imgui
    )
    target_compile_definitions(ArrowOut PRIVATE NO_EDITOR)
endif()

add_executable(Playground
    Playground/main.cpp
)

set_target_properties(Playground PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Game")

if(NOT NO_EDITOR)
    target_link_libraries(Playground
        PRIVATE Engine Editor imgui
    )
else()
    target_link_libraries(Playground
        PRIVATE Engine imgui
    )
    target_compile_definitions(Playground PRIVATE NO_EDITOR)
endif()

add_executable(Snake
    Snake/main.cpp
)

set_target_properties(Snake PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Game")

if(NOT NO_EDITOR)
    target_link_libraries(Snake
        PRIVATE Engine Editor imgui
    )
else()
    target_link_libraries(Snake
        PRIVATE Engine imgui
    )
    target_compile_definitions(Snake PRIVATE NO_EDITOR)
endif()

add_executable(Logo
    Logo/main.cpp
)

set_target_properties(Logo PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Game")

if(NOT NO_EDITOR)
    target_link_libraries(Logo
        PRIVATE Engine Editor imgui
    )
else()
    target_link_libraries(Logo
        PRIVATE Engine imgui
    )
    target_compile_definitions(Logo PRIVATE NO_EDITOR)
endif()
