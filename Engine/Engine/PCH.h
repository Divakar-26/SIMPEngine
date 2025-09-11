// -----Events--------------------
#include "Events/ApplicationEvent.h"
#include "Events/EventDispatcher.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"

// -----utils-----
#include "Core/Log.h"
#include "Core/TimeStep.h"
#include "Input/Input.h"
#include "Input/SIMP_Keys.h"

// ------Rendering-----------------
#include "Rendering/TextureManager.h"
#include "Rendering/Sprite.h"
#include "Rendering/Animation.h"
#include "Rendering/SpriteAtlas.h"
// #include "Rendering/SDLRenderingAPI.h"
#include "Rendering/GLRenderingAPI.h"

#include <iostream>
#include <functional>
#include <memory>

