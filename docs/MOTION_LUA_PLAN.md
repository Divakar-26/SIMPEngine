# SIMP Motion: a manim-like Lua library on SIMPEngine

Status: plan, not built yet. The renderer work it depends on (MSAA framebuffers, pixel readback, PNG writing, glyphs on demand, text as vector outlines) is done. Two working videos exist: `assets/scripts/Demo.lua` (area under a curve, Lua) and `Game/NeuralNetVideo` (C++). `Game/IntegrationVideo` is a C++ port of `Demo.lua`, made to compare the two languages.

This plan replaces the JSON document and MCP design in [MOTION_MCP.md](MOTION_MCP.md). Its rendering pipeline (section 7), math typesetting (8) and narration (9) sections still apply.

## 0. Decisions

1. **The product is a library.** A manim-like Lua library (`motion`) on top of the engine's C++ renderer. You, an agent, or a chat assistant writes a video as a Lua file against it.
2. **Two goals, in order: extremely good looking, then few tokens.** The look comes from the renderer and the library's design defaults, not from the language. Tokens come mostly from the library removing boilerplate and from how fixes are made.
3. **Lua for videos, C++ underneath.** Lua for what a video says and shows. C++ for the renderer, heavy drawing primitives, recording and timing. Measured: same pixels, ~15% fewer tokens in Lua, no meaningful speed cost (section 1).
4. **A scene is a function of time.** What's on screen at time `t` is computed from `t`. Scrubbing, hot reload, partial renders and repeatable output all follow from this one rule. `Demo.lua` already works this way.
5. **Hot reload with a live preview.** Save the file and the preview shows the new version at the same moment in the video, in well under a second.
6. **No MCP for now.** Agents use a command-line tool and a short guide. Chat assistants without tools (ChatGPT, claude.ai) use a copy and paste loop the preview is built for (section 8). An MCP can come later if a need appears (section 11).
7. **Anything a game could use goes into the engine**, not the Motion module (section 3).

## 1. Evidence: Lua vs C++ on the same video

`Demo.lua` was ported line by line to C++ (`Game/IntegrationVideo/main.cpp`) and both were rendered through the same engine.

| | Lua (`Demo.lua`) | C++ (`IntegrationVideo`) |
|---|---|---|
| Look | identical | identical: 55–100 dB PSNR against Lua at 12 moments across the video (above ~40 dB is indistinguishable) |
| Size: code tokens (words, numbers, symbols) | 11,890 | 13,568 (+14%) |
| Size: characters without whitespace | 23,794 | 25,527 (+7%) |
| Scene drawing per frame, 1080p | 3.9 ms (0.04 ms of it in the script's update) | 3.4–3.6 ms |
| Scene drawing per frame, 4K | 3.9 ms (one noisy run 8.5 ms) | 3.6–4.7 ms |
| GPU work + pixel readback, 4K | 21–31 ms | 21–25 ms |
| Edit to new frame | no rebuild; today a running script needs Stop/Play to reload (fixed by E2) | 1.7 s one-file rebuild (measured) + relaunch |
| A mistake | error message, app keeps running | compile error, or a crash that ends the app |
| Typos found | only when that line runs | at compile time, everywhere |

The C++ count leaves out the C++ version's recorder and command-line code, which Lua gets from the engine.

Conclusions:

- **Looks don't depend on the language.** Both call the same C++ drawing code. Looks depend on the renderer and the design (section 6).
- **Language choice is worth ~15% of tokens.** Generic code is worth far more. Measured by region, `Demo.lua` spends about 8% (2.9 KB of 34.7 KB) on small helpers (lerp, progress, easing, color, dashed lines, arrows, draw wrappers), which are now built into the engine (section 3, E7/E8). Another ~33% (11.6 KB) is generic structure every video re-writes: the camera path (12%), the caption bar and progress bar (4%), the grid backdrop (2%), axes (3%), the glowing curve and area fill (4%), and the Riemann rectangles (5%). That is the `motion` library's job (section 4). Together about 41%.
- **Lua's real advantages:** no compiler needed, instant reload, a mistake doesn't crash the preview, sandboxable.
- **C++'s real advantages:** typos caught before running, the whole engine available without bindings. The library compensates for the first with a check pass that runs every scene (section 5.4).

Other observations from the two videos:

| Observation | `Demo.lua` | `NeuralNetVideo` | What the library does |
|---|---|---|---|
| Rendering is a pure function of `t` | `OnRender` reads `self.t` | `Render(t)` | Make it the rule, and check it |
| Named time segments | `T["curve"] = {a, b}` | hard-coded seconds | `Scene`, `Play`, `Wait`, `Mark` |
| Own easing functions | `ease.inOutCubic`... | `EaseIO`, `Back` | Engine `Curves` |
| Own helpers | `lerp`, `prog`, `mix`, `dashed`, `arrow`, `rgba` | `Seg`, `Win`, `Mix` | Engine `Math`/`Color`/`Draw` additions |
| Z management | `Draw.SetZ` 16 times | z per call | `Draw.Layer(z, fn)` |
| Text width | guessed: `#str * size * 0.56` | `MeasureText` | `Text.Measure` |
| Camera path | own pose table | fixed | Camera keyframes and framing |
| Captions, progress bar, title, end card | hand-drawn | hand-drawn | Theme components |
| Heavy drawing | up to 2048 rectangles per frame | 1,296 grid cells per frame | `Draw.Rects`, `Draw.Function` in C++ |
| Expensive data | Riemann sums at load | network training at start | `Precompute`, cached across reloads |

Engine facts that shaped the plan: `LuaRuntime::PushScriptClass` reloads a file when its mtime changes, but only when a new instance is created; modules loaded with `require` stay cached; `luaL_openlibs` opens `io` and `os` for every script; Lua has no way to measure text.

## 2. Architecture

```
 you · an agent (Claude Code, Codex...) · a chat assistant (ChatGPT, claude.ai) via copy/paste
                 │ write
                 ▼
 videos/<name>.lua  (or a folder: main.lua + scenes/*.lua)
                 │ watched
                 ▼
 ┌──────────────── MotionHost (C++, Engine/Motion) ─────────────────────┐
 │ build video in a sandbox → timeline → evaluate at t → render         │
 │ hot reload: rebuild on save, keep playhead, keep last good build     │
 │ check pass · recorder · copy error report · apply from clipboard     │
 └────────────┬─────────────────────────────────────────────────────────┘
   SIMPMotion preview <video>   SIMPMotion check|snapshot|render <video>   Editor Motion panel
              │ uses
 ┌────────────▼──────── motion Lua library (engine://lua/motion) ────────┐
 │ Video, Scene, objects, verbs (manim names), Camera, themes           │
 └────────────┬─────────────────────────────────────────────────────────┘
              │ uses
 ┌────────────▼──────── engine, shared with games ──────────────────────┐
 │ FileWatcher · live Lua reload · sandboxes · error overlay ·          │
 │ Clock/Random · FrameRecorder · Draw/Math/Color/Text upgrades ·       │
 │ Sequencer · camera keyframes · LuaRuntime instances · API reference  │
 └────────────┬─────────────────────────────────────────────────────────┘
              ▼
 existing: Renderer, Vector, TextOutline, Framebuffer (MSAA), Font, Tween/Curves, Lua bindings
```

| Layer | Language | Written by | Changes |
|---|---|---|---|
| Videos | Lua | you, agents, chat assistants | every edit |
| `motion` library | Lua (+ C++ fast paths) | you, or a task you approve | per feature |
| MotionHost, CLI, preview | C++ | you / approved task | rarely |
| Shared engine additions | C++ + Lua bindings | you / approved task | rarely |

The library is written in Lua so it hot-reloads too, and anyone can read exactly what a function does.

## 3. Engine additions shared with games

| # | Addition | For Motion | For games and the editor |
|---|---|---|---|
| E1 | **FileWatcher** (mtime polling ~10 Hz, coalesced, main thread) | rebuild on save | live reload of textures, scenes, `.particle` files, Tiled maps |
| E2 | **Live Lua reload of running scripts**: swap the class of live instances, `OnReload(self)` hook, reload `require`d modules from a dependency graph | library edits apply instantly | tweak gameplay while playing, no Stop/Play |
| E3 | **Script sandboxes**: per-script `_ENV`, profiles `game` (today's behavior, default for existing projects), `video` and `mod` (no `io`/`os`, instruction-count limit) | safe to run generated code | safe mods, no editor hang on `while true do end` |
| E4 | **Structured script errors + on-screen overlay** with file:line and "copy report" | errors in the preview, pasteable into a chat | faster debugging in the Game panel and dev builds |
| E5 | **Clock and Random**: real, fixed-step or driven time for `Time.*`; seeded generators | repeatable renders, scrubbing | replays, deterministic tests, seeded levels |
| E6 | **FrameRecorder**: MSAA target, PBO ring readback, ffmpeg pipe (mp4/webm/gif/mov), PNG fallback, screenshots | rendering videos | trailers, editor Record button, `SIMPPlayer --record`, visual tests |
| E7 | **Draw upgrades** (done: color tables, `"#hex"` strings and palette names in every `Draw.*` color position, `Color.Define`, an options table for `Draw.Text`/`Draw.Label`, `Draw.Layer`, `Draw.Arrow`, `Draw.Dashed`, `Draw.Label`; open: `Draw.Push/Pop`, option tables for the other calls): color tables and `"#hex"` and theme names everywhere, option tables, `Draw.Push/Pop` transforms, `Draw.Layer(z, fn)`, `Draw.Arrow`, `Draw.Dashed` | less boilerplate | HUDs, debug drawing, menus |
| E8 | **Math and Color helpers** (done, see SCRIPTING.md "Built-in helpers"; also `Timeline` and `Ease`): `Math.Remap/Smoothstep/Progress/Window`, `Color.Hex/Mix/WithAlpha` | removes per-video helpers | every game script |
| E9 | **Text layout for Lua** (done: `Draw.MeasureText`; open: `Text.Layout`, wrapping, rich spans): `Text.Measure`, `Text.Layout` (glyph boxes, wrapping), rich spans | per-letter effects, captions that fit | dialogue boxes, damage numbers |
| E10 | **Sequencer**: time-functional timeline of named segments and keyed tracks, evaluated at any `t` | the video timeline | cutscenes, scripted intros, scrubbable UI animation |
| E11 | **Camera keyframes and framing** (done in Lua: `CameraRig` poses along a `Timeline`, `Captions` bar; open: `camera:Frame(bounds, pad)`, a C++ sampler): `camera:Frame(bounds, pad)`, keyed paths | camera moves | cutscene cameras |
| E12 | **LuaRuntime instances** (more than one `lua_State`) | isolate a video from the editor and game | isolate editor tools from game scripts |
| E13 | **API reference from code**: `gen_signatures.py` also writes Markdown pages and a compact reference card | the LLM guide (8.3) stays in sync | docs for game scripters |
| E14 | **Batch and analytic primitives in C++**: `Draw.Rects(list)`, `Draw.Function(fn, x0, x1, opts)`, `Draw.Grid` | 2048 slices or dense graphs at 4K | charts, debug overlays, minimaps |

Motion-only: the `motion` library, themes for explainers, the check pass's lint rules, TeX and TTS providers, MotionHost, the preview and the CLI.

## 4. The `motion` library

### 4.1 Naming: borrow manim's vocabulary

Language models have read a lot of manim code. Using manim's names (`Write`, `Create`, `FadeIn`, `Transform`, `Indicate`, `Axes`, `MathTex`-like `Tex`, `next_to`-like placement, `play`/`wait`) means an assistant already knows most of the API before reading any docs, which saves tokens and mistakes. The syntax is Lua, the semantics are manim's where they make sense. Where we differ (time as a function, pixel design units), the guide says so in one line.

### 4.2 Shape of a video

A video can be **one file** (best for chat assistants) or a **folder** (best for long videos and agents):

```
videos/area-under-curve.lua          single file: every scene in one file

videos/area-under-curve/             folder
  main.lua                           Video{...}, scene order
  scenes/intro.lua                   one scene per file
  data/curve.lua                     shared functions and data
  assets/                            images, svg, fonts, audio
```

In a single file, scenes are separate top-level blocks (`Scene("name", function(s) ... end)`), each starting with a `-- @scene name` marker, so a scene can be replaced on its own (section 8).

Design units: **1920 × 1080, origin at the center, y up**, the same as `Demo.lua`. Output can be any size (`draft`, `hd`, `fhd`, `4k`) with the same composition, because the engine camera's visible height is 540 × zoom regardless of resolution.

### 4.3 API sketch (illustrative, not final)

```lua
local M = require "motion"

local function f(x) return 1.8 + 1.2 * math.sin(1.2 * x) + 0.25 * x end

local video = M.Video { title = "How integration works", size = "4k", fps = 60, theme = "midnight", seed = 7 }

-- @scene curve
video:Scene("curve", function(s)
    local ax = s:Axes { x = {0, 5, 1}, y = {0, 4, 1}, at = {-120, -60}, size = {1400, 720} }
    local g = s:Graph(ax, f, { color = "cyan" })
    local area = s:Area(ax, f, 0, 5, { color = "teal", alpha = 0.3 })

    s:Caption("STEP 1 - A NON-LINEAR CURVE", "Its height f(x) changes at every point along x")
    s:Play(M.Create(ax), 1.2)
    s:Play(M.Create(g), 2.0, "easeInOutSine")
    s:Camera { frame = g, pad = 80, time = 1.5 }
    s:Play(M.FadeIn(area), 1.0)
    s:Mark("question")
end)

-- @scene slices
video:Scene("slices", function(s)
    local ax = s:Use("curve.ax")
    local n = s:Value(4)
    s:Draw(function(t, d)
        d.Riemann(ax, f, 0, 5, n:At(t), { fill = "violet" })
    end)
    for _, k in ipairs { 8, 16, 32, 64, 128, 256 } do
        s:Play(n:To(k), 1.0, "easeInOutCubic")
        s:Wait(0.4)
    end
end)

return video
```

- A scene function runs **once per build**, declaring objects and appending animations to the scene's timeline (`Play` and `Wait` advance a cursor, like manim).
- Objects are handles with ids, so the check pass and the preview can name them.
- `s:Value(x)` is a tracker whose value at any `t` comes from the animations applied to it.
- `s:Draw(fn)` is the immediate-mode escape hatch (`fn(t, d)` must be pure in `t`). Most of `Demo.lua`'s custom drawing would live here.
- `s:Precompute(key, fn)` runs expensive work once and caches it by a hash of its code, so hot reload doesn't redo it. This is how `NeuralNetVideo` becomes Lua.
- `s:Use("scene.id")` reuses an object from an earlier scene.

### 4.4 Building blocks

| Group | Contents |
|---|---|
| Objects | `Text`, `Tex` (TeX or the Unicode fallback), `Circle`, `Rect`, `Polygon`, `Line`, `Arrow`, `Dot`, `Path`, `Svg`, `Image`, `Brace`, `Box`, `Group`, `Axes`, `Graph`, `Area`, `Riemann`, `NumberLine`, `Plane`, `Vector`, `Matrix`, `Table`, `Code`, `Counter` |
| Placement | `at`, `NextTo`, `Align`, `Arrange`, `Grid`, `ToEdge`, `Fit` |
| Verbs | `Create`, `Write` (per glyph, from `TextOutline`), `FadeIn/FadeOut`, `GrowFrom`, `Move`, `Rotate`, `Scale`, `Color`, `Transform`, `Match` (glyph matching), `Indicate`, `Circumscribe`, `Flash`, `Focus`, `Count`, `Camera` |
| Timing | `Play(clip, time, curve)`, `Wait`, `Mark`, `Lag` (staggered), `Together`, `Seq` |
| Theme | `Caption`, `TitleCard`, progress bar, `EndCard`, section HUD |

Each verb is a Lua function returning a clip (`{duration, apply(t, obj)}`), so a video can define its own verbs locally. Curves are the engine's `Curves`.

## 5. Hot reload and the preview

### 5.1 The loop

1. FileWatcher (E1) sees a change to the video or anything it loaded (modules, the library, assets).
2. The host builds the video in a fresh sandbox (E3) on its own Lua state (E12): runs the file, runs every scene function, compiles timelines.
3. Success: swap in the new build, keep the playhead, redraw. Failure: keep the last good build on screen with the error overlay (E4).
4. Unchanged scenes reuse their cached build (keyed by a hash of the scene's code and dependencies).

Targets: under 150 ms from save to new frame for a 90-second video, under 50 ms when one scene changed.

### 5.2 Keeping your place

The playhead is stored as (scene name, time within scene). Making the intro longer doesn't move you out of the scene you're watching.

### 5.3 What the preview shows

`SIMPMotion preview <video>` and the editor's Motion panel share the same UI:

- **Scene strip** with durations and a status for each scene: built, error, changed in the last build (pulses briefly).
- **Timeline** with marks, play/pause/scrub, loop the current scene, 0.25×–2× speed.
- **Follow changes** (on by default): when a scene changes, jump to it and play it.
- **Change highlight**: objects whose definition changed flash briefly.
- **Activity feed**: "12:04:31 scenes/slices.lua changed (lines 40–52)", "build ok, 38 ms".
- **Error banner** with file:line; click to open the script editor there.
- Buttons for the chat workflow (8.2): **Copy error report**, **Copy snapshot**, **Apply from clipboard**.

### 5.4 The check pass (catches what a compiler would)

Lua only reports an error when the line runs, and a `Draw` body at 70 s doesn't run until you get there. So after every successful build, the host renders every scene in the background at a few sample times (start, middle, end, each mark) at low resolution, without showing them. Runtime errors anywhere in the video show up within a second of saving, plus:

- **Lint**: objects off-frame or outside the safe area, text overlapping text, text too small at 1080p, low contrast, empty scenes.
- **Purity**: render t, t − 3 s, then t again; differing pixel hashes mean the scene keeps state it shouldn't.
- **Budget**: a scene spending more than ~8 ms of Lua per frame at preview quality gets a note naming the hot `Draw` call (data for deciding what to move into C++).

### 5.5 Control

- `-- @lock` at the top of a scene: the preview warns if a locked scene changes, and "Apply from clipboard" refuses to replace it.
- Videos are plain Lua files: git shows exactly what changed. A "Checkpoint" button commits the video.

## 6. Looking extremely good

The comparison showed the look comes from the renderer and the design, so this gets its own track.

**Renderer (C++)**

| Item | State |
|---|---|
| MSAA render targets, crisp vector strokes at 4K | done |
| Text as vector outlines (sharp at any zoom), Greek and math symbols | done |
| Linear-light blending and sRGB output (soft glows and gradients without muddy edges) | planned |
| Motion blur for fast moves (sub-frame accumulation, offline only) | planned |
| Radial gradients, soft shadows behind text and panels | planned |
| Glow and bloom presets tuned for dark explainer themes | partly done (Glow, bloom exist), presets planned |
| Even-odd fill for self-intersecting paths | planned |

**Design defaults (Lua library)**

- **Themes** with a tested palette (contrast-checked for text), font pairs and sizes on a type scale, stroke widths in screen units so lines stay equally thick at any zoom.
- **Motion defaults** tuned for explainers: `easeInOutCubic` for moves, a slight overshoot for things appearing, staggers of ~0.05–0.1 s per item, holds long enough to read (about 2.5 words per second).
- **Layout defaults**: safe area, consistent margins, captions that measure and fit themselves.
- **Reference stills**: each theme has a set of golden frames, compared on change, so the look doesn't drift.

## 7. Tokens

Ranked by size of the saving:

| Lever | Saving | Source |
|---|---|---|
| Built-in helpers (E7/E8, done) | ~8% of a video's code | measured on `Demo.lua`: 2.9 of 34.7 KB |
| `motion` library objects: camera, captions, backdrop, Axes/Graph/Area/Riemann | measured: the compact rewrite `assets/scripts/IntegrationShort.lua` is 2.9 KB against 16.9 KB for the same parts of `Demo.lua` (about 5.8x smaller; it leaves out the backdrop grid, slice notes, readout panel, title and formula board) | `Demo.lua`: 11.6 of 34.7 KB is generic structure |
| Fix loop: one-line errors, text lint, small edits (one scene at a time) instead of rewriting the whole file | the largest saving over a full session | most tokens in a session go to fixing, not the first draft |
| manim vocabulary | fewer docs to read, fewer wrong guesses | 4.1 |
| Lua instead of C++ | ~10–20% | section 1 |

Estimates for a 90-second explainer (not measured yet): first draft ~4–6k output tokens with the library (vs ~10k for `Demo.lua` today), a typical fix 150–300 tokens.

## 8. Three ways to make a video

### 8.1 By hand, or an agent with tools (Claude Code, Codex)

The agent edits files directly and runs the CLI:

```
SIMPMotion check videos/area.lua                # build + check pass, one line per problem
SIMPMotion snapshot videos/area.lua slices      # contact sheet PNG of a scene
SIMPMotion render videos/area.lua --quality 4k
```

A short guide (a Claude Code skill or an `AGENTS.md` section) describes the workflow: write one scene, run `check`, fix, snapshot once per scene, render. The preview window shows you what it is doing as files change.

### 8.2 A chat assistant without tools (ChatGPT, claude.ai)

The assistant can't run anything or see the preview, so you are the loop, and the tools are built to make each round trip short:

1. **Start**: paste (or attach) the **LLM guide** (8.3) and describe the video. In ChatGPT, put the guide and a couple of example videos in a Custom GPT or Project's knowledge, so you don't paste them every time.
2. **The assistant writes** a single-file video, scene by scene, with `-- @scene` markers.
3. **You apply it**: save the file, or click **Apply from clipboard** in the preview. A whole file replaces the video; one or more `-- @scene` blocks replace just those scenes, so the assistant never has to repeat the whole file.
4. **You watch** it live in the preview (hot reload, follow changes).
5. **Something wrong?** Click **Copy error report** (8.4) and paste it into the chat. For visual issues, click **Copy snapshot** (a contact sheet of the current scene) and paste the image. Both ChatGPT and Claude can read images.
6. **The assistant answers with only the changed scene**, which you apply the same way.

### 8.3 The LLM guide

One Markdown file, `docs/motion/LLM_GUIDE.md`, generated from the code (E13) plus hand-written rules, kept under ~4k tokens:

- the rules: one file, `-- @scene` blocks, scenes are functions of time, no `os`/`io`/`math.random` (use `s:Random()`), reply with changed scenes only;
- the API card: every object, verb and option, one line each, with manim equivalents;
- two short complete examples;
- the error and lint codes with one-line fixes;
- the API version (`M.Video{ api = 1 }`), so an outdated guide is detected.

### 8.4 The error report

Built for pasting: short, complete, no log noise.

```
SIMP Motion report · api 1 · videos/area.lua · build failed
E slices:42  Riemann: n must be a positive integer (got 0)
   41 |     s:Draw(function(t, d)
   42 |         d.Riemann(ax, f, 0, 5, n:At(t) - 4, { fill = "violet" })
   43 |     end)
W curve: text "y = f(x)" overlaps the axis at 4.2 s
Scenes: intro 4.6s ok · curve 11.4s ok · slices 18.2s error · formula 14.0s ok
```

## 9. Where C++ is used

| In C++ | Examples |
|---|---|
| Per-frame work over many items | `Draw.Rects`, `Draw.Function`, Riemann slices, grids |
| Renderer and GPU | MSAA, glow, blur, gradients, sRGB, motion blur |
| Processes and I/O | ffmpeg, TeX, TTS, file watching |
| Timing and recording | Clock, Sequencer core, FrameRecorder |
| Anything games need too | section 3 |

What a video says and shows stays in Lua. Generated code is Lua only; C++ changes are separate tasks you approve. When the check pass reports a scene over its Lua budget, that's the signal for a new C++ primitive.

## 10. Roadmap

Each phase ends with something usable, and shared engine work comes first.

**Status (built so far):** `SIMPPreview` and `SIMPRecord` with a shared `MotionHost` (see [MOTION.md](MOTION.md)): hot reload of a script and the `.lua` files next to it with the playhead kept, the last Lua error shown in a banner, a fixed-step deterministic recorder with draft/hd/fhd/4k presets, partial ranges, stills and a PNG-sequence fallback, plus the `Video` global. Of the engine items: E1 (polling file watcher) lives inside `MotionHost` only, E2 (reloading *running* game scripts) is still open, E3 sandboxes and E5 clock/random are open, E4 is partly done (the runtime keeps the last error, only the preview shows it), E6 is done as `SIMPRecord` (no PBO readback, no editor Record button yet), E12 is open.

**Phase A: engine foundations (shared)**: E1 FileWatcher, E2 live Lua reload, E3 sandboxes, E4 error overlay and report, E5 Clock/Random, E6 FrameRecorder (+ `SIMPPlayer --record`, editor Record button), E12 LuaRuntime instances.
Done when: editing `Demo.lua` while it plays updates it without Stop/Play; a broken edit shows file:line while the last good version keeps running; `Demo.lua` records to a 4K MP4 from the command line, byte-identical on two runs.

**Phase B: Draw, Math, Color, Text, Camera (shared)**: E7, E8, E9, E11, E14, E13 (reference pages).
Done when: `Demo.lua` rewritten against the new APIs is noticeably shorter with identical frames (PSNR check against today's version), and its 2048-slice stage costs under 2 ms of Lua per frame.

**Phase C: Sequencer, MotionHost, preview, CLI**: E10, scene cache, playhead by scene, check pass, preview UI (5.3) including the three clipboard buttons, `SIMPMotion preview|check|snapshot|render`.
Done when: a single-file video hot-reloads in the preview with follow mode, a runtime error at 70 s is reported within a second of saving, and `render --quality 4k` works.

**Phase D: the `motion` library and the LLM guide**: objects, placement, verbs, Values, Precompute, Use, themes; `LLM_GUIDE.md` generated and checked in CI.
Done when: `Demo.lua` is ported to the library at about half its size with the same look, `NeuralNetVideo` is ported to Lua, and ChatGPT given only the guide produces a working 60-second video in three paste rounds or fewer.

**Phase E: looking great**: section 6 renderer items, theme golden frames, motion and layout defaults.

**Phase F: math and narration**: TeX providers and `Match` (MOTION_MCP.md section 8), TTS and timing sync (section 9 there), the full editor Motion panel.

## 11. Deferred: MCP

Not built for now. A command-line tool plus a guide gives agents the same abilities, and the chat workflow doesn't need it. Reasons to revisit later:

- controlling a running preview from an agent (jump to a scene, follow on/off) beyond what file watching gives;
- supporting an agent that can't run shell commands but speaks MCP;
- returning images directly instead of PNG files.

If built, it would be a thin layer over the same host functions as the CLI.

## 12. Risks and open questions

| Risk | Mitigation |
|---|---|
| Lua too slow for dense scenes at 4K | E14 batch primitives, per-scene budget notes; offline renders don't need real time |
| Scenes that keep state break scrubbing | sandbox removes the obvious sources, the purity check reports the rest |
| Live reload of running game scripts loses state | instances keep their fields, `OnReload(self)`, per-script opt-out |
| Sandboxing breaks existing game scripts | sandbox profiles; `game` keeps today's behavior |
| Chat assistants use an outdated guide | API version in the guide and in videos, with a clear warning |
| Assistants still rewrite whole files in chat | guide rule plus "Apply from clipboard" accepting scene blocks; the guide's examples show scene-only replies |
| Library API churn | `api` version field, aliases for one version |

Open questions:

1. Lua 5.4/5.5 (vendored, the log reports 5.5) or LuaJIT? Proposal: stay, solve speed with C++ primitives.
2. Where do videos live? Proposal: anywhere; inside a game project under `videos/` by convention.
3. Keep `Demo.lua` as an immediate-mode example after the port? Proposal: yes.
