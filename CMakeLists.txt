cmake_minimum_required(VERSION 3.16)
project(SIMPEngine VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/permissive- /W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(ANDROID)
    set(SIMP_GLES_DEFAULT ON)
else()
    set(SIMP_GLES_DEFAULT OFF)
endif()
option(SIMP_GLES "Render through OpenGL ES 3 instead of desktop OpenGL 4 (always on for Android)" ${SIMP_GLES_DEFAULT})

add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)

if(WIN32)
    set(SDLIMAGE_VENDORED OFF CACHE BOOL "" FORCE)
endif()

set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
if(NOT ANDROID)
    add_subdirectory(vendor/SDL_image EXCLUDE_FROM_ALL)
endif()

add_subdirectory(vendor/spdlog EXCLUDE_FROM_ALL)
add_subdirectory(vendor/glm EXCLUDE_FROM_ALL)
add_subdirectory(vendor/yaml-cpp EXCLUDE_FROM_ALL)
add_subdirectory(vendor/AccelEngine)
target_include_directories(AccelEngine PRIVATE ${CMAKE_SOURCE_DIR}/vendor/SDL/include)
add_subdirectory(vendor/json)

option(SIMP_ASAN "Build project code with AddressSanitizer (vendored libraries are not instrumented)" OFF)
if(SIMP_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

add_library(imgui STATIC
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/imgui_demo.cpp
    vendor/imgui/misc/cpp/imgui_stdlib.cpp
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
    ${CMAKE_SOURCE_DIR}/vendor/json
    ${CMAKE_SOURCE_DIR}/vendor/nanosvg
)

if(SIMP_GLES)
    target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_ES3)
    add_library(glad INTERFACE)
    if(ANDROID)
        target_link_libraries(glad INTERFACE GLESv3 EGL)
    else()
        target_link_libraries(glad INTERFACE GLESv2 EGL)
    endif()
else()
    add_library(glad STATIC vendor/glad/src/glad.c)

    target_include_directories(glad PUBLIC
        ${CMAKE_SOURCE_DIR}/vendor/glad/include
    )

    set_target_properties(glad PROPERTIES LINKER_LANGUAGE C)
endif()

target_link_libraries(imgui PUBLIC SDL3::SDL3 spdlog)

add_subdirectory(Engine)
if(ANDROID)
    add_subdirectory(Android/native)
else()
    add_subdirectory(Editor)
    add_subdirectory(Game)
    add_subdirectory(Apps)

    option(SIMP_BUILD_TESTS "Build the SIMPTests executable" ON)
    if(SIMP_BUILD_TESTS)
        enable_testing()
        add_subdirectory(tests)
    endif()
endif()

file(GLOB LUA_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/vendor/lua/*.c"
)

list(REMOVE_ITEM LUA_SOURCES
    "${CMAKE_SOURCE_DIR}/vendor/lua/lua.c"
    "${CMAKE_SOURCE_DIR}/vendor/lua/luac.c"
    "${CMAKE_SOURCE_DIR}/vendor/lua/onelua.c"
    "${CMAKE_SOURCE_DIR}/vendor/lua/ltests.c"
)

add_library(lua STATIC ${LUA_SOURCES})

target_include_directories(lua PUBLIC
    ${CMAKE_SOURCE_DIR}/vendor/lua
)

set_target_properties(lua PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(lua PRIVATE /W0)
else()
    target_compile_options(lua PRIVATE -w)
endif()

if(NOT WIN32)
    target_compile_definitions(lua PRIVATE LUA_USE_POSIX)
endif()

option(SIMP_INSTALL "Generate install rules for the SIMPEngine library" ON)
if(SIMP_INSTALL AND NOT SIMP_GLES)
    include(cmake/SIMPEngineInstall.cmake)
endif()
