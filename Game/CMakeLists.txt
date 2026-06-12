# Create GameApp executable
add_executable(GameApp
    main.cpp
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