# SIMPEngine API

The short, complete reference for writing games with SIMPEngine in Lua: one page, written to be read by people and coding agents. The long guides (SCRIPTING, ANIMATION, EFFECTS, PHYSICS, SHIPPING) explain the same things with more words.

## Project

| Thing | Where |
|---|---|
| Project file | `project.simp` (YAML: `name`, `start_scene`, `window`, `display`) |
| Game files | `<project>/assets`, addressed as `assets://...` |
| Scenes | `assets://scenes/*.yaml`, entities with components |
| Scripts | `assets://scripts/*.lua`, attached with a `Script: {lua: assets://scripts/x.lua}` component or `entity:SetScript(path)` |
| Input map | `<project>/input.yaml` (actions `MoveX MoveY Jump Interact Pause UIFocus UIConfirm UICancel` exist by default) |
| Run | `SIMPEditor <project>`, `SIMPPlayer <project>`, phone: `Android/build.sh <project>` |
| Check (no screen needed) | `SIMPPlayer <project> --check 3 --screenshot shot.png`: runs 3 s hidden, prints errors and warnings with file:line, `PASS` (exit 0) or `FAIL` (exit 2). Extra flags: `--shot 1.5 f.png` (frame at a time, repeatable), `--click x,y@t` (left click at window point x,y), `--size 480x1040` (window size). Check runs write saves to a throwaway folder (`--keep-saves` or `SIMP_SAVE_DIR=dir` to choose) |

Coordinates: world units, y up, the camera shows 1920x1080 at zoom 1 (Project Settings > Display). Angles are degrees. Every color argument accepts `r, g, b, a?` (0-255), `"#rrggbb"`, `{r, g, b, a?}` or a palette name (`Color.Names()`). Mistyped component, tween property and spawn names fail with a "did you mean" hint.

## Scene file

```yaml
Scene: My Game                 # the scene name, text (a map here gives "bad conversion")
Version: 1
Settings: {background: [0.08, 0.09, 0.13, 1]}
Entities:
  - ID: 1
    Tag: Camera
    Transform: {position: [0, 0], rotation: 0, zIndex: 0, scale: [1, 1]}
    Camera: {primary: true, zoom: 1}
  - ID: 2
    Tag: Game
    Transform: {position: [0, 0], rotation: 0, zIndex: 0, scale: [1, 1]}
    Script: {lua: assets://scripts/main.lua}
```

Everything else is easier to spawn from the script (`Scene.Spawn{...}`). Load errors say what was expected.

## Script

```lua
local S = {}

function S:OnCreate() end          -- self.entity is this entity
function S:OnUpdate(dt) end
function S:OnRender() end          -- Draw.* works only here
function S:OnDestroy() end
-- events: OnFixedUpdate(dt) OnCollision(other) OnTriggerEnter/Stay/Exit(other) OnTimer OnDamaged(amount, from)
--         OnDeath OnPathPoint(i) OnPathEnd OnLand OnJump OnGrab OnDrop OnClick OnHoverEnter OnHoverExit

return S
```

Each entity gets its own `self`. A script error disables that script and is shown in the Console, the scene keeps running.

Clipping, fonts and corners:

- `entity:SetClip(w, h)` (or the Clip Area component) masks everything below that entity to a `w x h` box centred on it; `entity:SetClip(nil)` removes it. The owner's rotation is ignored and a nested clip area replaces the outer one. `Draw.Clip(cx, cy, w, h)` and `Draw.ClearClip()` do the same for immediate drawing.
- `Draw.Text/Label(text, x, y, { font = "engine://fonts/Inter-SemiBold.ttf", rotation = 0, outline = "#000", outlineWidth = 2 })`; `text:SetFont(path)` for Text components.
- `Path.Rect(w, h, { tl, tr, br, bl })` takes one radius per corner (y up).
- `vector:SetShadow(color, offsetX, offsetY, blur)` draws a soft drop shadow behind a Vector (`ClearShadow()` removes it; the shadow is drawn at a world offset, not rotated). `vector:SetFixedStroke(true)` keeps the stroke width constant while the entity scales.
- `Pointer.New(entity)` then `pointer:Update(dt)` every frame gives `x, y` (world), `down, pressed, released, moved, dx, dy, vx, vy` (smoothed velocity), `travel`, `dragging`, `tap`, and `Capture(target)/IsCaptured(target)/Release()` to give one object the drag.
- Glow `spread` is a blur radius multiplier on the glow's mip chain, so it is relative to the screen and not in world units; keep it near 1 and lower `intensity` for thin lines.
- `Scene.Spawn{ parent = p, x = 0, y = 0 }` places the child at world x, y, not relative to the parent.

## Recipes

```lua
local coin = Scene.Spawn{name = "Coin", x = 100, y = 50, shape = "circle", size = 20, color = "#ffd166", glow = 1.5}
local wall = Scene.Spawn{shape = "rect", w = 800, h = 40, color = "muted", body = "static"}
local ball = Scene.Spawn{shape = "circle", size = 16, body = "dynamic", bounce = 0.6, script = "assets://scripts/ball.lua"}
local hero = Scene.Spawn{sprite = "assets://textures/hero.png", w = 64, h = 64}
local label = Scene.Spawn{text = "Score 0", fontSize = 40, color = "white", y = 300}
local e = Scene.Spawn("Plain", 0, 0)               -- name, x, y, parent? still works
local v = e:Add("Vector")                          -- any component name below
v:SetPath(Path.Star(5, 30)); v:SetFill("#ff6b81")
e:SetPosition(0, 0); e:Translate(5, 0); e:Destroy(0.5)

Tween.To(e, {x = 300, alpha = 0}, 0.6, "easeOutCubic", {delay = 0.2, onComplete = function() end})
Tween.Sequence({Tween.To(e, {y = 120}, 0.3), Tween.To(e, {scale = {2, 2}}, 0.3, "easeOutBack")})
Effect.Pulse(e)                                    -- 38 ready-made entity effects
Motion.CircleGlowPop(0, 0, {size = 40})            -- 235 ready-made animations that build their own shapes
Motion.ArrowSlide({{0, 0}, {0, 100}, {150, 100}})  -- Arrow Out style arrow from points
Motion.Plot(function(x) return math.sin(x) end, {range = {-6, 6}})

if Input.IsActionJustPressed("Jump") then end      -- keyboard, pad and touch
local mx, my = self.entity:GetMouseWorld()
Scene.Load("assets://scenes/Level2.yaml", {fade = 0.4})
Game.Add("coins", 1); Game.Save()
Audio.Play("assets://audio/hit.wav", 0.8)
```

`Scene.Spawn{}` keys: `name x y z rotation scale parent` · shape `shape` (`circle rect square star heart triangle hexagon polygon line points` or a Path) `size radius w h corner sides points color fill stroke strokeWidth` · `sprite` (image path; `w h` set its size, otherwise the image size) · `text fontSize align` · `glow glowColor` · `light lightRadius` · `body` (`"static"` or `"dynamic"`, shape follows `shape`) `mass bounce friction sensor` · `script`.

**Camera and view.** Zoom works like a field of view: the visible world is the display size times zoom, so `SetZoom(0.5)` shows half the world and makes things twice as big, `SetZoom(2)` zooms out. `Window.GetViewSize()` returns the visible world width, height (it follows the aspect mode, the window shape and the zoom); `Window.GetSize()` is the window in points; `Window.GetSafeArea()` is `x, y, w, h` in viewport pixels with y down. Poll `GetSize()` to notice resizes or rotation (there is no resize event).

**Drawing order.** Entities and `Draw.*` share one z axis (bigger z is on top; `Draw.SetZ`), so a `Draw` call at z 22 shows over an entity at z 20. `Draw.*` only works inside `OnRender`; a UI that mixes both lets one entity own the draw calls. A child's `zIndex` is absolute (world), not relative to its parent. Stroke width scales with the entity scale. Destroying an entity destroys its children. There is no clipping: cover scrolled content with an opaque entity.

**Text.** A `Text` entity is centred on its position for `align` 1, starts at it for 0 and ends at it for 2. `text:Measure()` returns the width, height of the current text, `text:SetFont("assets://fonts/x.ttf")` picks a font file (also `Path.Text` and `Glyphs`). Text variables: `Game.SetText(name, "Mira")` / `Game.GetText(name, fallback)` are saved with `Game.Save`, and `{name}` in any Text shows the number or the text. `Game.Get` and `Game.Add` return integers for whole numbers.

**Hit tests.** `vector:Contains(x, y)` tells whether a world point is inside a Vector shape (inside the fill, or on a stroke-only line). The Lua environment is the full standard library (`os.getenv`, `io`) so tools and bots can read the environment.

Tween properties: `x y position rotation scaleX scaleY scale z color alpha width height size tint shapeColor textColor fontSize lightColor lightIntensity lightRadius glowColor glowIntensity glowSpread zoom particleRate volume pitch animationSpeed fillColor strokeColor strokeWidth morph trimStart trimEnd trimOffset dashOffset shaderProgress` and any shader uniform `uName`. Alpha is 0-255. Curves: `Curves.Names()` (`linear easeIn easeOut easeInOut easeOutBack bounceOut elasticOut spring ...`).

Components (for `e:Add/Get/Has/Remove(name)` and `e:Get<Name>()`): `Sprite Animation Camera Light Velocity Collision Shape Text Glow Particles Audio Parallax Body Trigger Timer Path Health CharacterController Tilemap Vector Damage Oscillator Spawner Follow Pickup Checkpoint Respawn Music Pickable Button Teleporter CameraZone WorldBounds ShaderCanvas Canvas UIRect UIPanel UILabel UIImage UIButton UILayout`.

## Motion presets

`Motion.<Name>(...)` builds the shapes, animates them and returns the root entity. Call forms:

```lua
Motion.CirclePop(x, y, options?)          -- presets without input
Motion.Arrow(points, options?)            -- points: {{x, y}, ...} or {x1, y1, x2, y2, ...}, relative to x, y
Motion.BarChart(values, options?)         -- values: {3, 5, 2}
Motion.Plot(fn, {range = {a, b}})         -- fn is sampled into values
Motion.Typewriter("text", options?)
Motion.Play("CirclePop", options)         -- by name
```

Options: `x y size color accent duration delay width z hold parent points values text fn range samples`. `duration` stretches the whole timeline, `hold` removes it that many seconds after it ends (default: stays). Browse them with previews in View > Effects and Easing > Motion. Hovering a `Motion.*`, `Effect.*` or `Tween.*` call in the script editor plays your own call in a preview.

<!-- MOTION:BEGIN -->
| Category | Presets |
|---|---|
| Shapes | `<Shape><Style>` with shape Circle, Square, Triangle, Star, Heart, Hexagon and style Pop (pops in with a little overshoot), GlowPop (pops in with a glow flash that settles), Grow (grows and fades in smoothly), Fade (fades in while rising a little), Spin (spins in from nothing), Drop (drops from above and bounces), Elastic (springs in with an elastic wobble), Draw (outline draws itself, then the fill fades in), Ripple (pops in and sends two ripples out) |
| Exits | `<Shape><Style>`, same shapes, style PopOut (swells a little and pops away), FadeOut (fades away while drifting up), SpinOut (spins away into nothing), Explode (bursts into spinning shards) |
| Loops | `<Shape><Style>`, same shapes, style Pulse (pops in, then pulses with a glow), Wobble (pops in, then wobbles side to side), Float (pops in, then floats over its shadow) |
| Explain | **SliceSplit** Rectangles under a curve split in half again and again (Riemann refinement); **AreaSweep** The area under a curve fills left to right behind a sweeping edge; **TangentLine** A tangent line slides along a curve, tilting with the slope; **SecantTangent** A secant line closes in until it becomes the tangent; **FunctionPen** A pen draws the curve with dashed guides to both axes; **SineFromCircle** A point circling a wheel traces a sine wave beside it; **VectorAdd** Two vectors placed tip to tail, then their sum; **Pythagoras** A right triangle and the squares on its sides; **FractionBar** A bar splits into parts and some of them fill (values: parts, filled); **Clock** A clock face draws and its hands sweep; **MatrixGrid** A grid of cells pops in along its diagonals; **TreeGrow** A tree grows branch by branch; **NetworkGraph** Nodes pop in and edges connect them; **FlowChart** Boxes appear in a row, joined by arrows; **Timeline** A line draws and milestones pop along it; **Venn** Two circles slide together and overlap; **Lissajous** A Lissajous figure traces itself; **Spirograph** A spirograph loop draws round and round; **RoseCurve** A five-petal rose curve traces itself; **Projectile** A ball flies along a parabola, leaving its path; **PointPlot** A point appears with dashed guides and its (x, y) label |
| Charts | **StackedBars** Stacked bars build up layer by layer (values: one column per three); **RadarChart** A spider chart: web, then the value shape grows out (values 0..1); **PercentPie** A pie wedge sweeps to a percentage (values[1] is 0..1); **SortBars** Bars swap places until they are sorted; **Equalizer** Audio equalizer bars bounce; **Axes** X and Y axes draw with ticks; **Grid** A grid fades in line by line; **NumberLine** A number line with ticks and labels; **Plot** Axes, then the curve draws (points, values or fn); **PlotTracer** The curve draws while a glowing dot rides its tip; **PlotArea** The curve draws and the area under it fills; **Parabola** y = x squared draws over axes; **Riemann** A curve, then rectangles under it grow one by one; **BarChart** Bars grow one after another (values); **BarChartLabels** Bars grow and their values pop on top (values); **Histogram** Thin bars rise in a wave (values); **LineChart** A line through the values draws and its points pop; **ScatterPlot** Points pop in and a trend line draws (points); **PieChart** Pie slices pop in one by one (values); **DonutChart** A donut chart draws around segment by segment (values); **VectorArrow** A math vector arrow from the origin to points[1] with a label; **AngleArc** Two rays and the angle between them, labeled; **UnitCircle** A circle with a radius that sweeps around |
| Physics | **BouncingBall** A ball drops and bounces, squashing on each hit; **SpringMass** A weight bobs on a coil spring |
| Ambient | **Rain** Rain streaks fall; **Snow** Snowflakes drift down and sway; **Bubbles** Bubbles rise, wobble and pop; **Fireflies** Glowing fireflies wander and blink; **Starfield** Stars twinkle across the sky; **MeteorShower** Meteors streak diagonally across |
| Effects | **Explosion** A flash, a shockwave and flying debris; **Portal** Glowing rings spin open into a portal; **Lightning** A lightning bolt strikes and flickers (points); **MagicCircle** A rune circle draws itself and slowly turns; **RadarSweep** A radar beam sweeps and blips light up; **Heartline** A heart monitor line beats across; **RippleRings** Three rings spread out from a point; **PulseDot** A dot with a ring that pulses three times; **Burst** Dots shoot out from the center and fade; **Sparkle** Little stars twinkle around a point; **Confetti** Colored pieces burst up and fall; **Firework** A spark rises and bursts into a ring of light; **Shockwave** A thick glowing ring blasts outward and thins; **Heartbeat** A heart beats twice; **Orbit** Three moons circle a planet; **Spinner** A loading arc that spins; **DotsLoader** Three dots bounce one after another; **WaveDots** A row of dots rolls like a wave; **Pendulum** A pendulum swings back and forth; **Gauge** A gauge needle sweeps to a value (values[1] is 0..1) |
| Text | **ScrambleText** Random letters settle into the text, left to right; **WordsPop** Words pop in one at a time (text); **TextSlide** Text slides in from the left behind a moving bar (text); **NeonText** Neon sign text flickers on and hums (text); **Caption** An explainer caption bar: a small heading and a line of text (text); **CountUp** A number counts up to values[1]; **Typewriter** Text types itself out with a blinking caret; **TitlePop** A title pops in and an underline draws; **GlowTitle** A glowing title fades up; **TextRise** Text fades in while rising; **LettersPop** Each letter pops in, one after another; **LettersDrop** Letters fall in and bounce; **LettersWave** Letters ride a wave; **LettersSpin** Letters spin in one by one; **RainbowText** Letters pop in, each in its own color |
| UI | **ProgressBar** A bar fills from left to right (values[1] is 0..1); **ProgressRing** A ring fills around like a timer (values[1] is 0..1); **Checkmark** A green circle pops and a check draws inside; **CrossMark** A red circle pops and an X draws inside; **Toggle** A switch slides on and lights up; **Badge** A circle badge pops with a number (text); **Pin** A map pin drops in with a shadow; **Underline** A line draws under a point (size is the length); **Strike** A line strikes through, left to right; **CircleHighlight** A hand drawn loop circles a spot; **BoxHighlight** A rounded box draws around a spot; **Spotlight** A soft light ring closes in on a spot |
| Lines | **LineDraw** A line draws through the points; **GlowLine** A glowing line draws through the points; **DashedLine** A dashed line draws, then marches; **PathTrace** A dot travels the path and leaves a trail; **Comet** A glowing head races along with a short tail; **Arrow** An arrow draws itself along the points; **GlowArrow** A glowing arrow draws itself along the points; **ArrowSlide** Arrow Out style: the arrow draws, then slides out along its path (size is the slide distance); **DoubleArrow** A line draws with heads on both ends; **CurvedArrow** A curved arrow arcs from the first point to the last; **Connector** Two dots pop and a line joins them; **Laser** A bright beam flashes between two points; **ZigZag** A zig-zag line draws; **Spiral** A spiral winds out from the center; **SineWave** A sine wave draws across; **PolygonDraw** A closed shape through the points draws, then fills; **Scribble** A quick hand-drawn scribble |
| Morph | **MorphCircleSquare** Circle morphs into a Square; **MorphSquareStar** Square morphs into a Star; **MorphStarHeart** Star morphs into a Heart; **MorphHexagonCircle** Hexagon morphs into a Circle; **MorphTriangleCircle** Triangle morphs into a Circle |
<!-- MOTION:END -->

## Every function

Generated from the bindings (`python3 docs/tools/gen_api.py`). `?` marks an optional argument; `Type:Method` is called on a handle (`e:GetBody():ApplyImpulse(...)`).

<!-- API:BEGIN -->
**Global tables**

- `Area.` Draw(xr?, alpha?) · Polygon(xr?)
- `Audio.` FadeTo(voice, volume, seconds?, stopWhenSilent?) · GetBusVolume(bus) · GetMasterVolume() · IsPlaying(voice) · Play(path, volume?, pitch?, loop?, bus?) · SetBusVolume(bus, volume) · SetMasterVolume(volume) · SetVolume(voice, volume) · Stop(voice) · StopAll()
- `Axes.` Area(fn, from, to, options?) · Draw(progress?, zoom?) · New(options) · P(x, y) · Plot(fn, from, to, options?) · Riemann(fn, from, to, options?) · Tracer(fn, x, options?)
- `Backdrop.` Fill(x, y, zoom?, color?) · Grid(x, y, zoom?, options?)
- `CameraRig.` Apply(t, camera?) · At(t) · Move(segment, pose, curve?, options?) · New(timeline, start?) · Sway(x?, y?, zoom?) · Track(segment, fn)
- `Captions.` Add(from, to, title, text?) · Draw(t, x?, y?, scale?) · New(timeline, options?) · Visible(t)
- `Color.` Define(name, color) · Get(color) · Hex(hex, alpha?) · Mix(from, to, t) · Names() · New(r, g, b, a?) · Unpack(color, alpha?) · WithAlpha(color, alpha)
- `Curves.` Cubic(x1, y1, x2, y2) · Elastic(mode, period?) · Evaluate(c, t) · Flipped(c) · Interval(begin, end, curve?) · Names() · Reversed(c) · SawTooth(n) · Sine(cycles?) · Spring(dampingRatio?, frequency?) · Steps(n) · Threshold(t)
- `Draw.` Arrow(x1, y1, x2, y2, width, color, head?) · Bezier(x0, y0, c1x, c1y, c2x, c2y, x1, y1, width, r, g, b, a?) · Circle(x, y, radius, r, g, b, a?) · ClearClip() · Clip(number, number2, number3, number4) · Dashed(x1, y1, x2, y2, width, dash, gap, color, progress?) · GetZ() · Label(text, x, centerY, size, color, align?) · Layer(z, fn) · Line(x1, y1, x2, y2, width, r, g, b, a?) · MeasureText(text, size, fontPath?) · Path(path, x, y, options?) · Polygon(points, r, g, b, a?) · Polyline(points, width, r, g, b, a?, closed?) · Rect(x, y, w, h, r, g, b, a?, rotation?) · Ring(x, y, radius, thickness, r, g, b, a?) · RoundRect(x, y, w, h, radius, r, g, b, a?) · SetZ(z) · Text(text, x, y, size, r, g, b, a?, align?)
- `Effect.` Categories() · Chain(target, list, options?) · Info(name) · Names(category?) · Play(target, name, options?) · Stagger(targets, name, each?, options?)
- `Game.` Add(name, amount?) · DeleteSave(slot?) · Get(name, default?) · GetText(string, string2?) · Has(name) · HasSave(slot?) · HasText(string) · Load(slot?) · Quit() · Remove(name) · Reset() · Save(slot?) · Set(name, value) · SetText(string, string2)
- `Glyphs.` Bounds() · Count() · Draw(x, y, progress?, options?) · New(text, size, align?, fontPath?) · Size()
- `Input.` AddBinding(string, string2) · ClearBindings(name) · DescribeBinding(binding) · GetActions() · GetAxis(action | negativeKey, positiveKey) · GetBindings(name) · GetGamepadAxis(axis) · GetMouseWheel() · GetPinch() · GetPressedBinding() · GetTouches() · GetVector(name) · HasAction(name) · IsActionDown(name) · IsActionJustPressed(name) · IsActionJustReleased(name) · IsGamepadConnected() · IsGamepadDown(button) · IsKeyDown(key) · IsKeyJustPressed(key) · IsKeyJustReleased(key) · IsMouseDown(button) · IsMouseInViewport() · IsMouseJustPressed(button) · IsMouseJustReleased(button) · IsTouchDevice() · Rebind(name, binding) · RemoveBinding(string, string2) · ResetBindings(name?) · SetActionValue(name, value) · SetFloatingStick(floatingStick) · SetGamepadDeadzone(d) · SetTouchControls(touchControls)
- `log.` error() · info() · trace() · warn()
- `Math.` Angle(x1, y1, x2, y2) · Clamp(value, min, max) · Clamp01(value) · Distance(x1, y1, x2, y2) · Integrate(fn, from, to, steps?) · Length(x, y) · Lerp(a, b, t) · MoveTowards(current, target, maxDelta) · Normalize(x, y) · Progress(t, from, to) · Remap(value, inMin, inMax, outMin, outMax, clamp?) · Sign(value) · Smoothstep(edge0, edge1, x) · Window(t, from, to, fade?)
- `Motion.` Categories() · Info(name) · Names(category?) · Play(name, options?)
- `Music.` GetCurrent() · IsPaused() · IsPlaying() · Pause() · Play(path, volume?, fadeIn?, fadeOutPrevious?, loop?) · Resume() · SetVolume(volume) · Stop(fadeOut?)
- `Path.` Arc(r, startDeg, endDeg) · Circle(r) · Ellipse(rx, ry) · Heart(200) · Line(x1, y1, x2, y2) · New() · Points(points, closed?) · Polygon(n, r) · Rect(w, h, r?) · Star(n, outer, inner?) · Svg(source, fitSize?) · Text(text, size, align?, fontPath?) · TextGlyphs(text, size, align?, fontPath?)
- `Physics.` CreateDistanceJoint(a, b, ax?, ay?, bx?, by?, length?) · CreateRevoluteJoint(a, b, x, y) · CreateWeldJoint(a, b) · CreateWheelJoint(a, b, x, y, axisX?, axisY?) · GetAwakeCount() · GetBodyCount() · GetGravity() · GetSubsteps() · QueryBox(minX, minY, maxX, maxY) · QueryCircle(x, y, radius) · QueryPoint(x, y) · Raycast(originX, originY, dirX, dirY, maxDistance?) · SetGravity(x, y) · SetIterations(velocity, position?) · SetSleepEnabled(enabled) · SetSubsteps(count) · WakeAll()
- `Plot.` At(x) · Draw(xr?, zoom?) · Points(xr?) · Value(x)
- `Pointer.` Capture(target) · Inside(cx, cy, w, h) · IsCaptured(target) · New(entity, options?) · Release() · Update(dt)
- `Riemann.` Draw(n, options?) · Rects(n, options?) · Sum(n)
- `Scene.` Clone(entityOrName, x?, y?) · Find(name) · FindAll(name) · GetPath() · Instantiate(path, x?, y?, parent?) · IsSolidAt(x, y) · IsTransitioning() · Load(path, table?) · OverlapsSolid(x1, y1, x2, y2) · Reload(table?) · SetAmbient(r, g, b, arg4?) · SetFixedStep(seconds) · SetLighting(enabled) · Spawn(nameOrSpec, x?, y?, parent?)
- `Suggest.` Closest(name, candidates) · Hint(name, candidates)
- `Time.` Delta() · FixedDelta() · Now()
- `Timeline.` Active(t, name) · Add(name, seconds) · Current(t) · Eased(t, name, curve?) · New(segments?) · Progress(t, name) · Span(name) · Total() · Window(t, name, fade?)
- `Tween.` Along(target, path, seconds, curve?, options?) · Call(fn) · Count() · Delay(s) · From(target, props, seconds, curve?, options?) · FromTo(target, from, to, seconds, curve?, options?) · IsTweening(target, property?) · Kill(target, property?) · KillAll() · Parallel(items, options?) · Properties() · Sequence(items, options?) · Spring(target, props, options?) · Stagger(targets, props, seconds, curve?, each?, options?) · To(target, props, seconds, curve?, options?) · Value(from, to, seconds, curve, fn, options?)
- `Video.` Step(t, dt)
- `Window.` GetRenderScale() · GetSafeArea() · GetSize() · GetViewSize() · IsFullscreen() · SetFullscreen(fullscreen) · SetRenderScale(0.25..1)

**Handles (methods)**

- `Animation:` GetFrame() · GetFrameCount() · GetFrameDuration() · GetFrameSize() · GetSheet() · GetSpeed() · IsFinished() · IsLooping() · IsPlaying() · Pause() · Play() · Restart() · SetFrame(frame) · SetFrameDuration(frameDuration) · SetLoop(loop) · SetSheet(path, frameW, frameH, frameCount, frameDuration?, loop?) · SetSpeed(speed)
- `Audio:` GetPitch() · GetVolume() · IsPlaying() · Play() · SetClip(clip) · SetLoop(loop) · SetPitch(pitch) · SetVolume(volume) · Stop()
- `Body:` AddForce(x, y) · AddImpulse(x, y) · AddImpulseAt(impulseX, impulseY, pointX, pointY) · AddTorque(torque) · CreateBox(width, height, mass?) · CreateCapsule(halfLength, radius, mass?) · CreateCircle(radius, mass?) · CreatePoints(points, cornerRadius?, mass?) · CreatePolygon(sides, radius, mass?) · Destroy() · GetAngle() · GetBounce() · GetFriction() · GetGroup() · GetLayer() · GetMask() · GetMass() · GetShape() · GetSize() · GetSpin() · GetVelocity() · HasShape() · IsAwake() · IsLocked() · IsOneWay() · IsSensor() · IsStatic() · SetBounce(bounce) · SetCanSleep(enabled) · SetCollidesWith(...) · SetDamping(linear, angular?) · SetFriction(static, moving?) · SetGroup(group) · SetIgnoreGravity(enabled) · SetLayer(layer) · SetLocked(rotation, position?) · SetMask(bits) · SetMass(mass) · SetOneWay(enabled, normalX?, normalY?) · SetSensor(enabled) · SetSpin(radiansPerSecond) · SetVelocity(x, y) · Sleep() · Stop() · Teleport(x, y, degrees?) · Wake()
- `Button:` IsEnabled() · IsHovered() · IsPressed() · SetClickSound(clickSound?) · SetEnabled(enabled) · SetHoverScale(hoverScale) · SetHoverSound(hoverSound?) · SetPressScale(pressScale)
- `Camera:` GetZoom() · IsPrimary() · SetFollow(name, offsetX?, offsetY?) · SetZoom(z) · Shake(amount)
- `CameraZone:` IsEnabled() · SetBoundsMax(x, y) · SetBoundsMin(x, y) · SetEnabled(enabled) · SetSpeed(speed) · SetUseBounds(useBounds) · SetZoom(zoom)
- `Canvas:` GetScale() · GetSize() · IsVisible() · SetReferenceSize(w, h) · SetSortOrder(sortOrder) · SetVisible(visible)
- `CharacterController:` GetAcceleration() · GetAirControl() · GetCoyoteTime() · GetDeceleration() · GetFacing() · GetGravity() · GetJumpBuffer() · GetJumpCut() · GetJumpSpeed() · GetMaxFallSpeed() · GetMaxJumps() · GetMode() · GetMoveSpeed() · GetPlayerInput() · GetVelocity() · HitCeiling() · HitWall() · IsGrounded() · Jump() · JustJumped() · JustLanded() · ReleaseJump() · SetAcceleration(acceleration) · SetAirControl(airControl) · SetCoyoteTime(coyoteTime) · SetDeceleration(deceleration) · SetGravity(gravity) · SetInput(x, y?) · SetJumpBuffer(jumpBuffer) · SetJumpCut(jumpCut) · SetJumpSpeed(jumpSpeed) · SetMaxFallSpeed(maxFallSpeed) · SetMaxJumps(maxJumps) · SetMode(mode) · SetMoveSpeed(moveSpeed) · SetPlayerInput(playerInput) · SetVelocity(x, y)
- `Checkpoint:` GetOffset() · IsEnabled() · SetEnabled(enabled) · SetOffset(x, y) · SetSound(sound?)
- `Collision:` GetBounds() · GetOffset() · GetSize() · SetOffset(x, y) · SetSize(w, h)
- `Curve:` Flipped() · Transform(t)
- `Damage:` GetAmount() · GetFilter() · GetInterval() · GetKnockback() · IsEnabled() · Reset() · SetAmount(amount) · SetDestroyOnHit(destroyOnHit) · SetEnabled(enabled) · SetFilter(name?) · SetInterval(interval) · SetKnockback(knockback)
- `Entity:` Add(name) · Destroy(delay?) · FindByName(name) · Get(name) · GetAnimation() · GetAudio() · GetBody() · GetButton() · GetCamera() · GetCameraZone() · GetCanvas() · GetCharacterController() · GetCheckpoint() · GetChildren() · GetCollision() · GetDamage() · GetFollow() · GetGlow() · GetHealth() · GetLight() · GetMouseWorld() · GetMusic() · GetName() · GetOscillator() · GetParallax() · GetParent() · GetParticles() · GetPath() · GetPickable() · GetPickup() · GetPosition() · GetRespawn() · GetRotation() · GetScale() · GetScript() · GetShaderCanvas() · GetShape() · GetSpawner() · GetSprite() · GetTeleporter() · GetText() · GetTilemap() · GetTimer() · GetTrigger() · GetUIButton() · GetUIImage() · GetUILabel() · GetUILayout() · GetUIPanel() · GetUIRect() · GetVector() · GetVelocity() · GetWorldBounds() · GetWorldPosition() · GetZIndex() · Has(name) · IsValid() · IsVisible() · Remove(name) · SetClip(number?, number2?) · SetName(name) · SetPosition(x, y) · SetRotation(degrees) · SetScale(x, y?) · SetScript(path) · SetVisible(visible) · SetZIndex(z) · Translate(dx, dy)
- `Follow:` GetOffset() · GetTarget() · IsEnabled() · SetEnabled(enabled) · SetOffset(x, y) · SetSmoothing(smoothing) · SetStopDistance(stopDistance) · SetTarget(name?)
- `Glow:` GetColor() · GetIntensity() · GetSpread() · IsEnabled() · SetColor(r, g, b, a?) · SetEnabled(enabled) · SetIntensity(intensity) · SetSpread(spread)
- `Health:` Damage(amount, source?) · GetCurrent() · GetFraction() · GetInvulnerability() · GetMax() · Heal(amount) · IsDead() · IsInvulnerable() · Kill(source?) · Revive() · SetCurrent(current) · SetInvulnerability(invulnerability) · SetMax(max)
- `Joint:` Destroy() · DisableMotor() · GetAngle() · GetLength() · GetTranslation() · GetType() · IsValid() · SetCollideConnected(enabled) · SetLength(length) · SetLimits(lower, upper) · SetMotor(speed, maxTorque) · SetSoftness(compliance) · SetSpring(hertz, damping?)
- `Light:` GetColor() · GetIntensity() · GetRadius() · IsEnabled() · SetColor(r, g, b, arg4?) · SetEnabled(enabled) · SetIntensity(intensity) · SetRadius(radius)
- `Music:` AddTrack(track) · ClearTracks() · GetTrackCount() · Next() · Play() · SetShuffle(shuffle) · SetVolume(volume) · Stop()
- `Oscillator:` GetFrequency() · GetMove() · IsPlaying() · Play() · SetFrequency(hz) · SetMove(x, y) · SetPulse(fraction) · SetSpin(degreesPerSecond) · SetSwing(degrees) · Stop()
- `Parallax:` GetFactor() · SetFactor(x, y?)
- `Particles:` Burst(count) · GetRate() · IsPlaying() · Play() · Restart() · SetRate(rate) · Stop()
- `Path:` AddPoint(x, y) · ClearPoints() · GetPoint(i) · GetPointCount() · GetSpeed() · IsPlaying() · Pause() · Play() · Restart() · SetPoint(i, x, y) · SetSpeed(speed)
- `Pickable:` Drop() · IsEnabled() · IsHeld() · SetEnabled(enabled) · SetSmoothing(smoothing) · SetThrowStrength(throwStrength)
- `Pickup:` GetAmount() · GetKey() · IsEnabled() · SetAmount(amount) · SetDestroyOnPickup(destroyOnPickup) · SetEnabled(enabled) · SetKey(name?) · SetSound(sound?)
- `Respawn:` GetPoint() · Respawn() · SetDelay(delay) · SetKillY(y) · SetPoint(x, y) · SetUseKillY(useKillY)
- `ShaderCanvas:` GetError() · GetFloat(name) · GetProgress() · GetShader() · HasUniform(name) · IsEnabled() · SetBool(name, b) · SetColor(name, r, g, b, a?) · SetEnabled(enabled) · SetFloat(name, v) · SetInt(name, v) · SetProgress(progress) · SetShader(shader?) · SetTexture(name, path) · SetVec2(name, x, y) · SetVec3(name, x, y, z) · SetVec4(name, x, y, z, w)
- `Shape:` GetColor() · GetSize() · SetColor(r, g, b, a?) · SetSize(w, h)
- `Spawner:` GetAlive() · GetInterval() · GetPrototype() · GetSpawned() · IsRunning() · SetInterval(interval) · SetMaxAlive(maxAlive) · SetPrototype(name) · SetVelocity(x, y) · Spawn(count?) · Start() · Stop()
- `Sprite:` ClearRegion() · GetFlip() · GetSize() · GetTexture() · GetTint() · SetFlip(flipX, flipY) · SetRegion(x, y, w, h) · SetSize(w, h) · SetTexture(path, keepSize?) · SetTint(r, g, b, a?)
- `Teleporter:` GetOffset() · GetTarget() · IsEnabled() · SetEnabled(enabled) · SetKeepVelocity(keepVelocity) · SetOffset(x, y) · SetTarget(name?)
- `Text:` GetAlign() · GetColor() · GetFont() · GetFontSize() · GetText() · Measure() · SetAlign(align) · SetColor(r, g, b, a?) · SetFont(font?) · SetFontSize(fontSize) · SetText(text)
- `Tilemap:` Clear() · EraseTile(x, y) · FillRect(x1, y1, x2, y2, id) · FloodFill(x, y, id) · GetBounds() · GetCellSize() · GetTile(x, y) · GetTileAt(wx, wy) · GetTileCount() · GetTileSize() · GetTilesetSize() · GetTint() · IsSolid() · IsVisible() · SetSolid(solid) · SetTile(x, y, id, flipX?, flipY?) · SetTileSize(tileSize) · SetTileset(path, tileW, tileH, margin?, spacing?) · SetTint(r, g, b, a?) · SetVisible(visible) · TileToWorld(x, y) · WorldToTile(wx, wy)
- `Timer:` GetElapsed() · GetInterval() · GetRemaining() · IsRepeating() · IsRunning() · Restart() · SetInterval(interval) · SetRepeat(repeating) · Start() · Stop()
- `Trigger:` Contains(entity) · GetFilter() · GetInside() · GetOffset() · GetSize() · IsEnabled() · SetEnabled(enabled) · SetFilter(name?) · SetOffset(x, y) · SetSize(w, h)
- `Tween:` Curve(curve) · Delay(seconds) · Duration() · Forward() · IsFinished() · IsPlaying() · Kill() · OnComplete(fn) · OnRepeat(fn) · OnStart(fn) · OnUpdate(fn) · Pause() · Play() · Progress() · Repeat(count?) · Restart() · Resume() · Reverse() · Seek(t) · Speed(speed) · Then(fn) · Yoyo(enabled?)
- `UIButton:` IsEnabled() · IsFocused() · IsHovered() · IsPressed() · SetClickSound(clickSound?) · SetEnabled(enabled) · SetHoverScale(hoverScale) · SetHoverSound(hoverSound?) · SetPressScale(pressScale)
- `UIImage:` GetTint() · SetPreserveAspect(preserveAspect) · SetTexture(texture) · SetTint(r, g, b, a?)
- `UILabel:` GetColor() · GetFontSize() · GetText() · SetAlign(horizontal, vertical?) · SetColor(r, g, b, a?) · SetFontSize(fontSize) · SetShadow(shadow) · SetText(text)
- `UILayout:` SetDirection(direction) · SetEnabled(enabled) · SetPadding(left, bottom?, right?, top?) · SetSpacing(spacing)
- `UIPanel:` GetColor() · SetBorder(width, r?, g?, b?, a?) · SetColor(r, g, b, a?) · SetCornerRadius(r)
- `UIRect:` GetPosition() · GetRect() · GetSize() · IsVisible() · SetAnchors(minX, minY, maxX?, maxY?) · SetBlocksInput(blocksInput) · SetMargin(left, bottom, right, top) · SetPivot(x, y) · SetPosition(x, y) · SetSize(w, h) · SetVisible(visible)
- `Vector:` ClearGradient() · ClearMorphTarget() · ClearShadow() · Contains(number, number2) · GetBounds() · GetDash() · GetFill() · GetLength() · GetMorph() · GetPath() · GetPointAt(t) · GetStroke() · GetStrokeWidth() · GetTrim() · IsFillEnabled() · IsStrokeEnabled() · MorphTo(path, seconds, curve?, options?) · SetCap(cap) · SetCircle(radius) · SetDash(length, gap?, offset?) · SetFill(r, g, b, a?) · SetFillEnabled(enabled) · SetFixedStroke(fixedStroke?) · SetGradient(x1, y1, x2, y2, r1, g1, b1, a1, r2, g2, b2, a2) · SetHeart(size) · SetJoin(join, miterLimit?) · SetMorph(amount) · SetMorphTarget(path) · SetPath(path) · SetPolygon(sides, radius) · SetRect(width, height, radius?) · SetShadow(r, number?, number2?, number3?) · SetStar(points, outer, inner?) · SetStroke(r, g, b, a?) · SetStrokeEnabled(enabled) · SetStrokeWidth(width) · SetSvg(source, fitSize?, applyStyle?) · SetTrim(start, end, offset?)
- `VectorPath:` Append(path) · ArcTo(cx, cy, radius, startDeg, endDeg) · Close() · ContourCount() · Copy() · CubicTo(c1x, c1y, c2x, c2y, x, y) · GetBounds() · GetLength() · GetPointAt(t) · IsEmpty() · LineTo(x, y) · MoveTo(x, y) · QuadTo(cx, cy, x, y) · Rotate(degrees) · Scale(sx, sy?) · Translate(dx, dy)
- `Velocity:` Add(dx, dy) · Get() · Set(x, y)
- `WorldBounds:` GetMax() · GetMin() · IsEnabled() · SetEnabled(enabled) · SetMargin(margin) · SetMax(x, y) · SetMin(x, y) · SetMode(mode)
<!-- API:END -->
