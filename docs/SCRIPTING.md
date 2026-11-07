# Lua scripting

Lua scripts run through the same `ScriptComponent` as C++ scripts, so Play/Stop snapshots, duplicate and removal behave the same way.

## Binding a script

```cpp
registry.emplace<ScriptComponent>(entity).BindLua("assets://scripts/rotator.lua");
```

The file is loaded when the entity's script is first updated (the same lazy creation as C++ scripts). Paths go through the VFS. Files are cached per path and reloaded when their modification time changes, so editing a script and pressing Play again picks up the change.

## Script shape

A script file returns a table. Every hook is optional.

```lua
local Mover = {}

Mover.speed = 4

function Mover:OnCreate()
end

function Mover:OnUpdate(dt)
    self.entity:Translate(self.speed * dt, 0)
end

function Mover:OnRender()
end

function Mover:OnDestroy()
end

return Mover
```

Each entity gets its own `self` table that falls back to the returned table, so fields on the returned table act as defaults and `self.x = ...` is per entity. `self.entity` is the owning entity.

If a hook raises an error, the message and a traceback go to the log and that script instance is disabled until the next Play. Other scripts keep running.

## API

| Global | Members |
|---|---|
| `log` | `trace`, `info`, `warn`, `error` (any number of arguments); `print` is `log.info` |
| `Input` | `IsKeyDown`, `IsKeyJustPressed`, `IsKeyJustReleased`, `IsMouseDown`, `IsMouseJustPressed`, `IsMouseJustReleased`, `IsMouseInViewport`, `GetMouseWheel` |
| `Key` | constants named after `SIMPK_*` without the prefix: `Key.W`, `Key.SPACE`, `Key.LEFT`, `Key.F1`, `Key.NUM_0`, `Key.MOUSE_LEFT` |

Entity methods (`self.entity:Method()`):

| Method | Returns / effect |
|---|---|
| `IsValid()` | `true` while the entity exists |
| `GetName()`, `SetName(name)` | the `TagComponent` text |
| `GetPosition()`, `SetPosition(x, y)`, `Translate(dx, dy)` | position in world units |
| `GetRotation()`, `SetRotation(degrees)` | rotation in degrees |
| `GetScale()`, `SetScale(x [, y])` | scale |
| `GetMouseWorld()` | mouse position in world space |
| `FindByName(name)` | first entity with that name in the same scene, or `nil` |
| `Destroy([delay])` | removes the entity after `delay` seconds (default 0) through `LifetimeComponent` |

| `GetSprite()`, `GetCamera()`, `GetLight()` | a component handle, or `nil` when the entity has no such component |

Calling a method on an entity that no longer exists raises a Lua error; check `IsValid()` first when holding on to another entity. Component handles raise the same error once the entity or the component is gone.

Component handles (colors are 0-255 and clamped):

| Handle | Methods |
|---|---|
| Sprite | `GetTint()` / `SetTint(r, g, b [, a])`, `GetFlip()` / `SetFlip(flipX, flipY)`, `GetSize()` / `SetSize(w, h)`, `SetRegion(x, y, w, h)` (texture pixels), `ClearRegion()` |
| Camera | `Shake(amount)`, `GetZoom()` / `SetZoom(z)`, `SetFollow(name [, offsetX, offsetY])` (empty name stops following), `IsPrimary()` |
| Light | `GetColor()` / `SetColor(r, g, b)`, `GetIntensity()` / `SetIntensity(v)`, `GetRadius()` / `SetRadius(v)`, `IsEnabled()` / `SetEnabled(bool)` |

```lua
local sprite = self.entity:GetSprite()
sprite:SetTint(255, 120, 120)
self.entity:FindByName("Camera"):GetCamera():Shake(0.5)
```

`require "name"` loads `assets/scripts/name.lua` and `assets/scripts/name/init.lua`. Native modules are disabled (`package.cpath` is empty).

## C++ side

| Piece | Role |
|---|---|
| `LuaRuntime` (`Scripting/LuaRuntime.h`) | owns the `lua_State`; `LuaRuntime::Get()` creates it on first use, `Application` shuts it down. `ExecuteFile`, `ExecuteString`, `Call` (protected call with traceback), `PushScriptClass` |
| `LuaScript` (`Scripting/LuaScript.h`) | a `ScriptableEntity` that hosts one script instance and forwards the hooks |
| `LuaBindings` (`Scripting/LuaBindings.cpp`) | registers `log`, `Input`, `Key`, the `Entity` userdata and the Sprite/Camera/Light handles |

Adding a binding: write a `lua_CFunction` in `LuaBindings.cpp`, add it to the matching `luaL_Reg` table. Do not keep `std::string` or other objects with destructors alive across `luaL_check*` or `luaL_error` calls, since Lua unwinds with `longjmp`.

## Not yet done

- The script path is not serialized with the scene and the inspector does not show it.
- Entities driven by a physics body should not be moved with `SetPosition`.
- Other components (physics, particles, audio, text, glow, tilemap) are not exposed; no fixed-timestep hook.
