# Create Engine library from Window.cpp
add_library(Engine
    src/Window.cpp
    src/Application.cpp
)

# Make "include" folder public for #include "Engine/Window.h"
target_include_directories(Engine
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link Engine with SDL3 from vendor
target_link_libraries(Engine
    PUBLIC SDL3::SDL3
)
