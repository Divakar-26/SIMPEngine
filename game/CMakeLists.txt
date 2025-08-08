# Create GameApp executable
add_executable(GameApp
    src/main.cpp
)

# Link game with engine library
target_link_libraries(GameApp
    PRIVATE Engine
)
