# Effects: premade tweens

Premade animations for any entity, one line each. The same 38 presets show up in the **Effects and Easing** panel (View menu) as live previews with copy-ready code, and in C++ as `EffectPresets`.

```lua
Effect.PopIn(self.entity)                                  -- one-liner
Effect.Shake(self.entity, {time = 0.3, distance = 2})      -- with options
Effect.Play(self.entity, "slideInLeft", {delay = 0.2})     -- by name
local float = Effect.Float(self.entity)                    -- loops forever...
float:Kill()                                               -- ...until killed
```

`Effect.Chain(e, {"popIn", "pulse", "fadeOut"})` plays several one after another and `Effect.Stagger(list, "popIn", 0.1)` starts one on a list of entities with a delay between each (see [SCRIPTING.md](SCRIPTING.md#built-in-helpers)).

Every function returns the same tween handle as `Tween.To` (`Pause`, `Resume`, `Seek`, `Kill`, `OnComplete(fn)`, ...), so effects can be chained and put in `Tween.Sequence` / `Tween.Parallel`.

## Options

| Option | Default | Meaning |
|---|---|---|
| `time` (or `duration`) | the effect's own | length of one play in seconds |
| `delay` | 0 | seconds before it starts; entrances jump to their start state immediately so nothing flashes at full opacity first |
| `distance` | 1 | multiplies how far position moves go (slides, drops, shakes, bounces) |
| `amount` | 1 | multiplies the size of scale, rotation and opacity changes (pulse, wobble, flash...) |
| `speed` | 1 | playback speed |
| `loop` | effect default | `true` forever, `false` once, a number for that many extra plays |

Functions: `Effect.Play(target, name [, options])`, `Effect.Names([category])`, `Effect.Categories()`, `Effect.Info(name)` (a table with `name`, `label`, `category`, `description`, `duration`, `loop`), and `Effect.<Name>(target [, options])` for every effect below (`Effect.PopIn`, `Effect.slideInLeft` also works). An unknown name raises an error that lists `Effect.Names()` as the fix.

A target needs the components a property lives on: opacity changes need a Sprite, Shape, Vector, Text or Tilemap; position, scale and rotation need only a Transform. The editor panel previews on a plain shape.

## Entrance

Start away from rest and end exactly at the entity's current state, so put the entity where it should finish and play the effect. They animate from a changed value to what the entity already has (like `Tween.From`).

| One-liner | Seconds | What it does |
|---|---|---|
| `Effect.FadeIn(e)` | 0.5 | Fades from transparent to the current opacity. |
| `Effect.SlideInLeft(e)` | 0.6 | Slides in from the left while fading in. |
| `Effect.SlideInRight(e)` | 0.6 | Slides in from the right while fading in. |
| `Effect.SlideInTop(e)` | 0.6 | Slides down into place from above while fading in. |
| `Effect.SlideInBottom(e)` | 0.6 | Slides up into place from below while fading in. |
| `Effect.PopIn(e)` | 0.5 | Grows from nothing with a small overshoot. |
| `Effect.ZoomIn(e)` | 0.5 | Scales up from a smaller size while fading in. |
| `Effect.DropIn(e)` | 0.9 | Falls from above and bounces to rest. |
| `Effect.SpinIn(e)` | 0.7 | Spins and grows into place. |
| `Effect.FlipIn(e)` | 0.55 | Opens horizontally like a card turning toward you. |
| `Effect.RiseIn(e)` | 0.7 | A gentle upward drift while fading in. |
| `Effect.ElasticIn(e)` | 1 | Springs out of nothing and wobbles to rest. |

## Exit

Fade to transparent, usually moving or shrinking on the way. The entity stays invisible (and wherever it ended up) afterwards.

| One-liner | Seconds | What it does |
|---|---|---|
| `Effect.FadeOut(e)` | 0.5 | Fades to transparent. |
| `Effect.SlideOutLeft(e)` | 0.6 | Slides away to the left while fading out. |
| `Effect.SlideOutRight(e)` | 0.6 | Slides away to the right while fading out. |
| `Effect.SlideOutTop(e)` | 0.6 | Slides up and away while fading out. |
| `Effect.SlideOutBottom(e)` | 0.6 | Slides down and away while fading out. |
| `Effect.PopOut(e)` | 0.5 | Shrinks to nothing with a small wind-up. |
| `Effect.ZoomOut(e)` | 0.5 | Grows and fades away. |
| `Effect.DropOut(e)` | 0.6 | Drops away with a small wind-up. |
| `Effect.SpinOut(e)` | 0.7 | Spins and shrinks away. |

## Attention

Move away and come back to exactly where they started, so they are safe to play at any time, repeatedly.

| One-liner | Seconds | What it does |
|---|---|---|
| `Effect.Pulse(e)` | 0.6 | Swells once and settles back. |
| `Effect.Heartbeat(e)` | 0.9 | Two quick beats, then a rest. |
| `Effect.Shake(e)` | 0.5 | A quick horizontal shake. |
| `Effect.HeadShake(e)` | 0.7 | A short no-no shake with a little tilt. |
| `Effect.Wobble(e)` | 0.8 | Rocks side to side and settles. |
| `Effect.Swing(e)` | 1 | Swings like a pendulum and settles. |
| `Effect.Bounce(e)` | 1 | Three hops of shrinking height. |
| `Effect.Jump(e)` | 0.6 | One high jump. |
| `Effect.RubberBand(e)` | 1 | Stretches and snaps back like rubber. |
| `Effect.Tada(e)` | 1 | A burst of size and a celebratory wiggle. |
| `Effect.Flash(e)` | 0.9 | Blinks three times. |
| `Effect.SpinOnce(e)` | 0.8 | One full turn. |

## Loop

Never finish. They return a handle; call `handle:Kill()` to stop; the entity stays wherever it was in the cycle, so let a cycle finish or reset its transform yourself if that matters.

| One-liner | Seconds | What it does |
|---|---|---|
| `Effect.Float(e)` | 2.4 | Bobs gently up and down, forever. |
| `Effect.Breathe(e)` | 3 | Slowly grows and shrinks, forever. |
| `Effect.Spin(e)` | 3 | Turns at a steady pace, forever. |
| `Effect.Sway(e)` | 2.5 | Rocks slowly left and right, forever. |
| `Effect.Blink(e)` | 1.2 | Dips in and out of view, forever. |

## Expanded code

Every effect is made from ordinary tweens. The panel's "Expanded Tween code" box shows exactly what a preset does, with your current options, as plain `Tween.*` calls you can copy and edit. For example `Effect.PopIn` is:

```lua
local e = self.entity
Tween.Parallel({
    Tween.From(e, {scaleX = 0, scaleY = 0}, 0.5, "easeOutBack"),
    Tween.From(e, {alpha = 0}, 0.2, "easeOut")
})
```

## Easing curves

The Easing tab of the same panel previews every curve (the graph, plus a ball travelling along a line, like easings.net) and copies `"easeOutBack"`, `Tween.To(self.entity, {x = 300}, 0.6, "easeOutBack")`, `Ease.outBack(t)` or `Curves.Evaluate("easeOutBack", t)`. See [ANIMATION.md](ANIMATION.md).

## In C++

```cpp
#include <Engine/Animation/EffectPresets.h>

if (const EffectPreset *pop = EffectPresets::Find("popIn"))
{
    EffectOptions options;
    options.Duration = 0.4f;
    uint32_t id = EffectPresets::Play(scene.GetTweens(), entity.GetHandle(), *pop, options);
}
```

Presets are data (`EffectPreset` = parallel lanes of sequential segments of tween tracks, built in `EffectPresets.cpp`), so adding one is a single `add(...)` line; the tests check that every entrance ends at rest, every exit ends invisible, every attention effect returns to rest and every loop keeps running.
