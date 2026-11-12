# Motion: videos and animations from Lua

Write an animation as a Lua script, watch it live while you edit it, and render it to a video. Everything here is built into the engine; the Lua helpers are described in [SCRIPTING.md](SCRIPTING.md#built-in-helpers) and the premade effects in [EFFECTS.md](EFFECTS.md).

```sh
SIMPPreview my_video.lua                    # live window, reloads whenever you save
SIMPRecord  my_video.lua --quality 4k       # my_video.mp4
```

Both programs are in `build/<preset>/bin` (and in the packaged editor folder). Recording needs `ffmpeg` on the PATH; without it `SIMPRecord` writes a PNG sequence instead.

## A script

A video is an ordinary script: a table with `OnUpdate` and `OnRender`, plus two lines that make it work with the tools.

```lua
local S = {}

local tl = Timeline.New({{"intro", 2}, {"curve", 5}, {"outro", 2}})
Video.Duration = tl:Total()                         -- how long the video is (seconds)

local ax = Axes.New{origin = {-500, -250}, scale = {200, 150}, x = {-0.3, 5.4, 1}, y = {-0.1, 3.5, 1}}
local plot = ax:Plot(function(x) return 1.5 + math.sin(x) end, 0, 5)

function S:OnCreate() self.t = 0 end

function S:OnUpdate(dt)
    self.t = Video.Step(self.t, dt)                 -- the tool's clock when driven, your own clock otherwise
end

function S:OnRender()
    local t = self.t
    Backdrop.Fill(0, 0, 1, "bg")
    ax:Draw(tl:Progress(t, "intro"))
    plot:Draw(5 * Ease.inOutSine(tl:Progress(t, "curve")))
end

return S
```

- **`Video.Duration`**: the length in seconds. `SIMPRecord` needs it (or `--seconds`); `SIMPPreview` uses it for the progress bar and looping.
- **`Video.Step(t, dt)`**: returns the time to draw. Under `SIMPPreview` and `SIMPRecord` it is the tool's clock, so seeking and fixed-rate recording work; in a normal game it is `t + dt`. A script that keeps its own `self.t = self.t + dt` still records correctly (frames are stepped at a fixed rate), it just cannot be scrubbed in the preview.
- **`Video.Driven`** and **`Video.Recording`** tell the script how it is running, **`Video.Time`** is the tool's current time.
- A scene is best written as a function of time: everything on screen at `t` computed from `t`. The library (`Timeline`, `CameraRig`, `Captions`, `Axes`, `Glyphs`, `Effect`) is built that way.

`assets/scripts/IntegrationShort.lua` is a complete example (axes, curve, area, 4 to 512 slices, captions, camera) in 80 lines.

## SIMPPreview

`SIMPPreview script.lua [--assets DIR]` opens a window that runs the script.

| Key | Action |
|---|---|
| Space | play / pause |
| Left, Right | seek 2 seconds (hold Shift for 10) |
| `,` and `.` | step one frame back / forward |
| R or Home | back to the start |
| L | loop on / off |
| F5 | reload now |

**Hot reload**: the script is rebuilt whenever it, or any `.lua` file in its folder (up to three levels down), is saved, and your place in the video is kept. Modules loaded with `require` are reloaded too. A broken save does not stop anything: the error (with the file and line) appears in a red banner at the top, the last picture stays on screen, and fixing the file clears it. The bottom bar shows the time, play state and a "reloaded" notice.

## SIMPRecord

```
SIMPRecord script.lua [--out file.mp4] [--quality draft|hd|fhd|4k] [--fps N] [--size W H]
                      [--seconds S] [--from S] [--still T]... [--assets DIR]
```

| Quality | Size | fps | Anti-aliasing | Use |
|---|---|---|---|---|
| `draft` | 960x540 | 30 | 2x | quick checks (a 30 s video takes about 7 s) |
| `hd` | 1280x720 | 30 | 4x | |
| `fhd` (default) | 1920x1080 | 60 | 4x | |
| `4k` | 3840x2160 | 30 | 4x | final render |

- `--from S --seconds E` renders just that range (the scene is simulated up to `S` first), which is how you iterate on one scene of a long video.
- `--still 12.5 --still 30` writes PNGs of single frames (`<out>_12.50.png`) instead of a video; use it to check a moment without waiting for a render.
- `--out name.png` writes a numbered PNG sequence.
- The composition is the same at every resolution: the camera maps the same world area to the frame, so a draft is a faithful small copy of the final.
- Exit codes: 0 ok, 2 the script failed to load or has no `Video.Duration`, 3 no OpenGL context or ffmpeg could not start, 4 ffmpeg failed, 5 the script reported an error while recording (the video is still written).

Recording is deterministic: time advances in exact steps of 1/fps, so recording twice gives the same frames.

## Under the hood

`MotionHost` (`Engine/Motion/MotionHost.h`) is what both programs use: it owns a private scene with a camera and your script, drives `Video.Time`, rebuilds on file changes (`PollReload`, `Reload`), keeps the last Lua error (`Error()`), and renders into whatever render target is bound (`Render(w, h)`). A game or tool can embed it the same way.

```cpp
SIMPEngine::MotionHost host;
host.Load("my_video.lua", /*recording=*/false);
host.Step(1.0f / 60.0f);          // or host.Seek(12.0f)
host.Render(1920, 1080);          // into the currently bound framebuffer
```

## Tips

- Draw the background yourself (`Backdrop.Fill`, `Backdrop.Grid`) so the picture does not depend on the clear color.
- Use `Draw.Layer(z, fn)` to order things; larger z is in front.
- Names in errors are checked ("did you mean 'curve'?"); the preview banner shows them as they happen.
- Text that should be written on like handwriting: `Glyphs.New("e^(i*pi) + 1 = 0", 120):Draw(0, 0, progress)`.
- Put shared helpers in other `.lua` files next to the script and `require` them; saving any of them reloads the video.
