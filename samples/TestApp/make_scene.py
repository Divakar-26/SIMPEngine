#!/usr/bin/env python3
"""Regenerates assets/scenes/main.yaml for the test app."""
from pathlib import Path
import yaml

ents = []
next_id = [1]


def add(tag, comps, parent=None):
    entity = {"ID": next_id[0], "Tag": tag}
    next_id[0] += 1
    entity.update(comps)
    if parent:
        entity["Hierarchy"] = {"parent_id": parent}
    ents.append(entity)
    return entity["ID"]


def T(x, y, z=0, rot=0):
    return {"Transform": {"position": [x, y], "rotation": rot, "zIndex": z, "scale": [1, 1]}}


def solid(tag, x, y, w, h, color, z=0):
    add(tag, {**T(x, y, z), "Render": {"color": color, "size": [w, h]}, "Collision": {"size": [w, h], "offset": [-w / 2, -h / 2]}})


add("Camera", {**T(0, 0), "Camera": {"primary": True, "zoom": 0.5, "follow": "Player", "follow_offset": [0, 60], "smoothing": 6}})
add("Sky", {**T(0, 0, -10), "Render": {"color": [22, 28, 52, 255], "size": [6000, 3000]}})
solid("Ground", 0, -200, 3200, 80, [84, 150, 104, 255])
solid("WallL", -1600, 200, 40, 1000, [40, 60, 80, 255])
solid("WallR", 1600, 200, 40, 1000, [40, 60, 80, 255])

platforms = [(-300, -90, 220), (0, -20, 200), (320, -80, 200), (640, 40, 240), (-640, -10, 200),
             (980, -60, 220), (1250, 40, 200), (-980, -70, 220), (-1250, 20, 200)]
for i, (x, y, w) in enumerate(platforms):
    solid(f"Platform{i + 1}", x, y, w, 22, [110, 160, 200, 255], 1)

coins = [(x, y + 45) for x, y, _ in platforms] + [(x, -130) for x in (-120, 160, 480, 780, -480, 1100, -1100)]
for i, (x, y) in enumerate(coins):
    add(f"Coin{i + 1}", {**T(x, y, 2), "Render": {"color": [255, 214, 90, 255], "size": [18, 18]},
                         "Trigger": {"size": [26, 26], "offset": [-13, -13], "filter": "Player"},
                         "Pickup": {"key": "coins", "amount": 1, "destroy_on_pickup": True},
                         "Glow": {"color": [255, 200, 80, 255], "intensity": 0.8, "spread": 2}})

add("Water", {**T(1050, -150, 0.5), "Render": {"color": [48, 110, 210, 255], "size": [420, 70]}})
add("WaterRipple", {**T(1050, -150), "ShaderCanvas": {
    "shader": "assets://shaders/ripple.glsl", "scope": "z_range", "z_range": [0.4, 0.6], "limit_area": True, "area_size": [420, 70]}})

for i, x in enumerate((-700, 0, 700, 1250)):
    add(f"Lamp{i + 1}", {**T(x, 60, 3), "Light": {"type": "point", "color": [255, 220, 170, 255], "intensity": 1.0, "radius": 320, "falloff": 1.6}})

add("Player", {**T(0, -110, 3), "Render": {"color": [90, 220, 190, 255], "size": [32, 48]},
               "CharacterController": {"mode": "platformer", "size": [32, 48], "move_speed": 300, "jump_speed": 720, "max_jumps": 2},
               "Light": {"type": "point", "color": [150, 240, 220, 255], "intensity": 0.7, "radius": 260, "falloff": 1.4},
               "Glow": {"color": [110, 240, 210, 255], "intensity": 0.2, "spread": 2}})

add("Vignette", {"ShaderCanvas": {"shader": "assets://shaders/vignette.glsl", "scope": "scene", "order": 1}})

hud = add("HUD", {"Canvas": {"reference_size": [1280, 720], "scale_mode": "scale_with_screen", "match": 0.5, "safe_area": 0.02}})
bar = add("CoinBar", {"UIRect": {"anchor_min": [0, 1], "anchor_max": [0, 1], "pivot": [0, 1], "position": [16, -16], "size": [220, 52]},
                      "UIPanel": {"color": [14, 18, 34, 200], "corner_radius": 14, "border_color": [255, 255, 255, 70], "border_width": 2}}, hud)
add("CoinLabel", {"UIRect": {"anchor_min": [0, 0], "anchor_max": [1, 1]},
                  "UILabel": {"text": "Coins {coins}", "size": 28, "color": [255, 222, 120, 255], "align": 1, "vertical_align": 1}}, bar)
add("Info", {"UIRect": {"anchor_min": [0.5, 1], "anchor_max": [0.5, 1], "pivot": [0.5, 1], "position": [0, -18], "size": [520, 40]},
             "UILabel": {"text": "", "size": 22, "color": [190, 205, 235, 255], "align": 1, "vertical_align": 1, "shadow": True},
             "Script": {"lua": "assets://scripts/hud.lua"}}, hud)
reset = add("RestartButton", {"UIRect": {"anchor_min": [1, 1], "anchor_max": [1, 1], "pivot": [1, 1], "position": [-16, -16], "size": [170, 52]},
                              "UIPanel": {"color": [70, 100, 180, 255], "corner_radius": 14}, "UIButton": {},
                              "Script": {"lua": "assets://scripts/restart_button.lua"}}, hud)
add("RestartText", {"UIRect": {"anchor_min": [0, 0], "anchor_max": [1, 1]},
                    "UILabel": {"text": "Restart", "size": 24, "color": [255, 255, 255, 255], "align": 1, "vertical_align": 1}}, reset)

scene = {"Scene": "TestApp", "Version": 1,
         "Settings": {"background": [0.05, 0.06, 0.12, 1], "lighting": {"enabled": True, "ambient": [0.55, 0.6, 0.78, 1]},
                      "bloom": {"enabled": True, "threshold": 0.85, "knee": 0.3, "intensity": 0.5, "radius": 1.1}},
         "Entities": ents}
out = Path(__file__).parent / "assets/scenes/main.yaml"
out.write_text(yaml.dump(scene, sort_keys=False, default_flow_style=None, width=1000))
print("wrote", out, len(ents), "entities")
