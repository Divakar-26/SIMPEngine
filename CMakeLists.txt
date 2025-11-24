cmake_minimum_required(VERSION 3.16)
project(SIMPEngine LANGUAGES C CXX)

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
add_subdirectory(vendor/yaml-cpp EXCLUDE_FROM_ALL)
add_subdirectory(vendor/AccelEngine)


# ImGui wrapper (manual CMake target)
add_library(imgui STATIC
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp             
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/imgui_demo.cpp
    vendor/imgui/backends/imgui_impl_sdl3.cpp
    vendor/imgui/backends/imgui_impl_sdlrenderer3.cpp
    vendor/imgui/backends/imgui_impl_opengl3.cpp

    vendor/ImGuizmo/ImGuizmo.cpp
)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE
    ${CMAKE_SOURCE_DIR}/vendor/stb
)

target_include_directories(imgui PUBLIC
    ${CMAKE_SOURCE_DIR}/vendor/imgui
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends
    ${CMAKE_SOURCE_DIR}/vendor/ImGuizmo
)

# glad
add_library(glad STATIC vendor/glad/src/glad.c)

target_include_directories(glad PUBLIC
    ${CMAKE_SOURCE_DIR}/vendor/glad/include
)

set_target_properties(glad PROPERTIES LINKER_LANGUAGE C)

target_link_libraries(imgui PUBLIC SDL3::SDL3 spdlog)

# === Engine, Editor, Game ===
add_subdirectory(Engine)
add_subdirectory(Editor)
add_subdirectory(Game)
