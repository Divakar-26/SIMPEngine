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

add_executable(Sudoku
    Sudoku/main.cpp
)

set_target_properties(Sudoku PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Game")

if(NOT NO_EDITOR)
    target_link_libraries(Sudoku
        PRIVATE Engine Editor imgui
    )
else()
    target_link_libraries(Sudoku
        PRIVATE Engine imgui
    )
    target_compile_definitions(Sudoku PRIVATE NO_EDITOR)
endif()

add_executable(CrossSum
    CrossSum/main.cpp
)

set_target_properties(CrossSum PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Game")

if(NOT NO_EDITOR)
    target_link_libraries(CrossSum
        PRIVATE Engine Editor imgui
    )
else()
    target_link_libraries(CrossSum
        PRIVATE Engine imgui
    )
    target_compile_definitions(CrossSum PRIVATE NO_EDITOR)
endif()

add_executable(Untangle
    Untangle/main.cpp
)

set_target_properties(Untangle PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Game")

if(NOT NO_EDITOR)
    target_link_libraries(Untangle
        PRIVATE Engine Editor imgui
    )
else()
    target_link_libraries(Untangle
        PRIVATE Engine imgui
    )
    target_compile_definitions(Untangle PRIVATE NO_EDITOR)
endif()

add_executable(NeuralNetVideo
    NeuralNetVideo/main.cpp
)

set_target_properties(NeuralNetVideo PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Game")
target_link_libraries(NeuralNetVideo PRIVATE Engine)
target_compile_definitions(NeuralNetVideo PRIVATE SIMP_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")

add_executable(IntegrationVideo
    IntegrationVideo/main.cpp
)

set_target_properties(IntegrationVideo PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Game")
target_link_libraries(IntegrationVideo PRIVATE Engine)
target_compile_definitions(IntegrationVideo PRIVATE SIMP_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
