# Animation: tweens, curves and vector shapes

SIMPEngine has a Flutter-style animation system. Values move from A to B along a curve, can be chained and staggered, behave like springs, and can drive vector shapes that morph, draw themselves and follow paths. Everything here is usable from Lua; the engine side lives in `Engine/Engine/Animation/` and `Engine/Engine/Vector/`.

| Flutter | SIMPEngine |
|---|---|
| `Tween` + `AnimationController.forward()` | `Tween.To(entity, {x = 300}, 0.6, "easeOut")` |
| `Curves.easeOutBack`, `Cubic(...)`, `Interval(...)` | `Curves.easeOutBack`, `Curves.Cubic(...)`, `Curves.Interval(...)` (same names and values as Flutter) |
| `controller.reverse()`, `repeat(reverse: true)`, `value = 0.5` | `handle:Reverse()`, `{loop = true, yoyo = true}`, `handle:Seek(0.5)` |
| Implicit animations (`AnimatedContainer`) | Calling `Tween.To` again on the same property takes over smoothly from where it is |
| `TweenSequence`, staggered animations | `Tween.Sequence`, `Tween.Parallel`, `Tween.Stagger` |
| `SpringSimulation` | `Tween.Spring(entity, {x = 0}, {stiffness = 180, damping = 14})` |
| `addListener` / `AnimatedBuilder` | `onUpdate` callbacks, `Tween.Value(from, to, s, curve, fn)` |
| `CustomPainter` + `Path` | `Draw.*` in `OnRender`, `Path.*`, Vector Shape component |
| `PathMetric.extractPath` (line drawing) | Vector `trimStart/trimEnd/trimOffset` |
| Motion along a path | `Tween.Along(entity, path, seconds, curve, {orient = true})` |
| Shape morphing | `vector:MorphTo(Path.Heart(200), 0.8, "elasticOut")` |

## Tweens

```lua
local t = Tween.To(self.entity, {x = 300, rotation = 90, alpha = 0}, 0.6, "easeOutBack", {
    delay = 0.2, loop = 2, yoyo = true, speed = 1,
    onStart = function() end,
    onUpdate = function(eased) end,
    onComplete = function() end,
    onRepeat = function() end,
})
```

- `Tween.To(target, props, seconds [, curve] [, options])` animates from the current values.
- `Tween.From(target, props, ...)` animates from `props` to the current values.
- `Tween.FromTo(target, fromProps, toProps, seconds [, curve] [, options])`.
- `Tween.Value(from, to, seconds, curve, function(v) ... end [, options])` animates a plain number.
- `Tween.Along(target, pathOrVectorEntity, seconds [, curve] [, {orient, orientOffset, relative}])` moves along a curve. A Vector entity's outline is used in world space; a `Path` is used as-is (or relative to the start position with `relative = true`).
- `Tween.Spring(target, props [, {stiffness, damping, mass, threshold, onComplete}])` is physics-based with no duration. Calling it again on the same entity retargets and keeps the velocity, which is what you want for drag-and-release and following.
- `Tween.Sequence({...} [, options])`, `Tween.Parallel({...} [, options])`: items are tweens, numbers (pauses) or functions (called at that point). Groups nest and can repeat, yoyo, reverse and seek as one.
- `Tween.Stagger(entities, props, seconds [, curve] [, each = 0.1] [, options])`.
- `Tween.Delay(s)`, `Tween.Call(fn)` make sequence items explicitly.
- `Tween.Kill(entity [, property])`, `Tween.KillAll()`, `Tween.IsTweening(entity)`, `Tween.Count()`, `Tween.Properties()`.

Values: numbers, tables for multi-value properties (`position = {x, y}`, `color = {r, g, b, a}`, missing entries keep their current value), or relative strings `"+=100"` / `"-=50"`. Options use `loop` (number of extra plays, `true` = forever) because `repeat` is a Lua keyword (`["repeat"] = 2` also works). The default curve is `easeInOut`.

**Handles** (Flutter's `AnimationController`): `Play()`, `Pause()`, `Resume()`, `Restart()`, `Reverse()`, `Forward()`, `Seek(0..1)`, `Kill()`, `Progress()`, `IsPlaying()`, `IsFinished()`, `Duration()`, and chainable setup `Delay(s)`, `Repeat(n)`, `Yoyo(b)`, `Speed(s)`, `Curve(c)`, `OnStart/OnUpdate/OnComplete/Then/OnRepeat(fn)`. A tween stays alive after finishing while a script holds its handle, so you can `Reverse()` or `Restart()` it later. Configure tweens before putting them in a group.

**Animatable properties**

| Group | Names |
|---|---|
| Transform | `x`, `y`, `position`, `rotation`, `scaleX`, `scaleY`, `scale`, `z` |
| Look (picks Sprite, then Vector, Shape, Text, Tilemap) | `color`, `alpha` |
| Size (Sprite or Shape) | `width`, `height`, `size` |
| Specific | `tint`, `shapeColor`, `textColor`, `fontSize`, `lightColor`, `lightIntensity`, `lightRadius`, `glowColor`, `glowIntensity`, `glowSpread`, `zoom` (camera), `particleRate`, `volume`, `pitch`, `animationSpeed` |
| Vector Shape | `fillColor`, `strokeColor`, `strokeWidth`, `morph`, `trimStart`, `trimEnd`, `trimOffset`, `dashOffset` |

Moving an entity with a rigid body moves the body too. Tweens run after scripts' `OnUpdate`/`OnFixedUpdate` and before path followers, triggers and physics. They are cleared when the scene is cleared (Play/Stop restores the scene without them).

## Curves

`Curves.<name>` holds every Flutter curve: `linear`, `decelerate`, `ease`, `easeIn`, `easeOut`, `easeInOut`, `fastOutSlowIn`, `slowMiddle`, `fastLinearToSlowEaseIn`, `linearToEaseOut`, `easeInToLinear`, `easeIn/Out/InOut` × `Sine/Quad/Cubic/Quart/Quint/Expo/Circ/Back`, `bounceIn/Out/InOut`, `elasticIn/Out/InOut`, plus Material 3 `standard`, `standardAccelerate`, `standardDecelerate`, `emphasizedAccelerate`, `emphasizedDecelerate`, and `spring`. Names are also accepted as strings (`"easeOutBack"`, case-insensitive).

Builders: `Curves.Cubic(x1, y1, x2, y2)` (CSS/Flutter cubic-bezier), `Curves.Interval(begin, end [, curve])` (staggering inside one animation), `Curves.Steps(n)`, `Curves.SawTooth(n)`, `Curves.Threshold(t)`, `Curves.Elastic("in"|"out"|"inOut", period)`, `Curves.Spring(dampingRatio, frequency)`, `Curves.Sine(cycles)` (shakes and pulses that return to 0), `Curves.Flipped(c)`, `Curves.Reversed(c)`. Evaluate with `Curves.Evaluate(c, t)` or `c:Transform(t)`; list with `Curves.Names()`.

## Vector shapes

Add Component → Vector Shape (or `entity:Add("Vector")`). The shape is a path of lines and cubic Béziers in the entity's local space, filled and/or stroked, anti-aliased at any zoom, lit by scene lighting, and usable with Glow.

- Fill: solid color or a linear gradient (`SetGradient(x1, y1, x2, y2, r1, g1, b1, a1, r2, g2, b2, a2)`). Contours inside other contours become holes.
- Stroke: width, caps `butt/round/square`, joins `miter/round/bevel`, trim (`SetTrim(start, end, offset)`), dashes (`SetDash(length, gap, offset)`; animate `dashOffset` for marching ants).
- Morph: `SetMorphTarget(path)` + `morph` (0..1), or `MorphTo(path, seconds [, curve] [, options])`, which starts from whatever is on screen. Shapes are matched by arc length with the best rotation, so any shape morphs into any other; extra contours grow out of the center.
- Shapes: `SetCircle`, `SetRect(w, h [, radius])`, `SetPolygon(sides, r)`, `SetStar(points, outer [, inner])`, `SetHeart(size)`, `SetSvg(fileOrPathDataOrSvgText [, fitSize] [, applyStyle])`, `SetPath(path)`.
- Queries: `GetLength()`, `GetPointAt(t)` (world x, y, angle), `GetBounds()`.

The inspector section has shape presets, Load SVG, fill/gradient, stroke, caps, joins, trim and dash, plus morph target presets with an Amount slider. Shapes are saved in scenes (`VectorShape:`).

## Paths (`Path` global)

`Path.New()`, `Path.Rect(w, h [, r])`, `Path.Circle(r)`, `Path.Ellipse(rx, ry)`, `Path.Polygon(n, r)`, `Path.Star(n, outer [, inner])`, `Path.Heart(size)`, `Path.Line(x1, y1, x2, y2)`, `Path.Arc(r, startDeg, endDeg)`, `Path.Points({x1, y1, ...} or {{x, y}, ...} [, closed])`, `Path.Svg(source [, fitSize])` (an `.svg` file, SVG text, or path data like `"M0 0 C ..."`; returns `nil, message` on failure), `Path.Text(text, size [, align] [, fontPath])` (the glyph outlines of `text`, `align` is `"left"`, `"center"` or `"right"`, laid out like `Draw.Text`: `x, y` is the top of the first line; Greek letters and math symbols work when the font has them; returns `nil, message` when nothing is visible).

Text as a path is how "write it on" effects work: put it in a Vector Shape with a stroke and animate `trimEnd` from 0 to 1, then fade the fill in. Each contour is one closed outline of a glyph (a letter with a hole has two), and morphing one text path into another works like any other morph.

Builders return the path: `MoveTo`, `LineTo`, `QuadTo`, `CubicTo`, `ArcTo(cx, cy, r, a0, a1)`, `Close`, `Append(other)`, `Translate`, `Scale`, `Rotate`. Queries: `Copy()`, `GetBounds()`, `GetLength()`, `GetPointAt(t)`, `IsEmpty()`, `ContourCount()`. Y points up, like the rest of the engine (SVG files are flipped and centered on import).

## Custom painting (`Draw` global)

Inside a script's `OnRender` only (it errors elsewhere; `Draw.MeasureText` and `Draw.Layer` also work outside). Coordinates are world space; z defaults to the entity's Z index (`Draw.SetZ(z)` to change it). A color is `r, g, b [, a]` (0-255), a table `{r, g, b [, a]}` or a `"#rrggbb[aa]"` string; arguments that come after the color (rotation, `closed`, alignment) follow it. Also built in: `Draw.Dashed`, `Draw.Arrow`, `Draw.MeasureText`, `Draw.Layer`, and the `Color`, `Timeline`, `Ease` and extra `Math` helpers, see [SCRIPTING.md](SCRIPTING.md#built-in-helpers).

`Draw.Line(x1, y1, x2, y2, width, r, g, b [, a])`, `Draw.Circle(x, y, radius, ...)`, `Draw.Ring(x, y, radius, thickness, ...)`, `Draw.Rect(x, y, w, h, r, g, b [, a] [, rotation])`, `Draw.RoundRect(x, y, w, h, radius, ...)`, `Draw.Polyline(points, width, r, g, b [, a] [, closed])`, `Draw.Polygon(points, r, g, b [, a])`, `Draw.Bezier(x0, y0, c1x, c1y, c2x, c2y, x1, y1, width, ...)`, `Draw.Path(path, x, y [, {fill = {r,g,b,a} or false, stroke = {r,g,b,a}, width, cap, join, rotation, scale, trim = {s, e, o}}])`, `Draw.Text(text, x, y, size, r, g, b [, a] [, "left"|"center"|"right"])`.

## Effects and easing gallery (editor)

View → Effects and Easing opens a gallery like easings.net. The **Effects** tab shows all premade effects ([EFFECTS.md](EFFECTS.md): fade, slide, pop, drop, spin, shake, pulse, bounce, tada, float and more) as live looping previews; click one for a large preview with Duration, Distance, Amount, Delay and Loop sliders, then copy the one-liner (`Effect.PopIn(self.entity)`), the call with your options, or the expanded `Tween.*` code. Double-click a card to copy its one-liner. The **Easing** tab shows every curve as a graph with a travelling ball, and copies the curve name, a `Tween.To` call, `Ease.<name>(t)` or `Curves.Evaluate`. Both tabs have a search box.

## Tween Lab (editor)

View → Tween Lab opens a live tween editor next to the Viewport. Pick a shape and its style, turn on what should move (move, scale, rotate, fade, fill color, draw on, erase, morph, follow a path), then pick the curve, duration, delay, repeat and yoyo, or switch to a spring. The preview plays with the real tween engine and shows a trail of positions and the curve with a moving marker. Hovering a curve in the list previews it. Play, pause, restart, reverse and scrub from the bar under the preview. The Lua for the current setup is shown below it; **Copy Lua** puts it on the clipboard.

## Engine side

- `Curve` (`Animation/Curves.h`): evaluation and the named table.
- `TweenSystem` (`Animation/Tween.h`), one per `Scene` (`scene.GetTweens()`): nodes (property, value, motion, group, callback, spring), timeline rendering that handles reverse, yoyo and repeats in nested groups, overwrite of the same property, reference counting for handles. `Read/Write(entity, property, values)` is the property table.
- `VectorPath` (`Vector/VectorPath.h`): contours of nodes with in/out handles, builders, nanosvg import, flattening, resampling, trim, dash, morph pairing.
- `VectorTessellator`: ear-clipped fills with hole bridging and 1-pixel feathered edges, strokes with joins and caps; produces a `VectorMesh`.
- `VectorShapes` + `VectorSystem`: cache per component (keyed by a hash of the path and style), world transform, `Renderer::DrawTriangles` (new `PrimitiveMode::Mesh` in the batch renderer: triangles written as degenerate quads with vertex colors).

## Limits

- One fill style and one stroke style per shape; use several entities for multi-colored artwork (SVG import keeps the first shape's colors).
- Linear gradients only.
- Self-intersecting fills are filled per contour, not by an even-odd rule.
- Translucent strokes overlap slightly at sharp corners.
- Paths can't be edited with handles in the viewport yet; use presets, SVG files or Lua.
