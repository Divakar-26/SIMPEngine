    add_library(Engine STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Core/Window.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Core/Log.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Core/FileSystem.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Core/VFS.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Assets/AssetRegistry.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Assets/TextureImporter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Assets/Importers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Assets/AssetManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Application/Application.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/UI/ImGuiLayer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/Renderer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/GLRenderingAPI.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/Shader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/RenderingAPI.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/Animation.cpp
        # ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/SpriteAtlas.cpp
        # ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/Sprite.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/Texture.cpp
        # ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Rendering/TextureManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Layer/RenderingLayer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Input/Input.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Math/Camera2D.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/Scene.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/SceneManager.cpp
        # ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/ScriptableEntity.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/Systems/CameraSystem.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Scene/SceneSerializer.cpp
        # ${CMAKE_SOURCE_DIR}/vendor/glad/src/glad.c
    )

    target_include_directories(Engine
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Engine    # D:/SIMPEngine/Engine/Engine
        PUBLIC ${CMAKE_SOURCE_DIR}/vendor/entt/src
        PUBLIC ${CMAKE_SOURCE_DIR}/vendor/glm
        PUBLIC ${CMAKE_SOURCE_DIR}/vendor/glad/include
        PUBLIC ${CMAKE_SOURCE_DIR}/vendor/yaml-cpp/include
        PUBLIC ${CMAKE_SOURCE_DIR}/vendor/box2d/include
    )

    target_precompile_headers(Engine PRIVATE Engine/PCH.h)



    target_link_libraries(Engine
        PUBLIC SDL3::SDL3
        PUBLIC SDL3_image::SDL3_image
        PUBLIC spdlog
        PUBLIC imgui
        PUBLIC glad
        PUBLIC stb
        PUBLIC yaml-cpp
        PUBLIC box2d
    )
