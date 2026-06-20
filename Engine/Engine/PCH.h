// -----Events--------------------
#include <Engine/Events/ApplicationEvent.h>
#include <Engine/Events/EventDispatcher.h>
#include <Engine/Events/KeyEvent.h>
#include <Engine/Events/MouseEvent.h>

// -----utils-----
#include <Engine/Core/Log.h>
#include <Engine/Core/TimeStep.h>
#include <Engine/Core/VFS.h>
#include <Engine/Input/Input.h>
#include <Engine/Input/SIMP_Keys.h>
#include <Engine/Math/EntityUtility.h>

// ------Rendering-----------------
#include <Engine/Rendering/GLRenderingAPI.h>
#include <Engine/Rendering/TextureManager.h>
#include <Engine/Rendering/Sprite.h>
#include <Engine/Rendering/Animation.h>
// #include <Engine/Rendering/SDLRenderingAPI.h>

#include <iostream>
#include <functional>
#include <memory>


#include <Engine/Application/Application.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneManager.h>
#include <Engine/Scene/SceneSerializer.h>
#include <Engine/Layer/LayerStack.h>
#include <Engine/UI/ImGuiLayer.h>
#include <Engine/Layer/RenderingLayer.h>


