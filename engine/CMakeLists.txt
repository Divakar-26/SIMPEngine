cmake_minimum_required(VERSION 3.15)
project(SIMPEngine)

# Find SDL3 and spdlog (make sure you have SDL3 package config)
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(spdlog REQUIRED)

# ImGui library (assuming vendor/imgui is at project root)
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

# Engine library
add_library(Engine
    Engine/Window.cpp
    Engine/Application/Application.cpp
    Engine/Log.cpp
    Engine/UI/ImGuiLayer.cpp
    Engine/Rendering/Renderer.cpp
    Engine/Rendering/SDLRenderingAPI.cpp
    Engine/Rendering/RenderingAPI.cpp
    Engine/Rendering/Animation.cpp
    Engine/Rendering/Sprite.cpp
    Engine/Rendering/Texture.cpp
    Engine/Rendering/TextureManager.cpp
    Engine/Layer/RenderingLayer.cpp
    Engine/Input/Input.cpp
    Engine/Math/Camera2D.cpp
)

target_include_directories(Engine
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Engine
)

target_link_libraries(Engine
    PUBLIC SDL3::SDL3
    PUBLIC spdlog::spdlog
    PUBLIC imgui
    PUBLIC SDL3_image::SDL3_image 
)
