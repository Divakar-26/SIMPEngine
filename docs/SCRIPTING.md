# Lua scripting

Lua scripts run through the same `ScriptComponent` as C++ scripts, so Play/Stop snapshots, duplicate and removal behave the same way. The script path is saved in the scene (`Script: {lua: assets://scripts/x.lua}`).

## Binding a script

In the editor: Add Component > Script, then choose a `.lua` file, drop one from the content browser, or press New Script to create `assets/scripts/<EntityName>.lua` from a template.

From C++:

```cpp
registry.emplace<ScriptComponent>(entity).BindLua("assets://scripts/rotator.lua");
```

From Lua:

```lua
Scene.Spawn("Bullet", x, y):SetScript("assets://scripts/bullet.lua")
```

The file is loaded when the entity's script is first updated. Paths go through the VFS. Files are cached per path and reloaded when their modification time changes, so editing a script and pressing Play again picks up the change.

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

function Mover:OnFixedUpdate(dt)
end

function Mover:OnRender()
end

function Mover:OnDestroy()
end

return Mover
```

Each entity gets its own `self` table that falls back to the returned table, so fields on the returned table act as defaults and `self.x = ...` is per entity. `self.entity` is the owning entity.

If a hook raises an error, the message and a traceback go to the log and that script instance is disabled until the next Play. Other scripts keep running.

### Hooks

| Hook | When |
|---|---|
| `OnCreate()` | first update of the entity |
| `OnUpdate(dt)` | every frame |
| `OnFixedUpdate(dt)` | at a fixed rate (default 60 Hz, `Scene.SetFixedStep(seconds)`), zero or more times per frame, at most 5 catch-up steps |
| `OnRender()` | while the scene is drawn; do not add scripts or components from here |
| `OnDestroy()` | when the script component is destroyed |
| `OnTriggerEnter(other)`, `OnTriggerStay(other)`, `OnTriggerExit(other)` | a Trigger Zone on this entity, or one this entity (with a Collision component) overlaps; `other` is the other entity, or `nil` if it was destroyed |
| `OnCollision(other)` | AccelEngine reported a contact between this body and `other` during the step (deduplicated per frame) |
| `OnTimer()` | this entity's Timer fired |
| `OnDamaged(amount, source)` | Health took damage; `source` may be `nil` |
| `OnDeath(killer)` | Health reached zero; `killer` may be `nil` |
| `OnPathPoint(index)` | Path Follower reached a point (1-based) |
| `OnPathEnd()` | a `Once` path finished |
| `OnJump()` | the Character Controller jumped |
| `OnLand(speed)` | the Character Controller touched ground; `speed` is the fall speed at impact |

Events are delivered after the frame's `OnUpdate` calls. A script is created on its entity's first update, so events that arrive earlier are dropped.

## Globals

| Global | Members |
|---|---|
| `log` | `trace`, `info`, `warn`, `error` (any number of arguments); `print` is `log.info` |
| `Input` | `IsKeyDown`, `IsKeyJustPressed`, `IsKeyJustReleased`, `IsMouseDown`, `IsMouseJustPressed`, `IsMouseJustReleased`, `IsMouseInViewport`, `GetMouseWheel`, `GetAxis(negativeKey, positiveKey)` returning -1, 0 or 1 |
| `Key` | constants named after `SIMPK_*` without the prefix: `Key.W`, `Key.SPACE`, `Key.LEFT`, `Key.F1`, `Key.NUM_0`, `Key.MOUSE_LEFT` |
| `Time` | `Now()` scene seconds since Play, `Delta()` last frame time, `FixedDelta()` |
| `Scene` | `Spawn(name [, x, y [, parent]])`, `Find(name)`, `FindAll(name)`, `SetAmbient(r, g, b)` (0-255), `SetLighting(bool)`, `SetFixedStep(seconds)`, `IsSolidAt(x, y)` and `OverlapsSolid(x1, y1, x2, y2)` (against tilemaps marked solid) |
| `Audio` | `Play(path [, volume, pitch, loop])` returns a voice id, `Stop(voice)`, `StopAll()`, `IsPlaying(voice)`, `SetVolume(voice, v)`, `SetMasterVolume(v)`, `GetMasterVolume()` |
| `Tween`, `Curves` | Flutter-style animation: `Tween.To/From/FromTo/Value/Along/Spring/Sequence/Parallel/Stagger`, all Flutter curves. See [ANIMATION.md](ANIMATION.md) |
| `Path` | vector paths: `Path.Circle/Rect/Star/Heart/Svg/...`, builders and queries. See [ANIMATION.md](ANIMATION.md) |
| `Draw` | immediate drawing (lines, curves, polygons, paths, text) inside `OnRender`. See [ANIMATION.md](ANIMATION.md) |
| `Math` | `Lerp(a, b, t)`, `Clamp(v, min, max)`, `Sign(v)`, `MoveTowards(current, target, maxDelta)`, `Length(x, y)`, `Distance(x1, y1, x2, y2)`, `Normalize(x, y)`, `Angle(x1, y1, x2, y2)` in degrees |

`Scene.*` and `Time.*` use the scene of the script that is currently running. `Scene.Spawn` creates an entity with a Transform and a tag; with a parent, the position is local to that parent.

## Entity methods

`self.entity:Method()`:

| Method | Returns / effect |
|---|---|
| `IsValid()` | `true` while the entity exists |
| `GetName()`, `SetName(name)` | the `TagComponent` text |
| `GetPosition()`, `SetPosition(x, y)`, `Translate(dx, dy)` | local position; entities with a rigid body move the body too |
| `GetWorldPosition()` | position after parent transforms (as of the last update) |
| `GetRotation()`, `SetRotation(degrees)` | rotation in degrees |
| `GetScale()`, `SetScale(x [, y])` | scale |
| `GetZIndex()`, `SetZIndex(z)` | draw order |
| `GetMouseWorld()` | mouse position in world space |
| `FindByName(name)` | first entity with that name in the same scene, or `nil` |
| `GetParent()`, `GetChildren()` | parent entity or `nil`; table of child entities |
| `Destroy([delay])` | removes the entity after `delay` seconds (default 0) through `LifetimeComponent` |
| `GetScript()` | the `self` table of that entity's Lua script, or `nil` (no Lua script or not created yet) |
| `SetScript(path)` | binds a Lua script; fails if the entity already has a running script |
| `Get(name)`, `Has(name)`, `Add(name)`, `Remove(name)` | generic component access by name (below) |

Calling a method on an entity that no longer exists raises a Lua error; check `IsValid()` first when holding on to another entity. Component handles raise the same error once the entity or the component is gone.

`GetScript()` lets scripts talk to each other:

```lua
local boss = self.entity:FindByName("Boss"):GetScript()
boss:TakeDamage(3)
```

## Components

`entity:Get("Sprite")` returns a handle or `nil`. Every component also has a shortcut: `GetSprite()`, `GetCamera()`, `GetLight()`, `GetVelocity()`, `GetCollision()`, `GetShape()`, `GetText()`, `GetGlow()`, `GetParticles()`, `GetAudio()`, `GetParallax()`, `GetBody()`, `GetTrigger()`, `GetTimer()`, `GetPath()`, `GetHealth()`, `GetTilemap()`, `GetAnimation()`, `GetVector()`, `GetCharacterController()`. `Add(name)` adds the component if missing and returns the handle (Body cannot be added from a script). Colors are 0-255 and clamped.

| Handle | Methods |
|---|---|
| Sprite | `GetTint()` / `SetTint(r, g, b [, a])`, `GetFlip()` / `SetFlip(flipX, flipY)`, `GetSize()` / `SetSize(w, h)`, `SetRegion(x, y, w, h)` (texture pixels), `ClearRegion()` |
| Camera | `Shake(amount)`, `GetZoom()` / `SetZoom(z)`, `SetFollow(name [, offsetX, offsetY])` (empty name stops following), `IsPrimary()` |
| Light | `GetColor()` / `SetColor(r, g, b)`, `GetIntensity()` / `SetIntensity(v)`, `GetRadius()` / `SetRadius(v)`, `IsEnabled()` / `SetEnabled(bool)` |
| Velocity | `Get()`, `Set(x, y)`, `Add(dx, dy)` |
| Collision | `GetSize()` / `SetSize(w, h)`, `GetOffset()` / `SetOffset(x, y)`, `GetBounds()` returns world x, y, w, h |
| Shape | `GetColor()` / `SetColor(r, g, b [, a])`, `GetSize()` / `SetSize(w, h)` |
| Text | `GetText()` / `SetText(s)`, `GetFontSize()` / `SetFontSize(n)`, `GetColor()` / `SetColor(r, g, b [, a])`, `GetAlign()` / `SetAlign(0 left, 1 center, 2 right)` |
| Glow | `GetColor()` / `SetColor`, `GetIntensity()` / `SetIntensity`, `GetSpread()` / `SetSpread`, `IsEnabled()` / `SetEnabled` |
| Particles | `Play()`, `Stop()`, `IsPlaying()`, `Restart()`, `Burst(count)`, `GetRate()` / `SetRate(v)` |
| Audio | `Play()`, `Stop()`, `IsPlaying()`, `GetVolume()` / `SetVolume(v)`, `GetPitch()` / `SetPitch(v)`, `SetLoop(bool)`, `SetClip(path)` |
| Parallax | `GetFactor()` / `SetFactor(x [, y])` |
| Body | `GetVelocity()` / `SetVelocity(x, y)`, `AddImpulse(x, y)`, `GetSpin()` / `SetSpin(v)`, `IsStatic()`, `GetMass()` |
| Trigger | `GetSize()` / `SetSize(w, h)`, `GetOffset()` / `SetOffset(x, y)`, `GetFilter()` / `SetFilter(name)`, `IsEnabled()` / `SetEnabled(bool)`, `GetInside()` table of entities, `Contains(entity)` |
| Timer | `Start()`, `Stop()`, `Restart()`, `IsRunning()`, `GetInterval()` / `SetInterval(s)`, `IsRepeating()` / `SetRepeat(bool)`, `GetElapsed()`, `GetRemaining()` |
| Path | `Play()`, `Pause()`, `Restart()`, `IsPlaying()`, `GetSpeed()` / `SetSpeed(v)`, `AddPoint(x, y)`, `ClearPoints()`, `GetPointCount()`, `GetPoint(i)`, `SetPoint(i, x, y)` (1-based) |
| Tilemap | `GetTile(x, y)` returns id, flipX, flipY (id -1 is empty), `SetTile(x, y, id [, flipX, flipY])`, `EraseTile(x, y)`, `FillRect(x1, y1, x2, y2, id)` (inclusive, -1 erases), `FloodFill(x, y, id)` returns the count, `Clear()`, `GetBounds()` returns minX, minY, maxX, maxY or `nil`, `GetTileCount()`, `WorldToTile(wx, wy)`, `TileToWorld(x, y)` (cell center), `GetTileAt(wx, wy)`, `GetCellSize()` (world units), `GetTileSize()` / `SetTileSize(v)`, `IsSolid()` / `SetSolid(bool)`, `IsVisible()` / `SetVisible(bool)`, `GetTint()` / `SetTint(r, g, b [, a])`, `SetTileset(path, tileW, tileH [, margin, spacing])` returns whether it loaded, `GetTilesetSize()` returns columns, rows or `nil` |
| Animation | `SetSheet(path, frameW, frameH, frameCount [, frameDuration = 0.1, loop = true])` returns whether it loaded, `GetSheet()`, `Play()`, `Pause()`, `IsPlaying()`, `Restart()`, `GetFrame()` / `SetFrame(n)` (1-based), `GetFrameCount()`, `GetFrameSize()`, `GetFrameDuration()` / `SetFrameDuration(s)`, `IsLooping()` / `SetLoop(bool)`, `IsFinished()`, `GetSpeed()` / `SetSpeed(v)` |
| Vector | shapes (`SetCircle`, `SetRect`, `SetStar`, `SetHeart`, `SetPolygon`, `SetSvg`, `SetPath/GetPath`), fill/stroke/gradient, caps and joins, `SetTrim`, `SetDash`, `SetMorphTarget`, `SetMorph`, `MorphTo(path, seconds [, curve])`, `GetLength`, `GetPointAt(t)`, `GetBounds`. Details in [ANIMATION.md](ANIMATION.md) |
| CharacterController | `SetInput(x [, y])` (-1..1, added to keyboard input), `Jump()`, `ReleaseJump()` (cuts the jump short), `IsGrounded()`, `JustLanded()`, `JustJumped()`, `HitWall()`, `HitCeiling()`, `GetFacing()` (1 or -1), `GetVelocity()` / `SetVelocity(x, y)`, `GetPlayerInput()` / `SetPlayerInput(bool)`, `GetMode()` / `SetMode("platformer" or "topdown")`, `GetMaxJumps()` / `SetMaxJumps(n)`, and Get/Set for `MoveSpeed`, `Acceleration`, `Deceleration`, `AirControl`, `Gravity`, `MaxFallSpeed`, `JumpSpeed`, `JumpCut`, `CoyoteTime`, `JumpBuffer` |
| Health | `GetCurrent()` / `SetCurrent(v)`, `GetMax()` / `SetMax(v)`, `GetFraction()`, `IsDead()`, `IsInvulnerable()`, `GetInvulnerability()` / `SetInvulnerability(s)`, `Damage(amount [, source])` returns whether it applied, `Heal(amount)` returns the amount healed, `Kill([source])`, `Revive()` |

Tile ids count from 0 at the top-left of the tileset image, row by row. Cell y grows upward like world y. Tilemap and `GetWorldPosition` conversions use the world transform from the last update, so a tilemap moved this frame converts with its old position until the next frame. Animation `playing` and `speed` are saved with the scene.

```lua
local sprite = self.entity:GetSprite()
sprite:SetTint(255, 120, 120)
self.entity:FindByName("Camera"):GetCamera():Shake(0.5)
self.entity:Add("Health"):SetMax(50)
```

`require "name"` loads `assets/scripts/name.lua` and `assets/scripts/name/init.lua`. Native modules are disabled (`package.cpath` is empty).

## New components

| Component | Purpose | Script events |
|---|---|---|
| Trigger Zone | box that reports entities with a Collision component entering, staying in and leaving it (optional name filter) | `OnTriggerEnter/Stay/Exit` |
| Timer | fires after an interval, once or repeating | `OnTimer` |
| Path Follower | moves the entity through a list of points (Once, Loop, Ping-Pong), optional wait and facing | `OnPathPoint`, `OnPathEnd` |
| Health | max/current health, post-hit invulnerability, optional destroy on death | `OnDamaged`, `OnDeath` |
| Character Controller | moves the entity with a box against Collision components (except triggers; optionally only those tagged `Solid Tag`), one-way platforms (tag `One Way Tag`) and solid tilemaps. Platformer mode: acceleration, air control, variable jump height (jump cut), coyote time, jump buffer, multi-jump, gravity. Top Down mode: 8-way movement that slides along walls. Reads A/D/arrows and Space/W/Up unless Player Input is off; scripts steer it with `SetInput`/`Jump` | `OnJump`, `OnLand` |

All five are saved with the scene, appear in the Add Component menu under Logic, and have inspector sections. Trigger zones and the selected entity's path are drawn in the viewport (zones always when the collider overlay is on).

## Samples

### Demo: Coin Dash

A playable demo built entirely from Lua. Build and run the `ScriptDemo` target (next to `GameApp`; in the editor press Play). The C++ side (`Game/ScriptDemo/main.cpp`) only creates a following camera and one entity that runs `assets/scripts/demo/main.lua`; everything else is spawned by the scripts.

- Move with WASD or the arrow keys, dash with Space, collect coins, avoid the red patrolling squares, press R after dying.
- `main.lua`: game manager. Builds the arena, spawns the player, two path-following enemies and the HUD text (a child of the camera), spawns coins from a Timer (`OnTimer`), keeps score and heals every 5 coins. Other scripts reach it with `Scene.Find("Demo"):GetScript()`.
- `player.lua`: `Input.GetAxis`, dash cooldown, Health with invulnerability blink, `OnDamaged` camera shake, `OnDeath`.
- `enemy.lua`: Trigger Zone filtered to "Player", `OnTriggerEnter/Stay`, damage through the Health handle.
- `coin.lua`: pulsing scale, `OnTriggerEnter`, `Destroy()`.
- `util.lua`: shared helpers loaded with `require("demo.util")`.

The demo scripts were run against a mock of this API (method names taken from `LuaBindings.cpp`) but not inside the engine.

### Other samples

`assets/scripts/`: `patrol_enemy.lua` (health, trigger damage, hit flash), `pickup.lua` (bobbing trigger that heals), `spawner.lua` (timer + `Scene.Spawn` + `SetScript`), `moving_platform.lua` (path follower), `fixed_step_demo.lua`.

## Script editor

The editor has a built-in code editor for `.lua` files. Open it with View > Script Editor, by double-clicking a script in the FileSystem panel, or with Edit Script on a Script component (New Script opens the new file too).

- Autocomplete pops up while typing and after `.` or `:`; Ctrl+Space opens it by hand. It knows the Lua globals (`Scene`, `Input`, `Time`, ...), the `Entity` methods, and the methods of component handles. It infers handle types from `GetSprite()`, `Get("Health")`, `Add("Timer")`, `FindByName(...)`, hook arguments like `other`, and local variables assigned from those. Functions and `self.` fields written in the file are offered too.
- The file is syntax-checked a moment after you stop typing. The error line gets a red squiggle and the message shows in the status bar; click the message to jump to the line.
- Ctrl+S saves. Running scripts reload on the next Play because the runtime reloads files when their modification time changes. Files changed on disk reload in the editor when the tab has no unsaved edits.
- Keys: Ctrl+Z / Ctrl+Y undo and redo, Ctrl+F find (F3 / Shift+F3 next and previous), Ctrl+/ toggle comment, Ctrl+D duplicate line, Alt+Up/Down move lines, Tab / Shift+Tab indent, Ctrl+wheel text size.

### Themes

Themes use the VS Code color theme format (`colors` and `tokenColors`, JSON with comments). The theme menu in the editor toolbar lists:

- **Project**: every `.json` in `assets/editor/themes/` (Mossveil Night, Ember, Midnight Ink and Paper are bundled).
- **VS Code**: themes that ship with a local VS Code install.
- **VS Code extensions**: themes from installed extensions in `~/.vscode/extensions` and `~/.vscode-oss/extensions`.

To add a theme, drop its `.json` into `assets/editor/themes/` and press the refresh button next to the theme menu. Themes that `include` a base theme work as long as the included file sits next to them. The chosen theme and text size are saved in `imgui.ini`.

## C++ side

| Piece | Role |
|---|---|
| `LuaRuntime` (`Scripting/LuaRuntime.h`) | owns the `lua_State`; `LuaRuntime::Get()` creates it on first use, `Application` shuts it down. `ExecuteFile`, `ExecuteString`, `Call` (protected call with traceback), `PushScriptClass` |
| `LuaScript` (`Scripting/LuaScript.h`) | a `ScriptableEntity` that hosts one script instance, forwards the hooks and the `ScriptMessage` events |
| `LuaBindings` (`Scripting/LuaBindings.cpp`) | registers the globals (`log`, `Input`, `Key`, `Time`, `Scene`, `Audio`, `Math`), the `Entity` userdata, and one metatable per component binding |
| `LuaHelpers` (`Scripting/LuaHelpers.h`) | shared helpers for binding code: `CheckLiveEntity`, `CheckComponent<T>`, `CheckHandleEntity<T>`, `PushHandle`, `PushRgba`/`ReadRgba`, `RequireScene`, and `ComponentBinding` with `HasOf/AddOf/RemoveOf<T>` |
| `Scripting/Components/Lua<Name>.cpp` | one file per component handle (Sprite, Camera, Light, Velocity, Collision, Shape, Text, Glow, Particles, AudioSource, Parallax, Body, Trigger, Timer, Path, Health, Tilemap, Animation); each defines its methods and a `<Name>Binding()` |
| `Scripting/Components/LuaComponents.h/.cpp` | declares every `<Name>Binding()` and lists them in `ComponentBindings()`, which drives `Get/Has/Add/Remove(name)`, the generated `Get<Name>()` and metatable registration |
| `Scene::SendEvent(entity, ScriptEvent, other, value)` | how systems deliver events; C++ scripts override `OnTriggerEnter`, `OnTimer`, `OnDamaged`, ... on `ScriptableEntity` |

Adding a component binding:

1. Create `Engine/Engine/Scripting/Components/Lua<Name>.cpp`. In an anonymous namespace, define the metatable name (`"SIMP.<Name>"`), the methods as `lua_CFunction`s that start with `CheckComponent<T>(L, Meta)`, and a `luaL_Reg` table ending in `{nullptr, nullptr}`.
2. Define `const ComponentBinding &<Name>Binding()` returning a function-local static `ComponentBinding{"<Name>", Meta, Methods, &HasOf<T>, &AddOf<T> or a custom adder or nullptr, &RemoveOf<T>}`.
3. Declare it in `LuaComponents.h`, add it to the list in `LuaComponents.cpp`, and add the `.cpp` to `Engine/CMakeLists.txt`.

`Get<Name>()`, `Get/Has/Add/Remove("<Name>")` and script editor autocomplete pick it up automatically. Do not keep `std::string` or other objects with destructors alive across `luaL_check*` or `luaL_error` calls, since Lua unwinds with `longjmp`.

## Not yet done

- Script fields set in the inspector are not serialized (only the script path).
- Animation drives the component's single clip; `AnimationController` clips are not exposed (switch clips with `SetSheet`).
- `OnRender` iterates the live script pool, so adding or removing script components from it is not supported.
