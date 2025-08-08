add_library(imgui
    ${CMAKE_SOURCE_DIR}/vendor/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/vendor/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/vendor/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/vendor/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/vendor/imgui/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_sdl3.cpp
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_sdlrenderer3.cpp
)

target_include_directories(imgui PUBLIC
    ${CMAKE_SOURCE_DIR}/vendor/imgui
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends
)

target_link_libraries(imgui
    PUBLIC SDL3::SDL3
    PUBLIC spdlog::spdlog
)

# Your existing Engine library:
add_library(Engine
    src/Window.cpp
    src/Application.cpp
    src/Log.cpp
    src/ImGuiLayer.cpp
)

target_include_directories(Engine
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(Engine
    PUBLIC SDL3::SDL3
    PUBLIC spdlog::spdlog
    PUBLIC imgui
)
