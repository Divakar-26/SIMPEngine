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
    Engine/Rendering/Bloom.cpp
    Engine/Rendering/SpriteSheetSlicer.cpp
    Engine/Rendering/Sprite.cpp
    Engine/Rendering/Texture.cpp
    Engine/Rendering/TextureManager.cpp
    Engine/Rendering/Framebuffer.cpp
    Engine/Rendering/Font.cpp
    Engine/Rendering/ImageWriter.cpp
    Engine/Layer/RenderingLayer.cpp
    Engine/Input/Input.cpp
    Engine/Core/Display.cpp
    Engine/Rendering/GameView.cpp
    Engine/Input/InputActions.cpp
    Engine/Input/TouchControls.cpp
    Engine/Math/Camera2D.cpp
    Engine/Math/EntityUtility.cpp
    Engine/Scene/Scene.cpp
    Engine/Scene/SceneManager.cpp
    Engine/Scene/Systems/CameraSystem.cpp
    Engine/Scene/Systems/PhysicsSystem.cpp
    Engine/Physics/BodyFactory.cpp
    Engine/Scene/Systems/RenderSystem.cpp
    Engine/Scene/Systems/TilemapSystem.cpp
    Engine/Scene/Systems/ParticleSystem.cpp
    Engine/Scene/Systems/LightingSystem.cpp
    Engine/Scene/Systems/GlowSystem.cpp
    Engine/Scene/Systems/AudioSystem.cpp
    Engine/Audio/AudioEngine.cpp
    Engine/Particles/ParticleEmitter.cpp
    Engine/Particles/ParticlePresets.cpp
    Engine/Particles/ParticleSerializer.cpp
    Engine/Scene/Systems/AnimationSystem.cpp
    Engine/Scene/Systems/CollisionSystem.cpp
    Engine/Scene/Systems/HierarchySystem.cpp
    Engine/Scene/Systems/LifetimeSystem.cpp
    Engine/Scene/SceneSerializer.cpp
    Engine/Scene/Prefab.cpp
    Engine/Scene/Loaders/TiledLoader.cpp
    Engine/Scene/TilemapUtils.cpp
    Engine/Rendering/SvgTextureCache.cpp
    Engine/Scene/Systems/MovementSystem.cpp
    Engine/Scene/Systems/TriggerSystem.cpp
    Engine/Scene/Systems/TimerSystem.cpp
    Engine/Scene/Systems/PathFollowerSystem.cpp
    Engine/Scene/Systems/HealthSystem.cpp
    Engine/Animation/Curves.cpp
    Engine/Animation/EffectPresets.cpp
    Engine/Motion/MotionHost.cpp
    Engine/Animation/Tween.cpp
    Engine/Vector/VectorPath.cpp
    Engine/Vector/VectorTessellator.cpp
    Engine/Vector/VectorShape.cpp
    Engine/Vector/TextOutline.cpp
    Engine/Scene/Systems/VectorSystem.cpp
    Engine/Scene/Systems/CharacterControllerSystem.cpp
    Engine/Audio/Music.cpp
    Engine/Scene/Systems/DamageSystem.cpp
    Engine/Scene/Systems/FollowSystem.cpp
    Engine/Scene/Systems/PickupSystem.cpp
    Engine/Scene/Systems/RespawnSystem.cpp
    Engine/Scene/Systems/MusicSystem.cpp
    Engine/Scene/Systems/PickableSystem.cpp
    Engine/Scene/Systems/ButtonSystem.cpp
    Engine/Scene/Systems/UISystem.cpp
    Engine/Scene/Systems/ShaderCanvasSystem.cpp
    Engine/Rendering/ShaderEffect.cpp
    Engine/Scene/Systems/TeleporterSystem.cpp
    Engine/Scene/Systems/CameraZoneSystem.cpp
    Engine/Scene/Systems/BoundsSystem.cpp
    Engine/Core/GameVariables.cpp
    Engine/Scene/Systems/OscillatorSystem.cpp
    Engine/Scene/Systems/SpawnerSystem.cpp
    Engine/Project/Project.cpp
    Engine/Application/FrameCapture.cpp
    Engine/Core/CrashHandler.cpp
    Engine/Scripting/LuaRuntime.cpp
    Engine/Scripting/LuaBindings.cpp
    Engine/Scripting/LuaScript.cpp
    Engine/Scripting/LuaHelpers.cpp
    Engine/Scripting/LuaCallback.cpp
    Engine/Scripting/LuaTween.cpp
    Engine/Scripting/LuaPhysics.cpp
    Engine/Scripting/LuaInputActions.cpp
    Engine/Scripting/LuaPath.cpp
    Engine/Scripting/LuaDraw.cpp
    Engine/Scripting/LuaColor.cpp
    Engine/Scripting/LuaEffects.cpp
    Engine/Scripting/LuaPrelude.cpp
    Engine/Scripting/Components/LuaVector.cpp
    Engine/Scripting/Components/LuaComponents.cpp
    Engine/Scripting/Components/LuaSprite.cpp
    Engine/Scripting/Components/LuaCamera.cpp
    Engine/Scripting/Components/LuaLight.cpp
    Engine/Scripting/Components/LuaVelocity.cpp
    Engine/Scripting/Components/LuaCollision.cpp
    Engine/Scripting/Components/LuaShape.cpp
    Engine/Scripting/Components/LuaText.cpp
    Engine/Scripting/Components/LuaGlow.cpp
    Engine/Scripting/Components/LuaParticles.cpp
    Engine/Scripting/Components/LuaAudioSource.cpp
    Engine/Scripting/Components/LuaParallax.cpp
    Engine/Scripting/Components/LuaBody.cpp
    Engine/Scripting/Components/LuaTrigger.cpp
    Engine/Scripting/Components/LuaTimer.cpp
    Engine/Scripting/Components/LuaDamage.cpp
    Engine/Scripting/Components/LuaOscillator.cpp
    Engine/Scripting/Components/LuaSpawner.cpp
    Engine/Scripting/Components/LuaFollow.cpp
    Engine/Scripting/Components/LuaPickup.cpp
    Engine/Scripting/Components/LuaCheckpoint.cpp
    Engine/Scripting/Components/LuaRespawn.cpp
    Engine/Scripting/Components/LuaMusic.cpp
    Engine/Scripting/Components/LuaPickable.cpp
    Engine/Scripting/Components/LuaButton.cpp
    Engine/Scripting/Components/LuaTeleporter.cpp
    Engine/Scripting/Components/LuaUI.cpp
    Engine/Scripting/Components/LuaShaderCanvas.cpp
    Engine/Scripting/Components/LuaCameraZone.cpp
    Engine/Scripting/Components/LuaWorldBounds.cpp
    Engine/Scripting/Components/LuaPath.cpp
    Engine/Scripting/Components/LuaHealth.cpp
    Engine/Scripting/Components/LuaCharacterController.cpp
    Engine/Scripting/Components/LuaTilemap.cpp
    Engine/Scripting/Components/LuaAnimation.cpp
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

target_compile_definitions(Engine PUBLIC ENGINE_PROFILER_ENABLE_IMGUI SIMP_VERSION_STRING="${PROJECT_VERSION}")
if(SIMP_GLES)
    target_compile_definitions(Engine PUBLIC SIMP_GLES)
endif()
option(SIMP_PROFILER "Keep PROFILE_SCOPE timers in optimised builds" OFF)
if(NOT SIMP_PROFILER)
    target_compile_definitions(Engine PUBLIC $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:ENGINE_PROFILER_DISABLED>)
endif()
target_precompile_headers(Engine PRIVATE Engine/PCH.h)

if(NOT ANDROID)
    target_link_libraries(Engine PUBLIC SDL3_image::SDL3_image)
endif()

target_link_libraries(Engine
    PUBLIC SDL3::SDL3
    PUBLIC spdlog
    PUBLIC imgui
    PUBLIC glad
    PUBLIC stb
    PUBLIC nlohmann_json::nlohmann_json
    PUBLIC yaml-cpp
    PUBLIC AccelEngine
    PUBLIC lua
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(Engine INTERFACE -rdynamic)
endif()

if(MSVC)
    set_source_files_properties(Engine/Audio/StbVorbisImpl.c Engine/Rendering/StbImageWriteImpl.c PROPERTIES COMPILE_OPTIONS "/w" SKIP_PRECOMPILE_HEADERS ON)
else()
    set_source_files_properties(Engine/Audio/StbVorbisImpl.c Engine/Rendering/StbImageWriteImpl.c PROPERTIES COMPILE_OPTIONS "-w" SKIP_PRECOMPILE_HEADERS ON)
endif()
target_sources(Engine PRIVATE Engine/Audio/StbVorbisImpl.c Engine/Rendering/StbImageWriteImpl.c)
