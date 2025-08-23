cmake_minimum_required(VERSION 3.16)
project(SIMPEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# === Third-party libraries (vendor builds) ===
add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)


# OS-specific options
if(WIN32)
    set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_VENDORED OFF CACHE BOOL "" FORCE)
endif()

set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/SDL_image EXCLUDE_FROM_ALL)

add_subdirectory(vendor/spdlog EXCLUDE_FROM_ALL)
add_subdirectory(vendor/glm EXCLUDE_FROM_ALL)

# ImGui wrapper (manual CMake target)
add_library(imgui STATIC
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/imgui_demo.cpp
    vendor/imgui/backends/imgui_impl_sdl3.cpp
    vendor/imgui/backends/imgui_impl_sdlrenderer3.cpp
)

target_include_directories(imgui PUBLIC
    ${CMAKE_SOURCE_DIR}/vendor/imgui
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends
)

target_link_libraries(imgui PUBLIC SDL3::SDL3 spdlog)

# === Engine, Editor, Game ===
add_subdirectory(Engine)
add_subdirectory(Editor)
add_subdirectory(Game)
