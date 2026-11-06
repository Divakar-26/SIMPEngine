add_library(Engine STATIC
    Engine/Core/Window.cpp
    Engine/Core/EngineDimensions.cpp
    Engine/Core/Log.cpp
    Engine/Core/FileSystem.cpp
    Engine/Core/VFS.cpp
    Engine/Assets/AssetRegistry.cpp
    Engine/Assets/TextureImporter.cpp
    Engine/Assets/Importers.cpp
    Engine/Assets/AssetManager.cpp
    Engine/Application/Application.cpp
    Engine/UI/ImGuiLayer.cpp
    Engine/Rendering/Renderer.cpp
    Engine/Rendering/GLRenderingAPI.cpp
    Engine/Rendering/Shader.cpp
    Engine/Rendering/RenderingAPI.cpp
    Engine/Rendering/SVGDocument.cpp
    Engine/Rendering/Animation.cpp
    Engine/Rendering/AnimationController.cpp
    Engine/Rendering/Tileset.cpp
    Engine/Rendering/Sprite.cpp
    Engine/Rendering/Texture.cpp
    Engine/Rendering/TextureManager.cpp
    Engine/Rendering/Framebuffer.cpp
    Engine/Rendering/Font.cpp
    Engine/Layer/RenderingLayer.cpp
    Engine/Input/Input.cpp
    Engine/Math/Camera2D.cpp
    Engine/Math/EntityUtility.cpp
    Engine/Scene/Scene.cpp
    Engine/Scene/SceneManager.cpp
    Engine/Scene/Systems/CameraSystem.cpp
    Engine/Scene/Systems/PhysicsSystem.cpp
    Engine/Scene/Systems/RenderSystem.cpp
    Engine/Scene/Systems/TilemapSystem.cpp
    Engine/Scene/Systems/ParticleSystem.cpp
    Engine/Particles/ParticleEmitter.cpp
    Engine/Particles/ParticlePresets.cpp
    Engine/Particles/ParticleSerializer.cpp
    Engine/Scene/Systems/AnimationSystem.cpp
    Engine/Scene/Systems/CollisionSystem.cpp
    Engine/Scene/Systems/HierarchySystem.cpp
    Engine/Scene/Systems/LifetimeSystem.cpp
    Engine/Scene/SceneSerializer.cpp
    Engine/Scene/Loaders/TiledLoader.cpp
    Engine/Scene/TilemapUtils.cpp
    Engine/Rendering/SvgTextureCache.cpp
    Engine/Scene/Systems/MovementSystem.cpp
)

target_include_directories(Engine
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Engine
    PUBLIC ${CMAKE_SOURCE_DIR}/vendor/entt/src
    PUBLIC ${CMAKE_SOURCE_DIR}/vendor/nanosvg/src
    PUBLIC ${CMAKE_SOURCE_DIR}/vendor/glm
    PUBLIC ${CMAKE_SOURCE_DIR}/vendor/glad/include
    PUBLIC ${CMAKE_SOURCE_DIR}/vendor/yaml-cpp/include
    PUBLIC ${CMAKE_SOURCE_DIR}/vendor/AccelEngine/include
)

target_compile_definitions(Engine PUBLIC ENGINE_PROFILER_ENABLE_IMGUI)
target_precompile_headers(Engine PRIVATE Engine/PCH.h)

target_link_libraries(Engine
    PUBLIC SDL3::SDL3
    PUBLIC SDL3_image::SDL3_image
    PUBLIC spdlog
    PUBLIC imgui
    PUBLIC glad
    PUBLIC stb
    PUBLIC yaml-cpp
    PUBLIC AccelEngine
)
