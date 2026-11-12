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
| `Input` | `IsKeyDown`, `IsKeyJustPressed`, `IsKeyJustReleased`, `IsMouseDown`, `IsMouseJustPressed`, `IsMouseJustReleased`, `IsMouseInViewport`, `GetMouseWheel`, `GetAxis(negativeKey, positiveKey)` returning -1, 0 or 1. Actions (see "Input actions" below): `IsActionDown(name)`, `IsActionJustPressed(name)`, `IsActionJustReleased(name)`, `GetAxis(name)` (-1..1), `GetVector(name)` (x, y from `<name>X` and `<name>Y`, length at most 1), `HasAction(name)`, `GetActions()`, `GetBindings(name)`, `Rebind(name, binding)`, `AddBinding`, `RemoveBinding`, `ClearBindings(name)`, `ResetBindings([name])`, `DescribeBinding(binding)`, `GetPressedBinding()`, `SetActionValue(name, value)` (touch layers), `IsTouchDevice()`, `SetTouchControls(bool)` (show or hide the on-screen stick and buttons that Android builds draw; they drive `MoveX`, `MoveY`, `Jump` and `Interact`), `IsGamepadConnected()`, `IsGamepadDown(button)`, `GetGamepadAxis(axis)`, `SetGamepadDeadzone(d)` |
| `Key` | constants named after `SIMPK_*` without the prefix: `Key.W`, `Key.SPACE`, `Key.LEFT`, `Key.F1`, `Key.NUM_0`, `Key.MOUSE_LEFT` |
| `Time` | `Now()` scene seconds since Play, `Delta()` last frame time, `FixedDelta()` |
| `Scene` | `Spawn(name [, x, y [, parent]])`, `Clone(entityOrName [, x, y])` (copies an entity and its children, returns the copy or `nil`), `Instantiate(path [, x, y [, parent]])` (creates the entities of a `.prefab` file, returns the root or `nil`; without x, y the prefab keeps its saved position), `Find(name)`, `FindAll(name)`, `SetAmbient(r, g, b)` (0-255), `SetLighting(bool)`, `SetFixedStep(seconds)`, `Load(path [, {fade = seconds, shader = path}])` (switch to another scene file; the swap is queued and runs at the start of the next update, optionally through black or through a transition shader such as `assets://shaders/transition_wipe.glsl` that reads `uProgress` (0 to 1 going out, 1 to 0 coming in), `GameVariables`, saves and the playing music survive), `Reload([{fade = seconds, shader = path}])` (reload the current scene file), `GetPath()`, `IsTransitioning()`, `IsSolidAt(x, y)` and `OverlapsSolid(x1, y1, x2, y2)` (against tilemaps marked solid) |
| `Physics` | `SetGravity(x, y)` / `GetGravity()` (default `0, -980`; y points up), `SetSubsteps(n)` / `GetSubsteps()`, `SetIterations(velocity [, position])`, `SetSleepEnabled(bool)`, `WakeAll()`, `GetBodyCount()`, `GetAwakeCount()`; queries: `Raycast(originX, originY, dirX, dirY [, maxDistance])` returns entity, hitX, hitY, normalX, normalY, distance or `nil`, `QueryPoint(x, y)` returns an entity or `nil`, `QueryBox(minX, minY, maxX, maxY)` and `QueryCircle(x, y, radius)` return tables of entities; joints (both entities need a rigid body): `CreateDistanceJoint(a, b [, ax, ay, bx, by [, length]])`, `CreateRevoluteJoint(a, b, x, y)` (hinge at a world point), `CreateWheelJoint(a, b, x, y [, axisX, axisY])`, `CreateWeldJoint(a, b)` |
| `Window` | `SetFullscreen(bool)`, `IsFullscreen()`, `GetSize()` returns width, height in window points, `SetRenderScale(0.25..1)` / `GetRenderScale()` (draw the game at a fraction of the window size and scale it up: a quality setting for slow GPUs). In the editor `SetFullscreen` toggles fullscreen Play |
| `Audio` | `Play(path [, volume, pitch, loop, bus])` returns a voice id (`.wav` and `.ogg`; bus is `"sfx"` (default), `"music"` or `"ui"`), `Stop(voice)`, `StopAll()`, `IsPlaying(voice)`, `SetVolume(voice, v)`, `FadeTo(voice, volume, seconds [, stopWhenSilent])`, `SetBusVolume(bus, v)` / `GetBusVolume(bus)` (for an options menu), `SetMasterVolume(v)`, `GetMasterVolume()` |
| `Music` | one background track at a time, streamed from disk (OGG) and crossfaded: `Play(path [, volume, fadeIn, fadeOutPrevious, loop = true])` (playing the track that is already playing only updates it), `Stop([fadeOut])`, `Pause()`, `Resume()`, `IsPlaying()`, `IsPaused()`, `GetCurrent()`, `SetVolume(v)` |
| `Game` | shared variables that survive scene changes inside a run: `Get(name [, default])`, `Set(name, number)`, `Add(name [, amount])` returns the new value, `Has`, `Remove`, `Reset()`; save slots on disk: `Save([slot])`, `Load([slot])`, `HasSave([slot])`, `DeleteSave([slot])` (return success). Text components show variables with `{name}`, for example `Coins: {coins}`. Cleared on Play and Stop in the editor |
| `Tween`, `Curves` | Flutter-style animation: `Tween.To/From/FromTo/Value/Along/Spring/Sequence/Parallel/Stagger`, all Flutter curves. See [ANIMATION.md](ANIMATION.md) |
| `Path` | vector paths: `Path.Circle/Rect/Star/Heart/Svg/...`, builders and queries. See [ANIMATION.md](ANIMATION.md) |
| `Draw` | immediate drawing (lines, curves, polygons, paths, text) inside `OnRender`. See [ANIMATION.md](ANIMATION.md) |
| `Math` | `Lerp(a, b, t)`, `Clamp(v, min, max)`, `Sign(v)`, `MoveTowards(current, target, maxDelta)`, `Length(x, y)`, `Distance(x1, y1, x2, y2)`, `Normalize(x, y)`, `Angle(x1, y1, x2, y2)` in degrees |
| `Color` | colors as `{r, g, b, a}` tables (0-255): `New(r, g, b [, a])`, `Hex("#rrggbb" [, alpha])` (also `#rgb`, `#rrggbbaa`, with or without `#`; alpha 0 to 1), `Mix(from, to, t)`, `WithAlpha(color, alpha)` (0 to 1), `Unpack(color [, alpha])` returns r, g, b, a, `Get(color)`, `Define(name, color)`, `Names()`. A color argument can be a table, a `"#hex"` string or a palette name such as `"gold"`. See "Built-in helpers" |
| `Timeline`, `Ease`, `CameraRig`, `Captions`, `Axes` (with `Plot`, `Area`, `Riemann`), `Suggest` | named time segments, easing functions, a keyframed camera, a caption bar, graphing objects and "did you mean" hints, written in Lua and built in. See "Built-in helpers" |
| `Effect` | 38 premade tweens as one-liners: `Effect.PopIn(self.entity)`, `Effect.Shake(e, {time = 0.3})`, `Effect.Play(e, name [, options])`, `Effect.Chain(e, {"popIn", "pulse"})`, `Effect.Stagger(list, "popIn", 0.1)`. See [EFFECTS.md](EFFECTS.md) |
| `Video`, `Backdrop`, `Glyphs` | `Video.Duration/Step/Time/Driven/Recording` for scripts run by `SIMPPreview` and `SIMPRecord` ([MOTION.md](MOTION.md)), `Backdrop.Fill/Grid`, and handwriting-style text through `Glyphs`. See "Built-in helpers" |

`Scene.*` and `Time.*` use the scene of the script that is currently running. `Scene.Spawn` creates an entity with a Transform and a tag; with a parent, the position is local to that parent.

## Built-in helpers

Everything below is available in every script without `require`. The numeric, `Color` and `Draw` parts are C++ (`LuaBindings.cpp`, `LuaColor.cpp`, `LuaDraw.cpp`); `Timeline`, `Ease` and `Draw.Layer` are Lua compiled into the engine (`Scripting/LuaPrelude.cpp`, run when the Lua state is created), so exported games carry them without any extra file.

**Math**: `Math.Clamp01(v)`, `Math.Progress(t, from, to)` (0 to 1 position of `t` between two times, clamped; `from == to` gives a step), `Math.Remap(v, inMin, inMax, outMin, outMax [, clamp])`, `Math.Smoothstep(edge0, edge1, x)`, `Math.Window(t, from, to [, fade = 0.4])` (fades in over `fade` seconds after `from`, out over `fade` seconds before `to`; the usual "visible between two times" value).

**Color**: tables or strings anywhere a color is wanted.

```lua
local cyan = Color.Hex("#58ceff")
local glow = Color.WithAlpha(cyan, 0.2)
local mid = Color.Mix(cyan, "#ffce60", 0.5)
Draw.Circle(0, 0, 40, mid)
Draw.Text("hello", 0, 0, 32, "#ffffff", "center")
Draw.Rect(0, 0, 80, 50, cyan, 25)
```

`Draw.*` accepts `r, g, b [, a]` as before, a color table, a `"#hex"` string or a **palette name** in the color position; arguments after the color (rotation, `closed`, alignment) simply follow it.

```lua
Draw.Circle(0, 0, 40, "gold")
Draw.Rect(0, 0, 80, 50, Color.WithAlpha("coral", 0.5))
Color.Define("brand", "#7c5cff")        -- your own names work everywhere a color does
Draw.Ring(0, 0, 60, 4, "brand")
```

Built-in names: `white black bg panel ink muted faint cyan teal coral gold violet pink red orange green blue` (`Color.Names()` lists them all, including yours). An unknown name raises an error that lists the valid ones.

**Timeline**: named segments laid end to end, so a video or cutscene is described once and every part asks "how far along am I?".

```lua
local tl = Timeline.New({{"intro", 4.6}, {"pan", 2.4}, {"curve", 5.4}})
tl:Add("outro", 3)
local p = tl:Progress(t, "curve")        -- 0 before it starts, 1 after it ends
if tl:Active(t, "pan") then ... end
local from, to = tl:Span("curve")
local name, progress = tl:Current(t)     -- the segment that contains t
local total = tl:Total()
```

`Add` returns the timeline, so calls chain. A duplicate or unknown name raises an error that points at the calling line (and suggests the closest name: `unknown segment 'curv' (did you mean 'curve'?)`). Two more helpers: `tl:Window(t, "curve" [, fade])` is `Math.Window` over a segment (1 while it plays, fading at both ends), and `tl:Eased(t, "curve" [, curve])` is the segment's progress through an `Ease` name or function.

**Suggest and Math.Integrate**: `Suggest.Closest(name, list)` returns the closest list entry to a misspelled name (or `nil` when nothing is close), `Suggest.Hint(name, list)` returns the ` (did you mean 'x'?)` text for error messages (`Effect`, `Ease` and `Timeline` use it). `Math.Integrate(fn, from, to [, steps = 200])` is Simpson's rule, handy for the "true value" next to a visual approximation.

**Video, Backdrop and Glyphs**:

```lua
Video.Duration = tl:Total()                  -- seconds; read by SIMPRecord and SIMPPreview
self.t = Video.Step(self.t, dt)              -- tool clock when run by those tools, t + dt otherwise

Backdrop.Fill(cx, cy, zoom, "bg")            -- a colour behind everything
Backdrop.Grid(cx, cy, zoom, {step = 90})     -- faint grid with stronger lines every 5 steps (options: major, color, width, minorAlpha, majorAlpha, halfWidth, halfHeight)

local title = Glyphs.New("Integration", 160)                     -- text as glyph outlines (centered)
title:Draw(0, 200, Math.Progress(t, 0, 2))                       -- written on like handwriting, then filled in
title:Draw(0, 200, p, {color = "gold", stagger = 0.8, fill = false, width = 4})
local w, h = title:Size()                                        -- also :Bounds(), :Count()
```

`Glyphs:Draw(x, y, progress [, options])` draws every glyph's outline growing along its path, glyph after glyph (`stagger` 0 draws all at once, 1 one after the other; default 0.6), then fades the fill in at the end of each glyph (`fillStart`, default 0.6; `fill = false` for outline only). `(x, y)` is the centre of the text unless `middle = false`. Options: `color`, `stroke` (a color or `false`), `width`, `scale`. `Path.TextGlyphs(text, size [, align [, fontPath]])` is what it is built on: a list of `{path, index, codepoint, line, minX, minY, maxX, maxY}` for each visible glyph. Greek letters and math symbols work when the font has them. `Draw.Path` accepts palette names and `"#hex"` strings for `fill` and `stroke`.

`Effect.Chain(target, list [, options])` plays effects one after another and returns one handle: `Effect.Chain(e, {"popIn", {"shake", {time = 0.2}}, "fadeOut"})` (an item is a name or `{name, options}`; loops cannot be chained, and put entrances first because an entrance's start state is applied when its turn comes). `Effect.Stagger(targets, name [, each = 0.1 [, options]])` starts the same effect on each target `each` seconds after the previous one and returns the list of handles.

**Graphs**: `Axes` places a coordinate system on screen and everything drawn through it uses data coordinates.

```lua
local ax = Axes.New{origin = {-675, -330}, scale = {270, 180},   -- screen position of (0, 0), pixels per unit
                    x = {-0.4, 5.55, 1}, y = {-0.11, 3.95, 1}}      -- {min, max, tick step}
local plot  = ax:Plot(f, 0, 5)              -- the curve of f from x = 0 to 5
local area  = ax:Area(f, 0, 5)              -- the region under it
local sums  = ax:Riemann(f, 0, 5)           -- rectangles under it

function S:OnRender()
    ax:Draw(Math.Progress(t, 0, 2))         -- 0..1 grows the axes, then fades in ticks and labels
    area:Draw(5 * progress, 0.26)           -- fills up to x = 5 * progress, with an alpha
    sums:Draw(16)                           -- 16 slices (outlined up to 32, gaps appear from 64)
    plot:Draw(5 * progress)                 -- the curve, drawn on up to x (glow, core and highlight strokes)
    ax:Tracer(f, 2.5, {time = t})           -- dashed guides, a pulsing dot and the x and f(x) readout
end
```

`ax:P(x, y)` converts data to screen coordinates. Axis tick marks default to multiples of the step (x includes 0, y does not); pass `xticks = {0, 1, 4}` or `yticks = {}` to choose. Options of `Axes.New`: `color`, `tickColor`, `width`, `labels = {x = "x", y = "y"}`, `fontSize`. Every `Draw` method takes the camera zoom as an optional last argument (`ax:Draw(p, zoom)`, `plot:Draw(xr, zoom)`) so line widths follow a zoomed camera. A plot takes `{samples, color, width}`; an area `{samples, color}`.

`Riemann` options: `{mode = "left" | "right" | "mid", from = color, to = color, outline = bool}` for the object, and `Draw(n, {alpha, zoom, fillAlpha, outline, window = {xlo, xhi}, grow = fn(i), from = previousN, u = 0..1})`. `from` and `u` animate a refinement: with `from = 4` and `u` going 0 to 1, every slice of an `n = 8` sum starts at its parent's height and settles to its own. `sums:Sum(n)` is the numeric sum, `sums:Rects(n, opts)` returns the slice geometry (`x0, x1, top, base, h, index, t`) without drawing. `plot:Points(xr)` and `area:Polygon(xr)` return the screen-space points the same way, so everything can be drawn your own way.

**Ease**: `Ease.inOutCubic(t)`, `Ease.outBack(t)`, `Ease.linear(t)`... the engine's Flutter curves as plain functions. A name is any `Curves` name, with or without the `ease` prefix (`Ease.inOutCubic` is `Curves.easeInOutCubic`). The values are the engine's curves, not the polynomial easings found on easings.net.

**Draw** (inside `OnRender` unless noted):

- `Draw.Dashed(x1, y1, x2, y2, width, dash, gap, color [, progress])` draws a dashed line; `progress` (0 to 1) draws only that fraction of it, which animates a line growing.
- `Draw.Arrow(x1, y1, x2, y2, width, color [, head])` draws a line with an open arrowhead at the end (`head` is the head length, default `4.2 * width`).
- `Draw.MeasureText(text, size [, fontPath])` returns width, height with the same font and layout as `Draw.Text`; usable anywhere, not only in `OnRender`. Multi-line text measures every line.
- `Draw.Label(text, x, centerY, size, color [, align])` is `Draw.Text` with `y` at the vertical middle of the text instead of the top of its line box, which is usually what you want when placing text next to a shape.
- `Draw.Text` and `Draw.Label` also take an options table instead of `size, color, align`: `Draw.Text("Score 12", x, y, {size = 36, color = "gold", align = "center", middle = true})`. `middle` puts `y` at the vertical centre (the default for `Label`); every key is optional.
- `Draw.Layer(z, fn)` runs `fn` with the draw z set to `z` and restores the previous z afterwards, even if `fn` raises an error.

**CameraRig**: a camera that moves between named poses (`{x, y, zoom}`) along a `Timeline`.

```lua
local tl = Timeline.New({{"intro", 4}, {"pan", 2.4}, {"zoom", 3}})
local rig = CameraRig.New(tl, {0, 1520, 1.38})          -- pose before the first segment
rig:Move("intro", {0, 1520, 1.14}, "outCubic")          -- pose to reach by the end of the segment
rig:Move("pan", {0, -75, 1.02}, "inOutCubic", {bump = 0.3})   -- bump adds a zoom breath mid-move
rig:Track("zoom", function(u) return -100 * u, 0, 1 - 0.5 * u end)  -- your own function of 0..1
rig:Sway()                                              -- optional slow handheld drift

function S:OnUpdate(dt) self.t = self.t + dt; self.x, self.y, self.zoom = rig:Apply(self.t, self.camera) end
```

`rig:At(t)` returns x, y, zoom without touching a camera; `rig:Apply(t, cameraEntity)` also calls `SetPosition` and `SetZoom` and returns the same three numbers. Before the first segment the camera sits at the start pose, between segments it holds the last pose. Curves are `Ease` names or functions.

**Captions**: a caption bar and progress bar for explainers and cutscenes.

```lua
local caps = Captions.New(tl, {footer = "HOW INTEGRATION WORKS"})
caps:Add("axes", "pull", "STEP 1  -  A NON-LINEAR CURVE", "Its height changes at every point along x")
-- in OnRender, with the camera's pose so the bar stays on screen:
caps:Draw(self.t, self.x, self.y, self.zoom)
```

A caption is shown from the start of its first segment to the end of its second (a quarter second in from each side, fading over 0.45 s). `caps:Visible(t)` returns the active captions with their alpha without drawing, so you can draw them yourself. Options (all optional): `background`, `title`, `text`, `muted`, `bar` (colors), `titleSize`, `textSize`, `fade`, `inset`, `progress` (false hides the progress bar), `footer`.

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

`entity:Get("Sprite")` returns a handle or `nil`. Every component also has a shortcut: `GetSprite()`, `GetCamera()`, `GetLight()`, `GetVelocity()`, `GetCollision()`, `GetShape()`, `GetText()`, `GetGlow()`, `GetParticles()`, `GetAudio()`, `GetParallax()`, `GetBody()`, `GetTrigger()`, `GetTimer()`, `GetPath()`, `GetHealth()`, `GetTilemap()`, `GetAnimation()`, `GetVector()`, `GetCharacterController()`, `GetDamage()`, `GetOscillator()`, `GetSpawner()`, `GetFollow()`, `GetPickup()`, `GetCheckpoint()`, `GetRespawn()`, `GetMusic()`, `GetPickable()`, `GetButton()`, `GetTeleporter()`, `GetCameraZone()`, `GetWorldBounds()`, `GetCanvas()`, `GetUIRect()`, `GetUIPanel()`, `GetUILabel()`, `GetUIImage()`, `GetUIButton()`, `GetUILayout()`, `GetShaderCanvas()`. `Add(name)` adds the component if missing and returns the handle (`Add("Body")` gives an empty body: call one of the `Create...` methods). Colors are 0-255 and clamped.

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
| Body | create: `CreateBox(w, h [, mass])`, `CreateCircle(r [, mass])`, `CreateCapsule(halfLength, r [, mass])`, `CreatePolygon(sides, r [, mass])`, `CreatePoints({{x, y}, ...} [, cornerRadius, mass])` (convex hull), `Destroy()`. Info: `HasShape()`, `GetShape()` (`"box"`, `"circle"`, `"capsule"`, `"polygon"`, `"compound"`), `GetSize()`, `IsStatic()`, `IsAwake()`. Motion: `GetVelocity()` / `SetVelocity(x, y)`, `AddImpulse(x, y)`, `AddImpulseAt(ix, iy, px, py)`, `AddForce(x, y)` and `AddTorque(t)` (call every frame), `GetSpin()` / `SetSpin(rad/s)`, `GetAngle()` (degrees), `Teleport(x, y [, degrees])`, `Stop()`. Material: `GetMass()` / `SetMass(m)` (0 or less = static), `GetBounce()` / `SetBounce(v)`, `GetFriction()` / `SetFriction(static [, moving])`, `SetDamping(linear [, angular])` (fraction of speed kept per second). Behaviour: `SetSensor(bool)` / `IsSensor()`, `SetOneWay(bool [, nx, ny])` / `IsOneWay()`, `SetLocked(rotation [, position])` / `IsLocked()`, `SetIgnoreGravity(bool)`, `SetCanSleep(bool)`, `Wake()`, `Sleep()`. Filtering: `SetLayer(0..15)` / `GetLayer()`, `SetMask(bits)` / `GetMask()`, `SetCollidesWith(layer, ...)`, `SetGroup(n)` / `GetGroup()` |
| Joint | from `Physics.Create...Joint`: `IsValid()`, `Destroy()`, `GetType()`, `SetCollideConnected(bool)`; distance: `SetLength(l)`, `GetLength()`, `SetSoftness(compliance)`, `SetLimits(min, max)`; revolute: `SetLimits(minDegrees, maxDegrees)`, `SetMotor(speed, maxTorque)`, `DisableMotor()`, `GetAngle()`; wheel: `SetSpring(hertz [, damping])`, `SetMotor(speed, maxTorque)`, `SetLimits(min, max)`, `GetTranslation()`; weld has no settings |
| Trigger | `GetSize()` / `SetSize(w, h)`, `GetOffset()` / `SetOffset(x, y)`, `GetFilter()` / `SetFilter(name)`, `IsEnabled()` / `SetEnabled(bool)`, `GetInside()` table of entities, `Contains(entity)` |
| Timer | `Start()`, `Stop()`, `Restart()`, `IsRunning()`, `GetInterval()` / `SetInterval(s)`, `IsRepeating()` / `SetRepeat(bool)`, `GetElapsed()`, `GetRemaining()` |
| Path | `Play()`, `Pause()`, `Restart()`, `IsPlaying()`, `GetSpeed()` / `SetSpeed(v)`, `AddPoint(x, y)`, `ClearPoints()`, `GetPointCount()`, `GetPoint(i)`, `SetPoint(i, x, y)` (1-based) |
| Tilemap | `GetTile(x, y)` returns id, flipX, flipY (id -1 is empty), `SetTile(x, y, id [, flipX, flipY])`, `EraseTile(x, y)`, `FillRect(x1, y1, x2, y2, id)` (inclusive, -1 erases), `FloodFill(x, y, id)` returns the count, `Clear()`, `GetBounds()` returns minX, minY, maxX, maxY or `nil`, `GetTileCount()`, `WorldToTile(wx, wy)`, `TileToWorld(x, y)` (cell center), `GetTileAt(wx, wy)`, `GetCellSize()` (world units), `GetTileSize()` / `SetTileSize(v)`, `IsSolid()` / `SetSolid(bool)`, `IsVisible()` / `SetVisible(bool)`, `GetTint()` / `SetTint(r, g, b [, a])`, `SetTileset(path, tileW, tileH [, margin, spacing])` returns whether it loaded, `GetTilesetSize()` returns columns, rows or `nil` |
| Animation | `SetSheet(path, frameW, frameH, frameCount [, frameDuration = 0.1, loop = true])` returns whether it loaded, `GetSheet()`, `Play()`, `Pause()`, `IsPlaying()`, `Restart()`, `GetFrame()` / `SetFrame(n)` (1-based), `GetFrameCount()`, `GetFrameSize()`, `GetFrameDuration()` / `SetFrameDuration(s)`, `IsLooping()` / `SetLoop(bool)`, `IsFinished()`, `GetSpeed()` / `SetSpeed(v)` |
| Vector | shapes (`SetCircle`, `SetRect`, `SetStar`, `SetHeart`, `SetPolygon`, `SetSvg`, `SetPath/GetPath`), fill/stroke/gradient, caps and joins, `SetTrim`, `SetDash`, `SetMorphTarget`, `SetMorph`, `MorphTo(path, seconds [, curve])`, `GetLength`, `GetPointAt(t)`, `GetBounds`. Details in [ANIMATION.md](ANIMATION.md) |
| CharacterController | `SetInput(x [, y])` (-1..1, added to keyboard input), `Jump()`, `ReleaseJump()` (cuts the jump short), `IsGrounded()`, `JustLanded()`, `JustJumped()`, `HitWall()`, `HitCeiling()`, `GetFacing()` (1 or -1), `GetVelocity()` / `SetVelocity(x, y)`, `GetPlayerInput()` / `SetPlayerInput(bool)`, `GetMode()` / `SetMode("platformer" or "topdown")`, `GetMaxJumps()` / `SetMaxJumps(n)`, and Get/Set for `MoveSpeed`, `Acceleration`, `Deceleration`, `AirControl`, `Gravity`, `MaxFallSpeed`, `JumpSpeed`, `JumpCut`, `CoyoteTime`, `JumpBuffer` |
| Damage | `GetAmount()` / `SetAmount(v)` (negative heals), `GetInterval()` / `SetInterval(s)`, `GetKnockback()` / `SetKnockback(v)`, `GetFilter()` / `SetFilter(name)`, `IsEnabled()` / `SetEnabled(bool)`, `SetDestroyOnHit(bool)`, `Reset()` (forget cooldowns so targets inside are hit again) |
| Oscillator | `Play()`, `Stop()`, `IsPlaying()`, `GetMove()` / `SetMove(x, y)`, `GetFrequency()` / `SetFrequency(hz)`, `SetSwing(degrees)`, `SetSpin(degreesPerSecond)`, `SetPulse(fraction)` |
| Spawner | `Start()`, `Stop()`, `IsRunning()`, `Spawn([count])` (spawns on the next update, limits still apply), `GetAlive()`, `GetSpawned()`, `GetInterval()` / `SetInterval(s)`, `SetMaxAlive(n)`, `SetVelocity(x, y)`, `GetPrototype()` / `SetPrototype(name)` |
| Button | `SetEnabled(bool)` / `IsEnabled()`, `IsHovered()`, `IsPressed()`, `SetClickSound(path)`, `SetHoverSound(path)`, `SetHoverScale(v)`, `SetPressScale(v)` |
| Canvas | `SetVisible(bool)` / `IsVisible()`, `GetSize()` (canvas units after scaling), `GetScale()`, `SetReferenceSize(w, h)`, `SetSortOrder(n)` |
| UIRect | `SetPosition(x, y)` / `GetPosition()`, `SetSize(w, h)` / `GetSize()`, `SetAnchors(minX, minY [, maxX, maxY])`, `SetPivot(x, y)`, `SetMargin(left, bottom, right, top)`, `SetVisible(bool)` / `IsVisible()`, `SetBlocksInput(bool)`, `GetRect()` (x, y, w, h on the canvas, valid after the first update) |
| UIPanel | `SetColor(r, g, b [, a])` / `GetColor()`, `SetCornerRadius(r)`, `SetBorder(width [, r, g, b, a])` |
| UILabel | `SetText(text)` / `GetText()` (`{name}` is replaced by game variables), `SetFontSize(v)` / `GetFontSize()`, `SetColor(r, g, b [, a])` / `GetColor()`, `SetAlign(horizontal [, vertical])` (0 left/top, 1 center, 2 right/bottom), `SetShadow(bool)` |
| UIImage | `SetTexture(path)`, `SetTint(r, g, b [, a])` / `GetTint()`, `SetPreserveAspect(bool)` |
| UIButton | `SetEnabled(bool)` / `IsEnabled()`, `IsHovered()`, `IsPressed()`, `IsFocused()`, `SetClickSound(path)`, `SetHoverSound(path)`, `SetHoverScale(v)`, `SetPressScale(v)` |
| UILayout | `SetSpacing(v)`, `SetPadding(left [, bottom, right, top])`, `SetDirection("row" | "column")`, `SetEnabled(bool)` |
| ShaderCanvas | `SetShader(path)` / `GetShader()`, `SetEnabled(bool)` / `IsEnabled()`, `SetProgress(v)` / `GetProgress()` (the `uProgress` uniform), `GetError()` (compile message, empty when fine), `SetFloat(name, v)` / `GetFloat(name)`, `SetVec2(name, x, y)`, `SetVec3(name, x, y, z)`, `SetVec4(name, x, y, z, w)`, `SetColor(name, r, g, b [, a])` (0-255), `SetInt(name, v)`, `SetBool(name, b)`, `SetTexture(name, path)`, `HasUniform(name)`. `Tween.To(entity, {uAmount = 1}, 0.5)` and `{shaderProgress = 1}` work on a Shader Canvas entity (vec and color uniforms take tables of 0..1 numbers) |
| Teleporter | `SetTarget(name)` / `GetTarget()`, `SetOffset(x, y)` / `GetOffset()`, `SetEnabled(bool)` / `IsEnabled()`, `SetKeepVelocity(bool)` |
| CameraZone | `SetZoom(v)`, `SetSpeed(v)`, `SetEnabled(bool)` / `IsEnabled()`, `SetUseBounds(bool)`, `SetBoundsMin(x, y)`, `SetBoundsMax(x, y)` |
| WorldBounds | `SetMin(x, y)` / `GetMin()`, `SetMax(x, y)` / `GetMax()`, `SetMode("clamp" or "wrap" or "bounce" or "destroy")`, `SetMargin(v)`, `SetEnabled(bool)` / `IsEnabled()` |
| Pickable | `IsHeld()`, `Drop()`, `IsEnabled()` / `SetEnabled(bool)`, `SetThrowStrength(v)`, `SetSmoothing(v)` |
| Follow | `SetTarget(name)` / `GetTarget()`, `SetOffset(x, y)` / `GetOffset()`, `SetSmoothing(v)`, `SetStopDistance(v)`, `SetEnabled(bool)` / `IsEnabled()` |
| Pickup | `SetKey(name)` / `GetKey()`, `SetAmount(v)` / `GetAmount()`, `SetSound(path)`, `SetEnabled(bool)` / `IsEnabled()`, `SetDestroyOnPickup(bool)` |
| Checkpoint | `SetOffset(x, y)` / `GetOffset()`, `SetSound(path)`, `SetEnabled(bool)` / `IsEnabled()` |
| Respawn | `GetPoint()` / `SetPoint(x, y)`, `Respawn()` (brings the entity back now), `SetDelay(s)`, `SetKillY(y)`, `SetUseKillY(bool)` |
| Music | `Play()`, `Next()`, `Stop()`, `AddTrack(path)`, `ClearTracks()`, `GetTrackCount()`, `SetVolume(v)`, `SetShuffle(bool)` |
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
| Rigid Body | AccelEngine body: circle, box, capsule, convex polygon or compound, with mass, bounce, friction, damping, locks, sleeping, collision layers/mask/group, sensor and one-way platform. Gravity and solver settings are on the scene. The Transform is the pose: static and locked bodies follow it, dynamic bodies drive it. Solid contacts call `OnCollision(other)`; a sensor body calls `OnTriggerEnter/Stay/Exit(other)` | `OnCollision`, `OnTriggerEnter/Stay/Exit` |
| Trigger Zone | box that reports entities with a Collision component or a Character Controller entering, staying in and leaving it (optional name filter) | `OnTriggerEnter/Stay/Exit` |
| Timer | fires after an interval, once or repeating | `OnTimer` |
| Path Follower | moves the entity through a list of points (Once, Loop, Ping-Pong), optional wait and facing | `OnPathPoint`, `OnPathEnd` |
| Health | max/current health, post-hit invulnerability, optional destroy on death | `OnDamaged`, `OnDeath` |
| Character Controller | moves the entity with a box against Collision components (except triggers; optionally only those tagged `Solid Tag`), one-way platforms (tag `One Way Tag`) and solid tilemaps. Platformer mode: acceleration, air control, variable jump height (jump cut), coyote time, jump buffer, multi-jump, gravity. Top Down mode: 8-way movement that slides along walls. Reads A/D/arrows and Space/W/Up unless Player Input is off; scripts steer it with `SetInput`/`Jump` | `OnJump`, `OnLand` |

| Damage | needs a Trigger Zone on the same entity (added with it). Takes `amount` from the Health of every entity inside (negative heals), again every `interval` seconds while it stays (0 = once per entry), optional name filter, knockback for Character Controllers, optional destroy on hit for projectiles and pickups | the target gets `OnDamaged`/`OnDeath` |
| Oscillator | sine motion around where the entity is: `move` offset, `swing` rotation, constant `spin`, scale `pulse`, `frequency` and `phase`. Adds deltas, so it combines with other movement | |
| Button | clickable entity: hover and press scale feedback, optional hover and click sounds (UI bus). A click is a press and release on it with the chosen mouse button. Hit area as for Pickable. Buttons are world entities, so put a HUD button under the Camera entity so it moves with the view | `OnClick`, `OnHoverEnter`, `OnHoverExit` |
| Canvas | screen-space UI root. Children with a UI Rect are laid out in canvas units (y up), drawn after the world, lighting and bloom, and are not affected by the camera, zoom or shake. `scale_mode` Scale With Screen keeps the layout at the reference resolution (`match` 0 follows the width, 1 the height); Constant Pixel keeps one unit per pixel. `safe_area` insets the canvas. In the Viewport panel a canvas is drawn at its reference size beside the world origin; the Game panel shows the real thing | |
| UI Rect | position of a UI node inside its parent: anchors (0..1 of the parent), pivot, position and size; when the anchors differ on an axis the node stretches and uses the margins | |
| UI Panel / UI Label / UI Image | drawing nodes: rounded rectangle with a border, text (supports `{variable}`), and a texture | |
| UI Button | hover, press and focus feedback; `OnClick`, `OnHoverEnter`, `OnHoverExit` go to the entity. `UIFocus` (Tab, pad Back) shows the focus ring, then the arrow keys, WASD, d-pad or stick (`MoveX`/`MoveY` actions) move it and `UIConfirm` (Enter, pad A) clicks the focused button; moving the mouse hides the ring again. Drawn panels, images and buttons stop the mouse from reaching world Buttons and Pickables | `OnClick`, `OnHoverEnter`, `OnHoverExit` |
| UI Layout | arranges its UI children in a row or column with spacing, padding, justify and alignment | |
| Shader Canvas | runs a fragment shader (`.glsl` under `assets://shaders/`) over the picture. Scope Scene: post-processes everything drawn so far (before the UI), several canvases run in `order`. Scope Children / Z Range: the canvas children (or entities with a Z Index in the range) are drawn on their own, then composited through the shader, so only they are affected (they draw above the rest of the world, below the lighting). The shader gets `in vec2 uv`, `uniform sampler2D uTexture` (premultiplied alpha), `uTime`, `uResolution`, `uMouse` (0..1, y up), `uProgress`, `uRegion` (x, y, width, height of the area in 0..1 uv, `0,0,1,1` when unlimited) and writes `out vec4 color`; do not declare them. Turn on Limit To Area to restrict the effect to a rectangle centred on the entity (Area Size times the entity Scale), so you can move and scale it in the Viewport. Extra `uniform float/vec2/vec3/vec4/int/bool/sampler2D` lines are picked up automatically and appear in the inspector (comment `// color`, `// range(0,1)`, `// default(0.5)` as hints). A shader that fails to compile logs the message with the file name and line, shows the scene unshaded and never crashes; saving the file reloads it | |
| Teleporter | needs a Trigger Zone (added with it, filtered to `Player`). Moves whatever enters to the entity named `Destination` plus an offset, optionally keeping or stopping its velocity. Whatever arrives is not sent back until it leaves the zone, so two Teleporters pointing at each other make a two-way portal | |
| Camera Zone | needs a Trigger Zone (added with it, filtered to `Player`). While the player is inside, the main camera eases to the zone's zoom and, optionally, is limited to its bounds; leaving restores the previous values. For rooms, boss arenas, tight corridors | |
| World Bounds | keeps the entity inside a rectangle: Clamp, Wrap (reappear on the opposite side), Bounce (reflects Velocity, Rigid Body and Character Controller motion) or Destroy (removed once outside). Margin accounts for the entity's size. The rectangle is outlined in the viewport when selected | |
| Pickable | grab and drag the entity with the mouse in the running game: press and hold on it (left, right or middle button), move, release. The top-most pickable under the cursor wins. Optional smoothing, throw on release (Rigid Body, Character Controller and Velocity all pick up the mouse speed), lift scale and bring-to-front while held, grab padding. Hit area comes from the Sprite or Shape size (rotation and scale respected), else the Collision box. Works in the editor's Game panel and exported games | `OnGrab`, `OnDrop` |
| Follow | eases the entity toward another entity (by name) with an offset, per-axis switches and a stop distance; for companions, enemies that chase, floating labels | |
| Pickup | needs a Trigger Zone (added with it, filtered to `Player`). When touched it adds `amount` to the game variable `key`, plays a sound and removes itself (or stays and re-triggers after you leave). Coins, keys, ammo, any counter | |
| Checkpoint | needs a Trigger Zone. Entities with a Respawn component that touch it will come back at it (plus `offset`) | |
| Respawn | when its Health dies (or it falls below `Fall Below Y`) it waits `delay`, returns to where it started or the last Checkpoint, and gets its health back. Don't combine with Health's Destroy On Death | |
| Music | playlist (OGG or WAV) with volume, fade in/out (tracks crossfade), shuffle and loop; plays on start, or when something enters its own Trigger Zone and optionally stops on exit, for area themes and boss rooms. Preview button in the inspector | |
| Spawner | copies the entity named `prototype` (with its children and scripts), or the prefab when `prototype` is a `.prefab` path such as `assets://prefabs/Bat.prefab` every `interval`: `per spawn`, `max alive`, `total limit`, offset, random spread, starting velocity and lifetime for the copies. `Hide Prototype` removes the original from the running game so it can be edited in place as a template | |

Audio Source also has a Bus (Sfx, Music or UI) so an options menu can set each volume with `Audio.SetBusVolume`.

All of these are saved with the scene, appear in the Add Component menu under Logic, and have inspector sections. Trigger zones and the selected entity's path are drawn in the viewport (zones always when the collider overlay is on).

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
- **Argument-aware completion.** The editor knows where you are inside a call. In `Tween.To(entity, {` it offers the animatable properties (`trimEnd`, `alpha`, `fillColor`, ...) with their value shape, and inserts `name = `; properties already in the table are left out. In the options table (`Tween.To(e, {...}, 1, "easeOut", {`) it offers `delay`, `loop`, `yoyo`, `speed`, `onComplete`, ... (spring and `Tween.Along` have their own sets). At the curve argument, quoted or not, it lists every curve; accepting one without quotes adds them. The same goes for `:MorphTo(...)`, `Curves.Interval(...)`, `Tween.Kill(entity, "prop")` and `Audio.Play(..., "music")`.
- **Curve preview.** Whenever a curve is highlighted in the popup, or the cursor or mouse rests on a curve name (`"easeOutBack"`, `Curves.bounceOut`), a small graph with the curve's name and a moving dot appears. Overshoot shows beyond the box lines.
- **Signature help for every built-in function.** Inside any call to an engine function, a component handle method or the Lua standard library, a hint above the cursor shows the parameters with the current one underlined; optional parameters end in `?`. Method calls use the receiver's type when it can be worked out (`self.entity:GetSprite():SetTint(` shows `Sprite:SetTint(r, g, b, a?)`), otherwise the hint lists the types that have that method. The data is generated from the docs and the binding code by `Editor/ScriptEditor/tools/gen_signatures.py` into `LuaSignatureData.inc`; run it after adding or changing a Lua function (a test fails when a registered function has no signature).
- **Go to definition.** Ctrl+click (or F12 on the cursor) jumps to where a local, parameter, function, `self:method`, table member or `require` module is defined, across files (`Util.double` opens `util.lua` at `M.double`). Hold Ctrl to see what is clickable (underlined); over engine functions the hint shows their signature instead. Alt+Left goes back through your jumps.
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
| `LuaBindings` (`Scripting/LuaBindings.cpp`) | registers the globals (`log`, `Input`, `Key`, `Time`, `Scene`, `Audio`, `Math`, `Color`, `Effect`), then runs the built-in Lua helpers (`LuaPrelude.cpp`: `Timeline`, `Ease`, `Draw.Layer`, `CameraRig`, `Captions`, the `Effect.<Name>` one-liners), the `Entity` userdata, and one metatable per component binding |
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

## Input actions

Actions give a gameplay meaning to physical inputs, so the same script works with a keyboard and a gamepad and players can rebind. Bindings are strings: `Key.Space`, `Key.A`, `Mouse.Left`, `Pad.South` (`South/East/West/North/Back/Start/LeftStick/RightStick/LeftShoulder/RightShoulder/DPadUp/DPadDown/DPadLeft/DPadRight`), `Axis.LeftX` (`LeftX/LeftY/RightX/RightY/LeftTrigger/RightTrigger`). A leading `-` negates the contribution (`-Key.A` on `MoveX`).

```lua
function Player:OnUpdate(dt)
    local mx, my = Input.GetVector("Move")
    if Input.IsActionJustPressed("Jump") then ... end
end

Input.Rebind("Jump", "Pad.North")
```

- An action's value is the sum of its pressed keys and buttons (clamped to -1..1) or the strongest stick/trigger, whichever is larger. It counts as down at 0.5 or more.
- `Rebind(name, binding)` replaces the player's bindings of the same device group (keyboard and mouse, or gamepad) and adds the new one, so a keyboard rebind keeps the pad bindings. In the standalone player the result is saved in the preference folder (`input-rebinds.yaml`) and loaded at the next start. In the editor rebinds last until Stop.
- `GetPressedBinding()` returns the first key, button or stick pushed this frame, for a "press a key" rebind screen.
- The project defaults live in `input.yaml` next to `project.simp` (edit them in Project Settings, Input). Without one a project has `MoveX`, `MoveY`, `Jump`, `Interact`, `Pause`, `UIFocus`, `UIConfirm` and `UICancel`.
- The character controller reads `MoveX`, `MoveY` and `Jump`.

