# SIMP Motion: explainer videos made by an agent, through MCP

Status: design, not built yet. Phase 0 (renderer work) is done; everything after it is the plan.

> **Direction changed:** the product is now a manim-like Lua library, and the MCP is deferred. See [MOTION_LUA_PLAN.md](MOTION_LUA_PLAN.md). The JSON document (section 4), the MCP tools (section 10) and the token estimates built on them (section 12) are replaced by that plan. Sections 7 (rendering pipeline), 8 (math typesetting) and 9 (narration) still apply and are referenced from it.

SIMP Motion is a 2D math and explainer animation tool, in the spirit of manim, built on SIMPEngine's renderer, tween system and vector shapes. It is driven by an AI agent through an MCP server. The agent describes a video as a small document of objects and timed beats; the engine lays it out, checks it, previews it and renders it to MP4.

## 0. The short version

- **What:** one executable, `SIMPMotion`, that is both a CLI (`SIMPMotion render explainer.motion.json`) and an MCP server (`SIMPMotion mcp`). The agent never writes C++ or Python. It sends a compact JSON document (objects, layout relations, beats of animations, narration) and gets back short text reports.
- **Why this engine:** SIMPEngine already has the hard parts of a manim-style renderer: anti-aliased vector paths with trim, dash and morph, a Flutter-grade tween timeline (sequence, parallel, stagger, springs, seek, reverse), cameras with smooth follow, zoom and shake, glow, bloom and lighting, particles, SDF text, and Lua. What's missing is the "video" layer: a deterministic timeline, offline rendering, math typesetting, layout, and an agent-facing interface.
- **Will it use fewer tokens than an agent writing manim code? Yes, if it's designed for it, which this plan is.** Estimate: about 2 to 3 times fewer tokens for a finished 2-minute video. The savings mostly don't come from the first draft, which costs about the same either way. They come from the fix-and-check loop: text reports instead of screenshots, small patches instead of rewritten files, and no Python tracebacks or LaTeX logs to read. Designed naively (one tool call per object, full state in every reply, a screenshot after every step), it would cost *more* than manim. Section 12 has the numbers and the rules that keep it cheap.

## 1. Goals and non-goals

Goals

1. An agent (Claude Code, Claude Desktop, any MCP client) can produce a finished, narrated explainer video from a prompt like "explain why the derivative of sin is cos, 90 seconds".
2. Output quality at least matching manim's: crisp vector strokes at 1080p and 4K, smooth easing, LaTeX-quality math, readable layout.
3. Deterministic: the same document always renders the same frames, so the agent can reason about a frame without looking at it.
4. Cheap to drive: a typical edit costs a couple of hundred tokens in and out.
5. A human can open the same document in the SIMPEditor, scrub the timeline, tweak it, and hand it back to the agent.

Non-goals (for now)

- 3D scenes (manim's `ThreeDScene`). The engine is 2D; fake 3D with projections in Lua if needed.
- A general video editor (cuts, footage, color grading). We output a clean MP4 plus audio; editing happens elsewhere.
- Real-time collaboration between several agents on one document.

## 2. What the engine already has, mapped to manim

| manim concept | SIMPEngine today | Gap |
|---|---|---|
| `VMobject` (Bézier paths, fill + stroke) | `VectorPath` + `VectorShapeComponent` + `VectorTessellator` (fills with holes, joins, caps, 1-px AA fringe, linear gradients) | Radial gradients, even-odd fill, per-subpath styles |
| `Create` (draw the stroke on) | `trimStart/trimEnd/trimOffset` tweenable | Per-contour vs. whole-path ordering option (see 5.2) |
| `Transform` / `ReplacementTransform` | `BuildMorphPairs/BlendMorph`, `morph` property, `MorphTo` | Matching by glyph for text (5.4) |
| `Text`, `MarkupText` | SDF text (`Renderer::DrawText`) and, as of Phase 0, **text as vector outlines** (`BuildTextOutline`, `Path.Text`) with per-glyph contour ranges | Rich text spans (bold/color runs) |
| `MathTex`, `Tex` | none | TeX provider (section 8) |
| `animate.shift/scale/rotate`, `FadeIn/Out` | `TweenSystem` property tweens on x, y, scale, rotation, alpha, colors, stroke width | none |
| `LaggedStart`, `AnimationGroup`, `Succession` | `Tween.Stagger`, `Tween.Parallel`, `Tween.Sequence` | none |
| `rate_func`s | every Flutter and Material curve, `Cubic`, `Interval`, `Steps`, `Elastic`, `Spring` | none |
| `MoveAlongPath` | `Tween.Along` (orient, relative) | none |
| `ValueTracker` + updaters | `Tween.Value` + Lua `OnUpdate/OnRender` | Declarative bindings (5.6) |
| `MovingCameraScene`, `camera.frame.animate` | `CameraComponent` zoom, follow, bounds, shake; `zoom` is tweenable | "Frame this object" helper (5.7) |
| `Axes`, `NumberLine`, `FunctionGraph`, `NumberPlane` | `Draw.*` primitives, `Path.Points` | Library objects (4.3) |
| `Brace`, `SurroundingRectangle`, `Arrow`, `DashedLine` | dashes, caps, `Path.*` builders | Library objects |
| Glow, flashes | Glow component, bloom, lighting, particles | none; these go beyond manim |
| Rendering to file | Phase 0: MSAA `Framebuffer`, `ReadPixels`, `ImageWriter::WritePng` | Encoder, deterministic clock (section 7) |
| Sections and partial renders | none | Beats are the unit of partial renders (6.4) |
| Voiceover (manim-voiceover plugin) | `AudioEngine` (WAV, OGG), `Music` | TTS provider + timing sync (section 9) |

Phase 0 (already done, in this commit):

- `Framebuffer(w, h, hdr, samples)` with multisampling, `Resolve()`, and `ReadPixels()` (top-down RGBA8). Bloom works on MSAA targets.
- `ImageWriter::EncodePng/WritePng` (vendored stb_image_write).
- Fonts bake glyphs on first use, so Greek letters, math operators and arrows draw. Before this, only Latin-1 was baked and everything else showed as `?`. The atlas grows to 4096 rows, and fonts load without a GL context, so tests and headless tools can lay out text.
- `BuildTextOutline` and Lua `Path.Text`: a string's glyph outlines as one `VectorPath`, with the contour range, codepoint, string index, line and bounds of each glyph. This is the foundation for `Write`, per-letter staggering and text morphs.

## 3. Architecture

```
 Agent (Claude Code / Desktop / any MCP client)
   │  JSON-RPC 2.0 over stdio (MCP)
   ▼
 SIMPMotion mcp ──────────────────────────────────────────────────────────────┐
 │ McpServer        tools/list, tools/call, resources/*, prompts/*,           │
 │                  progress notifications; logs to stderr only               │
 │ DocumentStore    the .motion.json documents, op log (undo), autosave       │
 │ Compiler         document → objects → entities, layout solve, timeline     │
 │   ├ ObjectLibrary  text, tex, shapes, arrow, brace, axes, graph, plane...  │
 │   ├ Layout         relations (next_to, align, arrange, grid, edges)        │
 │   └ Timeline       beats → absolute times → TweenSystem nodes             │
 │ Checker          layout, timing and readability lint → short text report   │
 │ Player           fixed-step clock, seek (snapshots), deterministic RNG     │
 │ FrameRenderer    offscreen MSAA Framebuffer → PBO readback                 │
 │ Encoder          ffmpeg subprocess (rawvideo pipe) or PNG sequence         │
 │ Providers        TeX (latex+dvisvgm / MathJax), TTS, fonts, SVG cache      │
 │ PreviewLink      optional: live window, or push to a running SIMPEditor    │
 └────────────────────────────────────────────────────────────────────────────┘
        │ uses
        ▼
 SIMPEngine: Scene (EnTT), TweenSystem, VectorShapes, Renderer, Font, Lua, Audio
```

Decisions and why:

1. **The MCP server lives in the C++ binary**, not in a Python or Node wrapper. The document, layout, checker and renderer have to be in the same process anyway (the checker needs real text metrics and real path bounds). JSON-RPC over stdio is small, and `vendor/json` (nlohmann) is already vendored. One binary is also easier to ship (CPack already packages `bin/` and `lib/`).
2. **The agent edits a document, not the engine.** Tools take and return document operations. The scene in the ECS is rebuilt from the document, either whole or incrementally per object. So the document is the single source of truth: it can be saved, diffed, opened in the editor, and re-rendered later without the agent.
3. **Lua is the escape hatch, not the main interface.** Anything the object library can't express goes into a `lua` object (a script with `OnRender`/`OnUpdate`, running the existing bindings). That keeps the declarative surface small and the token cost low, without capping what can be drawn.
4. **Rendering is offline and deterministic.** Time comes from the frame index, never from a wall clock. Live preview uses the same `Player` at real time.

New code lives in `Engine/Engine/Motion/` (library code, unit-testable headless) and `Apps/SIMPMotion/` (the executable: CLI + MCP). Details in section 14.

## 4. The Motion document

The document is the core idea of the whole design, so it gets the most detail.

### 4.1 Shape

```json
{
  "motion": 1,
  "video": { "size": [1920, 1080], "fps": 60, "theme": "chalk", "background": "#101318" },
  "styles": { "hl": { "color": "yellow", "stroke": 4 } },
  "objects": [ ... ],
  "beats": [ ... ]
}
```

Coordinate system: the frame is **16 units wide and 9 tall**, the origin is at the center, and y points up. Small numbers cost fewer tokens than pixels (`[2.5, -1]` vs `[400, -120]`) and match how people sketch a layout. The camera maps frame units to pixels for whatever output size is chosen, so the same document renders at 720p, 1080p and 4K with identical composition.

Colors: names from the theme palette (`"blue"`, `"accent"`, `"muted"`), `#rrggbb[aa]`, or `[r, g, b, a]` in 0 to 1.

### 4.2 Objects

Every object has an `id` (short, chosen by the agent, unique), a type key, and optional `style`, placement, `z`, `hidden`.

The type key doubles as the main argument, which keeps objects short:

```json
{ "id": "title", "text": "Why sin' = cos", "style": "title", "at": "top" }
{ "id": "eq",    "tex": "\\frac{d}{dx}\\sin x = \\cos x", "at": [0, -2.5] }
{ "id": "c",     "circle": 1.2, "fill": "blue", "at": "center" }
{ "id": "arr",   "arrow": ["c", "eq"], "bend": 0.3 }
```

Core types (Phase 1 to 3):

| Type | Main argument | Notable options | Built from |
|---|---|---|---|
| `text` | string | `size`, `font`, `align`, `width` (wrap), `weight` | `BuildTextOutline` → Vector entities, one per glyph (4.5) |
| `tex` | TeX string | `size`, `color_map: {"x": "blue"}` | TeX provider → SVG → `VectorPath` per glyph |
| `circle`, `ellipse`, `rect`, `square`, `polygon`, `star`, `line`, `dot`, `arc` | size / points | `fill`, `stroke`, `width`, `dash`, `corner` | `VectorPath` builders |
| `path` | SVG path data | as above | `LoadSvgPathData` |
| `svg` | file | `fit` | `LoadSvgFile` |
| `image` | file | `fit` | Sprite |
| `arrow` | `[from, to]` (ids or points) | `bend`, `tip`, `double`, `gap` | path + tip; re-solved when ends move |
| `brace` | target id | `side`, `label` | computed path |
| `box` | target id | `pad`, `corner` | `SurroundingRectangle` |
| `group` | list of ids | `arrange` | Hierarchy parent |
| `lua` | script path or inline source | `props` | `ScriptComponent::BindLua` |

Library types (Phase 4):

| Type | Example | Notes |
|---|---|---|
| `axes` | `{"axes": {"x": [-4, 4, 1], "y": [-2, 2, 1]}, "labels": ["x", "y"]}` | ticks, numbers, grid; exposes `c2p` for children |
| `graph` | `{"graph": "sin(x)", "on": "ax", "x": [-4, 4], "color": "blue"}` | expression evaluated in a Lua sandbox (`math.*` only), adaptive sampling, discontinuity breaks |
| `numberline` | `{"numberline": [-3, 3, 1]}` | |
| `plane` | `{"plane": {"x": [-8, 8], "y": [-4.5, 4.5]}}` | NumberPlane with faded sub-grid |
| `vector` | `{"vector": [2, 1], "on": "plane"}` | arrow from origin in plane coordinates |
| `matrix`, `table` | `{"matrix": [[1, 2], [3, 4]]}` | cells addressable as `m[1][2]` |
| `code` | `{"code": "def f(x): ...", "lang": "python"}` | reuses the script editor's lexer and VS Code themes |
| `counter` | `{"counter": 0, "format": "{:.2f}"}` | a number that can be tweened |
| `area` | `{"area": "g", "x": [0, 1]}` | region under a graph |
| `riemann` | `{"riemann": "g", "x": [0, 2], "n": 8}` | |

### 4.3 Addressing parts

Agents need to point at pieces without new ids:

- `eq[3]`, `eq[2:5]`: glyphs by index (visible glyphs only, in reading order).
- `eq{"\\cos x"}`: the glyphs that came from a TeX substring (section 8.3), or a substring of plain text.
- `ax.x`, `ax.y`, `ax.labels`, `ax.p(2, 1)`: named parts and points of library objects.
- `tri.edge(1)`, `tri.vertex(0)`, `c.top`, `c.left`, `c.center`: geometric anchors on any object (from the outline).
- `g.at(1.5)`: the point on a graph at x = 1.5. `path.t(0.25)`: the point a quarter of the way along a path.

Anchors are usable anywhere a point is expected: `"at": "c.right"`, `"arrow": ["eq{\\cos x}", "g.at(0)"]`.

### 4.4 Layout

Agents are bad at absolute coordinates and good at relations, so layout is relational and solved by the engine:

| Key | Meaning |
|---|---|
| `at` | a point, anchor, or a named place: `center`, `top`, `bottom`, `left`, `right`, `top_left`... (inside the safe area) |
| `next_to` | `["eq", "down", 0.3]`: beside another object, with a direction and gap |
| `align` | `["eq", "left"]`: share an edge or center line with another object |
| `arrange` | on a group: `{"dir": "row", "gap": 0.4, "align": "center"}` |
| `grid` | on a group: `{"cols": 3, "gap": [0.5, 0.4]}` |
| `fit` | `{"width": 6}` or `{"in": "box1", "pad": 0.2}`: scale to fit |
| `shift` | a final offset applied after the rest |

The solver runs in dependency order (a topological sort over references; cycles are reported as errors). It uses real bounds: text from `BuildTextOutline`, shapes from `VectorPath::GetBounds`. It runs once at compile time and again for objects whose dependencies move in an animation, so an arrow attached to two moving objects follows them.

The safe area is the frame minus a 0.5-unit margin (configurable). Objects placed outside it get a warning from the checker, not an error.

### 4.5 How objects become entities

- One entity per object, holding a Transform, Hierarchy and the drawable.
- Text and TeX become a parent entity plus **one child Vector entity per glyph**, built from the per-glyph contour ranges of `TextOutline`. Per-letter animation (Write, stagger, color one term, transform matching) then reduces to ordinary tweens on child entities. A 40-glyph equation is 41 entities, which is nothing for EnTT.
- Groups are parents. Library objects are parents with named children (`ax.x` is a child entity).
- The entity's `TagComponent` is the document id, so the editor's Scene panel shows the document structure, and Lua can `Scene.Find("eq")`.

### 4.6 The timeline: beats

A video is a list of beats. A beat is "say this, while doing these things":

```json
{ "say": "Differentiate both sides.", "play": [["write", "eq2"], ["indicate", "eq{\\cos x}"]], "lag": 0.2 }
{ "play": [["fade_out", "title"]], "time": 0.5 }
{ "wait": 1.5 }
{ "mark": "proof" }
```

- `play`: a list of animations. Each is `[verb, target, options?]`, which is about 40% shorter than `{"verb": ..., "target": ...}`.
- Animations in one beat run in parallel. `lag` staggers them (manim's `LaggedStart`). `"seq": true` chains them.
- The beat duration is `time` if given, otherwise the longest animation, extended to fit the narration (`say`) when audio is on (section 9).
- `wait` holds. `mark` names a point in time, for partial renders and editor bookmarks.
- Absolute times are computed by the engine and reported back, so the agent never does arithmetic on timestamps.

### 4.7 Animation verbs

All verbs accept `time` (seconds), `curve` (any curve name or builder), `delay`, and `lag` (for multi-target or per-glyph stagger).

| Verb | Effect | Engine mechanism |
|---|---|---|
| `create` | draw the stroke on, then fade the fill in | `trimEnd` 0→1 per contour (or whole path, `order: "path"`), then `fillColor` alpha |
| `write` | like `create`, glyph by glyph | staggered `create` on glyph children, default `lag` from glyph count |
| `uncreate`, `unwrite` | reverse of the above | same tweens reversed |
| `fade_in`, `fade_out` | alpha, optional `shift: [x, y]`, `scale` | property tweens |
| `grow` | scale from 0, `from: "center"` or an edge or point | scale tween with a temporary pivot |
| `shrink` | the reverse | |
| `move` | `to` a point or anchor, or `by` an offset; `path` for an arc | position tween or `Tween.Along` |
| `rotate`, `scale` | | property tweens |
| `color` | change fill or stroke color | `fillColor`, `strokeColor` |
| `set` | instant property change at the beat start | `TweenSystem::Write` |
| `transform` | morph A into B; A takes B's place, B stays hidden | `BuildMorphPairs` + `morph`, plus a transform tween |
| `match` | transform text or TeX keeping shared glyphs (manim's `TransformMatchingTex`) | glyph matching (5.4) |
| `indicate` | pulse scale and color briefly | spring or `Curves.Sine` |
| `circumscribe` | draw a box or ellipse around the target, then remove it | temporary `box` + trim |
| `flash` | radial lines burst | particles preset, or lines with trim |
| `wiggle` | small rotation shake | `Curves.Sine` |
| `focus` | dim everything else (spotlight) | scene lighting with a spot light, or an overlay with a hole |
| `trace` | leave a trail behind a moving point | `lua` helper or a growing polyline |
| `count` | tween a `counter` to a value | `Tween.Value` |
| `camera` | `{"frame": "eq", "pad": 0.5}` or `{"zoom": 2, "to": [1, 0]}` | camera transform and `zoom` tweens |
| `glow` | pulse a Glow component | `glowIntensity` tween |
| `lua` | call a function in a `lua` object | `ScriptEvent` or direct call |

### 4.8 Bindings (manim's updaters)

The most common updater cases can be declared without code:

```json
{ "id": "t", "counter": 0 },
{ "id": "dot", "dot": 0.08, "at": "g.at(t)" },
{ "id": "tan", "line": { "tangent": "g", "x": "t", "length": 3 } },
{ "id": "lbl", "tex": "x = {t:.2f}", "next_to": ["dot", "up"] }
```

Any option written as an expression that refers to a `counter` (or `time`) is re-evaluated every frame. Expressions run in a sandboxed Lua environment (no `io`, no `os`) and are compiled once. Text with `{counter}` placeholders reuses `GameVariables::Format`. Anything more complex goes in a `lua` object.

### 4.9 Themes and styles

A theme fixes the palette, fonts, default sizes (title 0.9 units, body 0.5, tex 0.6), stroke widths, background and the default curve. Named `styles` in the document extend it. Themes keep documents short: the agent writes `"style": "hl"` instead of five properties, every time.

Built-in themes: `chalk` (dark, 3b1b-like), `paper` (light, print-like), `neon` (dark with glow and bloom), `blueprint`.

### 4.10 A complete example

A 30-second Pythagoras explainer. The whole document is about 650 tokens:

```json
{
  "motion": 1,
  "video": { "size": [1920, 1080], "fps": 60, "theme": "chalk" },
  "objects": [
    { "id": "title", "text": "a² + b² = c²", "style": "title", "at": "top" },
    { "id": "tri", "polygon": [[-2, -1.5], [2, -1.5], [-2, 1.5]], "stroke": "white", "at": [0, -0.5] },
    { "id": "a", "tex": "a", "next_to": ["tri.edge(2)", "left", 0.2] },
    { "id": "b", "tex": "b", "next_to": ["tri.edge(0)", "down", 0.2] },
    { "id": "c", "tex": "c", "next_to": ["tri.edge(1)", "up_right", 0.2] },
    { "id": "sa", "square_on": "tri.edge(2)", "outside": true, "fill": "blue" },
    { "id": "sb", "square_on": "tri.edge(0)", "outside": true, "fill": "green" },
    { "id": "sc", "square_on": "tri.edge(1)", "outside": true, "fill": "red" },
    { "id": "eq", "tex": "a^2 + b^2 = c^2", "at": "bottom", "size": 0.8 }
  ],
  "beats": [
    { "say": "Take any right triangle.", "play": [["write", "title"], ["create", "tri"]] },
    { "say": "Call the short sides a and b, and the long side c.", "play": [["write", ["a", "b", "c"]]], "lag": 0.3 },
    { "say": "Now build a square on every side.", "play": [["grow", "sa"], ["grow", "sb"], ["grow", "sc"]], "lag": 0.4 },
    { "say": "The two small squares hold exactly as much area as the big one.", "play": [["indicate", ["sa", "sb"]], ["indicate", "sc", { "delay": 1 }]] },
    { "say": "And that's the theorem.", "play": [["match", ["a", "b", "c"], "eq"], ["camera", { "frame": "eq", "pad": 1 }]] },
    { "wait": 1 }
  ]
}
```

(`square_on` is a library shape: the square erected on a segment.)

## 5. Animation semantics in detail

### 5.1 Compile to the existing TweenSystem

Every beat compiles into one `Tween.Parallel` (or `Sequence` when `seq` is set) of property, motion and value nodes. Beats are chained into one root `Sequence`, with each beat's narration padding added as a `Delay`. The whole video is then **one tween tree**, and `TweenSystem::Seek(root, t / total)` puts every property where it belongs at any time. That is the reason for building on the tween system and not writing a second animation engine.

Two things need care:

- **Implicit overwrite.** `TweenSystem::Overwrite` kills an older tween on the same entity and property. That's right for games, but on a seekable timeline it destroys history. The Motion compiler marks its nodes as timeline-owned, and overwrite is skipped for nodes inside the same root. This is a small flag on `TweenNode`, plus a branch in `Overwrite`.
- **From-values.** Property tweens capture their start value when they first run (`Captured`). For seeking backwards and for partial renders, every Motion tween is compiled with an explicit `From`, computed by simulating the document's property state beat by beat at compile time. The compiler keeps a "property ledger" per entity for this. It also produces the timeline summary the checker uses.

### 5.2 Create and Write

- `create` on a path: two child tweens: `trimEnd` 0→1 (stroke), then `fillColor.a` 0→target, overlapping by 20% like manim. If the object has no stroke, a temporary stroke in the fill color, width 2 px equivalent, is drawn during the trim and faded out with the fill fade-in.
- Trim today applies per contour (every contour draws at once, which is what Phase 0's "Write me" test frame shows). Manim draws the contours of one glyph together but glyphs in sequence. `write` gets that by staggering glyph children; an `order: "path"` option on `create` (trim along the concatenated length of all contours) is a small addition to `StrokeContours` in `VectorShape.cpp`.
- Default timing: `write` takes `clamp(0.6 + 0.05 * glyphs, 0.8, 3)` seconds, and `lag` spreads glyph starts across 60% of it.

### 5.3 Transform

`transform A B`: A's outline morphs into B's (existing `BuildMorphPairs`: arc-length matching with best rotation, extra contours grow from the center), while A's transform tweens to B's and its fill and stroke colors tween to B's. When it finishes, A is hidden and B shown (manim's `ReplacementTransform`), unless `keep: "a"`.

For text and TeX, the morph runs per glyph pair, not on the whole outline, which looks much better than one big blob.

### 5.4 Glyph matching (`match`)

Given glyph lists `A` and `B`, each glyph with a key:

1. Key = the TeX source token for TeX (section 8.3), or the codepoint for plain text.
2. Longest common subsequence over keys gives pairs that keep reading order. Leftover glyphs with equal keys are paired greedily by nearest position (so `a+b` → `b+a` swaps).
3. Paired glyphs: move, scale and morph to their target glyph (usually identical shapes, so effectively a move).
4. Unpaired glyphs in A fade out with a small shift toward their nearest paired neighbor's motion; unpaired glyphs in B fade in.
5. Options: `map: {"a": "x"}` forces pairs, `fade_unmatched: false` morphs leftovers instead.

This is all CPU code on glyph metadata, so it's unit-testable headless.

### 5.5 Indicate, circumscribe, flash and focus

These are short compound effects that leave nothing behind. Each compiles to a small group: temporary entities created at compile time, hidden outside the effect's time span, plus tweens. Temporary entities belong to the beat, so seeking outside it shows them hidden.

### 5.6 Determinism rules

- Time is `frame / fps`. The `Player` calls `Scene::OnUpdate(1 / fps)` per frame. Physics and particles step with that fixed dt, and the RNG used by particles and `Spawner` is reseeded from `video.seed` at t = 0.
- Seeking: tweens seek directly. Systems with their own state (particles, physics, `lua` objects) re-simulate from the nearest snapshot. The `Player` stores a `SceneSerializer` snapshot plus particle state every N seconds of simulated time (default 2 s), and scrubbing to t re-simulates at most 2 s.
- `lua` objects get `Time.Now()` from the `Player`, never from SDL ticks.

### 5.7 Camera

The document has one camera, the primary `CameraComponent`, with `follow` off and manual control off. `camera` verbs:

- `{"frame": target, "pad": 0.5}`: pan and zoom so the target's bounds fill the frame with padding (zoom = min(16 / w, 9 / h) after padding).
- `{"to": [x, y], "zoom": z}`, `{"by": [dx, dy]}`, `{"shake": 0.3}`, `{"reset": true}`.
- Library objects and text keep crisp edges at any zoom, because vector AA and SDF text widths are computed from `Renderer::GetWorldPerPixel()`.

## 6. Checking: how the agent knows it looks right without looking

This is the biggest lever on both quality and token cost. A text report of ~200 tokens replaces most screenshots, which cost ~700 to 1,600 tokens each.

### 6.1 Lint rules

Run after every `apply`. Every finding is one line, with the object id and beat:

| Rule | Example output |
|---|---|
| Off frame / outside safe area | `W off-frame b2 lbl: right edge at 8.4 (frame 8.0)` |
| Overlap of text with anything | `W overlap b3 eq ∩ tri (0.42 u²)` |
| Text too small | `W small b1 note: cap height 9 px at 1080p (min 18)` |
| Low contrast | `W contrast b0 sub: 2.1:1 on background (min 4.5)` |
| Narration too fast | `W pace b4: 31 words in 6.0 s (max 3.2 words/s)` |
| Nothing changes for too long | `I idle b5: 7.5 s with no animation` |
| Reference to a hidden or missing object | `E ref b2 indicate "eqq": no such object (did you mean "eq"?)` |
| Object never shown or never removed | `I unused sq4` |
| Morph between very different contour counts | `I morph b6 c→d: 1 vs 9 contours, expect a blob` |
| Layout cycle | `E layout: a next_to b next_to a` |

Severity: E (blocks render), W (should fix), I (info). The `apply` reply includes only E and W, at most 8 lines, followed by "+N more" when truncated.

### 6.2 The layout table

`motion_check` with `at: "b3"` returns what is on screen at the end of beat 3, as one row per visible object:

```
b3 t=9.40s  visible 7
id     type  center        size        z  alpha
title  text  (0.00,3.90)   5.10×0.82   0  1.00
tri    poly  (0.00,-0.50)  4.00×3.00   0  1.00
eq     tex   (0.00,-3.70)  4.30×0.70   0  0.00  ← fading in
...
```

This answers most "where is X?" questions without an image.

### 6.3 Snapshots (images, only when needed)

`motion_snapshot` renders the requested times or beats into **one** contact-sheet PNG (default 3×3 thumbnails at 1280×720 total, so about 1,200 image tokens for 9 moments, versus ~1,600 for a single 1080p frame). Thumbnails are labeled with the beat and time. A single detail frame can be requested at a crop and size.

The agent guidance (section 10.4) tells it to snapshot once per scene after the lint is clean, not after every edit.

### 6.4 Partial renders

`motion_render` takes `beats: [3, 5]` or `from: "proof"`, `to: 12.5`. With seekable tweens and snapshots, rendering beat 5 alone costs only beat 5's frames plus up to 2 s of re-simulation.

## 7. Rendering pipeline

### 7.1 Frame loop (offline)

```
for frame in range:
    player.SeekOrStep(frame)          // fixed dt
    Renderer::BeginRenderTarget(msaa) // Framebuffer(w, h, false, 4)
    Renderer::Clear()
    scene.OnRender()                  // lighting, glow and bloom included
    msaa.Resolve()
    readback.Queue(msaa)              // async PBO, 3 in flight
    encoder.Write(readback.Oldest())  // ffmpeg stdin
```

- Resolution and fps come from `video`. Presets: `draft` (854×480, 30 fps, MSAA 2), `preview` (1280×720, 30 fps, MSAA 4), `final` (1920×1080, 60 fps, MSAA 8), `4k`.
- **Readback without stalls:** three `GL_PIXEL_PACK_BUFFER`s in rotation. Frame n is read into PBO n % 3 with `glReadPixels` into the buffer (async), and frame n − 2 is mapped and sent to the encoder. `Framebuffer::ReadPixels` (Phase 0) is the simple synchronous version, used for snapshots.
- The vertical flip is done by ffmpeg (`-vf vflip`) on the video path, so the rows aren't copied twice.
- Optional **motion blur**: render k subframes at t + i / (k·fps) and average them in an RGBA16F accumulation buffer (cheap given the 2D workload).
- Optional **supersampling** (`ssaa: 2`) for print-quality stills: render at 2× into a normal framebuffer and downsample with a box filter. Note: hairlines specified in pixels get thinner. The Motion object library specifies widths in frame units, so it isn't affected.

### 7.2 Encoder

ffmpeg as a subprocess, never linked, so there's no GPL or LGPL linking question and nothing is added to the build:

```
ffmpeg -y -f rawvideo -pix_fmt rgba -s 1920x1080 -r 60 -i pipe:0 \
       [-i narration.wav] -vf vflip -c:v libx264 -preset slow -crf 16 \
       -pix_fmt yuv420p -movflags +faststart [-c:a aac -b:a 192k -shortest] out.mp4
```

- Launched with `SDL_CreateProcessWithProperties` (stdin as a pipe). The editor already wraps `SDL_CreateProcess` in `Editor/UI/FileDialogs`.
- Formats: `mp4` (H.264), `webm` (VP9 with alpha optional), `mov` (ProRes 4444 for editors, with alpha), `gif` (palettegen two-pass), `png` (image sequence through `ImageWriter`, no ffmpeg needed).
- If ffmpeg is missing, the render falls back to a PNG sequence and says so in the result.
- Captions: an `.srt` file next to the video, from the `say` texts and beat times.

### 7.3 Headless

- Desktop: SDL hidden window (`SDL_WINDOW_HIDDEN`), which is what the Phase 0 check program used. It works on X11, Wayland and Windows.
- Servers and CI: `SDL_VIDEO_DRIVER=offscreen` (SDL3 offscreen driver with EGL), or Xvfb. `Window::Init` already tries GL 4.6 down to 4.0. Mesa llvmpipe works for CI renders (slow but correct).

### 7.4 Performance targets

A 1080p60 scene with ~2,000 vector entities, text and glow should render faster than real time on a mid-range GPU. Costs to watch:

- Tessellation cache misses during morphs and trims: `VectorShapes` caches per component by path and style hash, but a morph changes the outline every frame. Per-glyph morphs keep each mesh small. If profiling shows it matters, add a "trim only" fast path that reuses the flattened outline and only re-strokes.
- Readback: with PBOs it overlaps the next frame's rendering.
- Encoding: x264 `slow` at 1080p60 can be the bottleneck. Use `-preset veryfast` for previews.

## 8. Math typesetting

### 8.1 Providers

`TexProvider` is an interface: `bool Render(const std::string &tex, TexOptions, TexResult &out)`, where `TexResult` is SVG text plus a glyph-to-source map. Implementations, tried in order:

1. **`latex` + `dvisvgm`** (what manim uses): best fidelity, full LaTeX packages. Needs a TeX install. Run as a subprocess in a temp directory with a template preamble (`amsmath`, `amssymb`), then `dvisvgm --no-fonts --exact-bbox`.
2. **MathJax** (Node, `mathjax-full` `tex2svg`): no TeX install, covers common math well, outputs paths with `data-c` glyph codes. Run as a small persistent Node process speaking line-delimited JSON (one process, not one per formula).
3. **Unicode fallback:** when neither is available, a small converter turns simple TeX (`^`, `_`, `\frac`, Greek, operators, `\sqrt`) into laid-out Unicode text through `BuildTextOutline` and a tiny box layout (superscripts at 0.7 scale and raised). This works out of the box thanks to Phase 0's lazy glyphs, and it's good enough for `a^2 + b^2 = c^2`, `\sin x`, and `\frac{dy}{dx}`.

Results are cached by hash of (provider, preamble, source) in `<project>/.motion-cache/tex/<hash>.svg`, so re-rendering costs nothing.

### 8.2 SVG import gaps

`VectorPath::LoadSvgText` uses nanosvg, which does not resolve `<use>` and `<defs>`. dvisvgm and MathJax both emit glyphs as `<defs><path id>` plus `<use href x y>`. A small pre-pass (`SvgUseExpander`) inlines `<use>` elements as transformed `<path>`s before nanosvg sees them. That's about 150 lines of XML-ish string handling; no new dependency is needed for this subset.

The importer also has to return **one `VectorPath` per glyph** (per inlined `<use>`), not one merged path. That means a `LoadSvgGlyphs` variant that keeps shape boundaries, which nanosvg already exposes as its shape list.

### 8.3 Mapping glyphs back to TeX source

For `eq{"\\cos x"}` and `match`:

- MathJax: each glyph `<use>` carries `data-c` (the Unicode codepoint) and sits inside `<g data-mml-node>` groups, so the TeX token can be recovered by walking the MathML tree in order.
- dvisvgm: no metadata. Use manim's trick: render the formula once, then once per requested substring wrapped in `\color{...}` with a unique color, and read which glyphs changed color. Only done for substrings the document actually references, and cached.
- Unicode fallback: exact, since the layout is ours.

## 9. Narration and audio

- `say` text per beat. With `audio.tts` configured, each `say` is synthesized to a WAV clip and its duration drives the beat length: `beat = max(animations, speech + 0.3 s)`. Without TTS, the duration is estimated at 2.6 words/s, and the checker uses the same estimate for the pace rule.
- `TtsProvider` interface with implementations: **Piper** (local, free, a subprocess), **OS voices** (`say` on macOS, SAPI on Windows, `espeak-ng`), and a **command template** for any cloud TTS (`"cmd": "my-tts --out {out} --text {text}"`). Cloud API keys never enter the document.
- Word timings (Piper and most cloud TTS can return them) allow `{"at_word": "triangle"}` on an animation, starting it when that word is spoken. That's the "voiceover-synced" feel manim-voiceover gives.
- Audio is mixed offline (narration clips at beat starts, an optional music bed with ducking) into one WAV that ffmpeg muxes. The real-time `AudioEngine` is used only for live preview.
- User-recorded narration: `"say": {"file": "assets://vo/b3.wav", "text": "..."}`. Its length drives the beat the same way.

## 10. The MCP surface

### 10.1 Transport and protocol

- `SIMPMotion mcp [--project DIR] [--preview]`: JSON-RPC 2.0 over stdio, newline-delimited, implementing MCP's `initialize`, `tools/list`, `tools/call`, `resources/list`, `resources/read`, `prompts/list`, `prompts/get`, and `notifications/progress` for renders. Negotiate the protocol version in `initialize`, and target the current MCP spec revision when Phase 2 starts.
- **stdout carries only protocol messages.** All logging goes to stderr: the spdlog sinks are reconfigured in `mcp` mode, and `Log::Init` gets a "stderr only" switch.
- One document is active per session, plus a project folder (`<project>/motion/*.motion.json`, renders in `<project>/renders/`).
- Register with Claude Code: `claude mcp add simp-motion -- /path/to/SIMPMotion mcp --project ~/videos/derivatives`

### 10.2 Tools

Nine tools. Short descriptions, with the details in resources the agent reads only when needed. Each tool's schema is kept under ~250 tokens, so the whole tool list costs about 2,000 tokens of (cached) context.

| Tool | Input | Returns (typical size) |
|---|---|---|
| `motion_open` | `name`, optional `video` settings and `theme`; opens or creates | summary: objects, beats, duration, last lint (~80 tokens) |
| `motion_apply` | `ops`: a list of `add`, `set`, `remove`, `beat`, `edit_beat`, `move_beat`, `delete_beat`, `style`, `video` | per-op `ok` or error, new beat times, E/W lint lines (~100–250) |
| `motion_get` | `path` (`objects.eq`, `beats[3]`, `outline`) | that slice of the document (only as big as asked) |
| `motion_check` | optional `at` (beat or time) | lint + layout table (~150–400) |
| `motion_snapshot` | `at`: list of beats or times, optional `size`, `crop` | one PNG contact sheet (~700–1,500 image tokens) |
| `motion_render` | `quality`, optional `beats` / `from` / `to`, `format` | job id; progress notifications; final path, size, duration |
| `motion_status` | `job` | progress or result (~40) |
| `motion_catalog` | `topic` (`verbs`, `objects`, `layout`, `themes`, `tex`, a single verb or object name) | the reference for that topic only (~200–800) |
| `motion_undo` | optional `steps` | summary |

Op format examples (what `motion_apply` receives):

```json
{"ops": [
  {"add": {"id": "eq2", "tex": "\\cos x", "next_to": ["eq", "down"]}},
  {"set": {"id": "eq", "color_map": {"x": "blue"}}},
  {"beat": {"after": 3, "say": "And here's the slope.", "play": [["write", "eq2"]]}},
  {"edit_beat": {"at": 5, "play": [["fade_out", "tri", {"shift": [0, -1]}]]}}
]}
```

`add` with an existing id is an error (use `set`), which catches typos. `set` merges keys (`null` deletes a key). Beats are addressed by index or by `mark`.

### 10.3 Resources and prompts

- `motion://reference/{topic}`: the same content as `motion_catalog`, for clients that prefer resources.
- `motion://document`: the current document (the full JSON, for clients that want to show it).
- Prompt `explainer`: a style guide for explainer videos, loaded by the client on demand. One idea per beat, introduce before transforming, keep under 2.5 words/s, reuse positions, color-code variables consistently, end each section on a still frame, and so on.

### 10.4 Agent workflow (the guidance shipped in the prompt and tool descriptions)

1. `motion_open`, then plan the beats in prose (no tool call).
2. One `motion_apply` per scene (5 to 10 beats): objects plus beats together.
3. Fix E and W lint lines with small `set` / `edit_beat` ops.
4. Once a scene's lint is clean, one `motion_snapshot` of its beats; fix what looks wrong.
5. `motion_render quality=preview` for the full video, then `final`.

Never resend whole objects to change one key. Never call `motion_get` for the full document; ask for the slice.

### 10.5 A realistic session, with token counts

Estimates, assuming a 90-second video with 3 scenes and 14 beats:

| Step | Agent output | Tool result in |
|---|---|---|
| Tool list + `explainer` prompt (once, cached) | – | 2,000 + 900 |
| `motion_open` | 30 | 80 |
| Scene 1 `apply` (5 objects, 5 beats) | 900 | 180 |
| 3 lint fixes | 3 × 80 | 3 × 120 |
| Scene 1 snapshot | 40 | 1,200 |
| Scenes 2 and 3 (same pattern) | 2 × 1,300 | 2 × 1,700 |
| Render preview + final | 60 | 200 |
| **Total** | **≈ 3,900** | **≈ 9,200** (of which ~2,900 is cached prompt) |

## 11. Editor integration

- **Motion panel** (`Editor/Panels/MotionPanel`): opens a `.motion.json`, shows the beat list with narration, a timeline with beat blocks and animation bars, and a scrubber. The Viewport shows the scene at the scrubbed time through the `Player`, and Play runs it at real time with audio.
- **Live link:** `SIMPMotion mcp --preview` either opens its own preview window, or, when an editor with the Motion panel is open, connects to it over a local socket (`127.0.0.1`, random port in a lock file in the SDL pref folder). Then you watch the agent build the video in the editor as it goes.
- **Round trip:** editing in the panel (dragging an object, changing a beat's duration, editing the text of `say`) writes ops into the same document. The MCP server sees the file change, reloads, and its next `apply` reply starts with `doc changed outside: 3 ops by the user`, so the agent doesn't overwrite your tweaks.
- **Export to a game scene:** `motion_export format=scene` writes a normal `.yaml` scene plus a Lua timeline driver, so an explainer can become an interactive tutorial inside a game.

## 12. Token economics: will it be cheaper than manim?

### 12.1 Where an agent's tokens go when it makes a manim video

1. Writing the script: 300–600 lines of Python for 2 minutes, about 4–9k output tokens.
2. API mistakes: manim CE vs. ManimGL naming differences, and changed signatures across versions. Each costs a traceback (1–3k input) plus a fix (0.5–2k output). Typically 2–5 rounds.
3. LaTeX errors: a log excerpt plus a retry, 1–3 rounds.
4. Layout problems the code can't reveal: overlaps, off-screen labels, text too small. The agent has to render and look at frames: ~1,600 input tokens per 1080p frame at full size, and several frames per scene.
5. Edits: the agent often rewrites whole scene classes (hundreds of tokens each) to change a few lines.

Rough total for a 2-minute video: **35–50k input** (new, non-cached) and **12–18k output**.

### 12.2 The same video through SIMP Motion

From the 10.5 table, scaled to 2 minutes: **about 12–16k input** (3k of it cached) and **5–7k output**. That's roughly **2.5–3× less input and 2–2.5× less output**.

Where the saving comes from, in order of size:

1. **Text lint and layout tables instead of screenshots** (section 6): the biggest single item.
2. **Patches instead of rewrites** (`set`, `edit_beat`): an edit costs 50–150 tokens instead of 500–2,000.
3. **No runtime errors to read**: references are validated with "did you mean" hints, there's no Python, and TeX errors come back as one line.
4. **Defaults and themes**: colors, sizes and timings don't have to be spelled out.
5. **Engine-side arithmetic**: layout and timing are solved by the engine and reported back, so the agent doesn't reason about coordinates in its output.

### 12.3 When it would cost more (what this design avoids)

- One tool call per object or per animation: each call has ~50–100 tokens of envelope in and out, so 200 calls is 20k tokens of pure overhead. Hence batched `apply`.
- Replies that echo the whole document or the whole scene state: they grow with the video and get resent every turn. Hence `ok` + lint only.
- Screenshots after every step. Hence lint first, and contact sheets once per scene.
- Huge tool schemas (a full JSON schema of every object type in `tools/list`). Hence a small schema plus `motion_catalog` on demand.
- An agent that doesn't know the vocabulary and guesses: mitigated by the error hints and the `explainer` prompt.

### 12.4 Measuring it

Build a benchmark into Phase 2: five fixed prompts (Pythagoras, derivative of sin, binary search, Fourier series intro, Bayes' theorem). Run each through Claude with SIMP Motion and with manim CE in the same harness, and log input, output and cached tokens, tool calls, wall time, and a human 1–5 quality score. Keep the results in `docs/motion-benchmark.md`, and treat any regression in tokens per finished minute as a bug.

## 13. Quality bar

- Strokes: AA fringe (existing) plus MSAA 4–8 (Phase 0), so there's no shimmer on thin lines during slow zooms.
- Text: vector outlines (Phase 0) at any scale. SDF text only for UI-like overlays.
- Color: blend in linear space for the final render. That means an sRGB framebuffer option (`GL_SRGB8_ALPHA8`) and gamma-correct gradients. It's a follow-up item; manim doesn't do it either, but it shows on soft glows.
- Motion: default curve `easeInOutCubic` (close to manim's `smooth`). The default `create` timing follows manim's, so videos feel familiar.

## 14. Code layout

```
Engine/Engine/Motion/
  MotionDocument.{h,cpp}     JSON ↔ document model, ops, undo log (nlohmann json)
  MotionObjects.{h,cpp}      object library: builders from document objects to entities
  MotionLayout.{h,cpp}       relation solver, anchors, safe area
  MotionTimeline.{h,cpp}     beats → tween tree, property ledger, beat times
  MotionChecker.{h,cpp}      lint rules, layout table
  MotionPlayer.{h,cpp}       fixed-step clock, seek, snapshots
  GlyphMatch.{h,cpp}         LCS matching for match/transform
  Expression.{h,cpp}         sandboxed Lua expressions for graphs and bindings
  Tex/TexProvider.{h,cpp}    providers, cache, SvgUseExpander, Unicode fallback
  Audio/Tts.{h,cpp}          TTS providers, offline mixer
  Render/FrameRenderer.{h,cpp}  MSAA target, PBO readback ring
  Render/Encoder.{h,cpp}     ffmpeg pipe, PNG sequence, SRT
Apps/SIMPMotion/
  main.cpp                   CLI: render, check, snapshot, mcp
  McpServer.{h,cpp}          JSON-RPC, tool dispatch, progress
  Tools.cpp                  the nine tools
  Reference/*.md             catalog topics (also embedded as resources)
Editor/Panels/MotionPanel.{h,cpp}
tests/MotionTests.cpp        document ops, layout, timeline times, glyph matching, lint
tests/motion/*.motion.json   golden documents
```

Tests: everything but `FrameRenderer` and `Encoder` is headless. Golden tests compare compiled beat times, the layout table and lint output against checked-in text. A GPU smoke test (not in default CI) renders three frames of each golden document and compares them to reference PNGs with a tolerance.

## 15. Roadmap

Each phase ends with something usable.

**Phase 0: renderer (done)**
MSAA framebuffers, readback, PNG, lazy glyphs, text outlines, `Path.Text`. Verified by unit tests plus a hidden-window render of text, math symbols and vector shapes to PNG.

**Phase 1: offline video from a document (core)**
- `MotionDocument` (load, save, ops, undo), `text`/shape/`group` objects, `at`/`next_to`/`align`/`arrange` layout, beats with `create`, `write`, `fade_in/out`, `move`, `scale`, `rotate`, `color`, `transform`, `indicate`, `camera frame`.
- Timeline-owned tweens (overwrite exemption) and explicit from-values.
- `MotionPlayer` with fixed step; `FrameRenderer` with PBO ring; ffmpeg encoder; PNG fallback.
- CLI: `SIMPMotion render doc.motion.json --quality preview`, `SIMPMotion check doc.motion.json`.
- Done when: the 4.10 example (with Unicode-fallback TeX) renders to MP4 at 1080p60 faster than real time, and the golden tests pass.

**Phase 2: MCP server**
- JSON-RPC stdio, the nine tools, catalog and resources, the `explainer` prompt, progress notifications, stderr-only logging.
- Lint rules from 6.1 and the layout table; contact-sheet snapshots.
- Token benchmark harness (12.4).
- Done when: Claude Code produces the five benchmark videos from a one-line prompt without hand edits, and token use is within the 12.2 estimate.

**Phase 3: math and text**
- TeX providers (MathJax and latex+dvisvgm), `SvgUseExpander`, per-glyph import, substring mapping, `color_map`, `match`, `write` ordering options, rich text runs.
- Done when: `match` of `a^2+b^2=c^2` → `c^2-b^2=a^2` animates glyph by glyph like manim's `TransformMatchingTex`.

**Phase 4: graphs and library objects**
- `axes`, `graph` (sandboxed expressions, adaptive sampling), `numberline`, `plane`, `vector`, `matrix`, `table`, `code`, `brace`, `box`, `arrow` with attachment, `counter`, `area`, `riemann`; bindings (4.8).
- Done when: a derivative-as-slope explainer (moving tangent line and value readout) is expressible without a `lua` object.

**Phase 5: narration**
- TTS providers, beat-length sync, `at_word`, offline mix, music ducking, SRT.
- Done when: a narrated video renders with speech aligned to the animations, entirely from the document.

**Phase 6: editor**
- Motion panel, live link, round-trip edits, export to a game scene.

## 16. Risks and open questions

| Risk | Mitigation |
|---|---|
| nanosvg can't handle some TeX SVGs | `<use>` expansion pre-pass; fall back to the Unicode layout for that formula, with a warning |
| Per-frame re-tessellation during morphs is slow at 4K | per-glyph meshes, trim fast path, profile with `PROFILE_SCOPE` |
| Self-intersecting fills (the tessellator fills per contour, not even-odd) | glyph outlines don't self-intersect; add even-odd for `path` objects if needed |
| Tween overwrite semantics break seeking | timeline-owned flag (5.1); golden test that seeks backwards |
| Agents misuse the vocabulary | "did you mean" errors, catalog on demand, the `explainer` prompt, benchmark-driven tuning of descriptions |
| Headless GL on CI | `SDL_VIDEO_DRIVER=offscreen` + Mesa; GPU tests opt-in |
| ffmpeg not installed | PNG-sequence fallback; the result says how to install ffmpeg |
| The document format churns | `"motion": 1` version field; the loader upgrades older versions, like `SceneSerializer`'s `Version` |

Open questions to settle before Phase 1:

1. Text default: vector outlines for everything (best quality, more entities), or SDF for small text? Proposal: outlines by default, with `"sdf": true` per object.
2. Frame units: 16×9 fixed, or follow the output aspect ratio (for example 9×16 for vertical shorts)? Proposal: height is always 9, width follows the aspect ratio.
3. Should the document allow inline Lua strings, or only script files? Inline is more convenient for agents but harder to review. Proposal: inline allowed, saved into `<project>/motion/scripts/` on first apply.

## Appendix A: manim → SIMP Motion

| manim | SIMP Motion |
|---|---|
| `self.play(Write(t))` | `{"play": [["write", "t"]]}` |
| `self.play(Create(c), run_time=2)` | `["create", "c", {"time": 2}]` |
| `self.play(FadeIn(t, shift=UP))` | `["fade_in", "t", {"shift": [0, 1]}]` |
| `self.play(Transform(a, b))` | `["transform", "a", "b"]` |
| `self.play(TransformMatchingTex(a, b))` | `["match", "a", "b"]` |
| `self.play(LaggedStart(*anims, lag_ratio=0.2))` | beat with `"lag": 0.2` |
| `self.play(c.animate.shift(RIGHT))` | `["move", "c", {"by": [1, 0]}]` |
| `self.play(Indicate(t))` | `["indicate", "t"]` |
| `self.wait(2)` | `{"wait": 2}` |
| `t.next_to(c, DOWN)` | `"next_to": ["c", "down"]` |
| `VGroup(a, b).arrange(RIGHT, buff=0.5)` | `{"id": "g", "group": ["a", "b"], "arrange": {"dir": "row", "gap": 0.5}}` |
| `self.camera.frame.animate.scale(0.5).move_to(t)` | `["camera", {"frame": "t"}]` |
| `ValueTracker` + `always_redraw` | `counter` + expressions (4.8) |
| `axes.plot(lambda x: np.sin(x))` | `{"graph": "sin(x)", "on": "ax"}` |
| `MathTex("a", "^2")` with `set_color_by_tex` | `"color_map": {"a": "blue"}` |

## Appendix B: object schema sketch

```
Object      := { id, <type>: <main>, style?, z?, hidden?, Placement?, Paint?, ...typeOptions }
Placement   := at? | next_to: [ref, dir, gap?] | align: [ref, edge] | fit? | shift?
Paint       := fill?: Color | stroke?: Color | width?: num | dash?: [len, gap] | opacity?: num
Ref         := id | id[i] | id[i:j] | id{"substring"} | id.part | id.anchor | id.fn(args)
Beat        := { say?, play?: [Anim], lag?, seq?, time?, wait?, mark? }
Anim        := [verb, target(s), options?]
Color       := name | "#rrggbb" | "#rrggbbaa" | [r, g, b, a?]
```
