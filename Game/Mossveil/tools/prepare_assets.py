import json
import math
import sys
from collections import deque
from pathlib import Path

import numpy as np
from PIL import Image

SOURCE = Path(sys.argv[1]) if len(sys.argv) > 1 else Path.home() / "Music" / "HollowKNight"
OUT = Path(sys.argv[2]) if len(sys.argv) > 2 else Path(__file__).resolve().parents[3] / "assets" / "mossveil"

WIZARD = SOURCE / "BlueWizard Animations" / "BlueWizard"
ANIMATIONS = {
    "wizard_idle": WIZARD / "2BlueWizardIdle",
    "wizard_walk": WIZARD / "2BlueWizardWalk",
    "wizard_jump": WIZARD / "2BlueWizardJump",
    "wizard_dash": WIZARD / "2BlueWizardJump" / "Dash2",
}
PLANTS = {
    "plant_tall": "Plant 1",
    "plant_curl": "Plant 2",
    "plant_tuft": "Plant 3",
    "plant_bulb": "Plant 4",
    "plant_sway": "Plant 5",
    "plant_fern": "Plant 6",
    "plant_reed": "Plant 7",
    "plant_wind": "Plant Wind 1",
    "flower_blue": "BlueFlower1",
    "flower_cap": "BlueFlower2",
}
SHEETS = {
    "tiles": "Mossy - TileSet.png",
    "platforms": "Mossy - FloatingPlatforms.png",
    "decor": "Mossy - Decorations&Hazards.png",
    "hanging": "Mossy - Hanging Plants.png",
    "hills": "Mossy - MossyHills.png",
    "background": "Mossy - BackgroundDecoration.png",
}


def frames_in(folder):
    return sorted(p for p in folder.iterdir() if p.suffix.lower() == ".png")


def union_bbox(images):
    box = None
    for img in images:
        b = img.getbbox()
        if not b:
            continue
        box = b if box is None else (min(box[0], b[0]), min(box[1], b[1]), max(box[2], b[2]), max(box[3], b[3]))
    return box


def pack_animation(name, files, cell_height, max_frames, manifest, shared_box=None, step=1):
    files = files[::step][:max_frames]
    images = [Image.open(f).convert("RGBA") for f in files]
    box = shared_box or union_bbox(images)
    pad = 4
    box = (max(0, box[0] - pad), max(0, box[1] - pad), box[2] + pad, box[3] + pad)
    bw, bh = box[2] - box[0], box[3] - box[1]
    scale = cell_height / bh
    cw, ch = max(1, round(bw * scale)), cell_height
    columns = min(len(images), 10)
    rows = math.ceil(len(images) / columns)
    sheet = Image.new("RGBA", (cw * columns, ch * rows), (0, 0, 0, 0))
    for i, img in enumerate(images):
        frame = img.crop(box).resize((cw, ch), Image.LANCZOS)
        sheet.paste(frame, ((i % columns) * cw, (i // columns) * ch))
    sheet.save(OUT / f"{name}.png", optimize=True)
    manifest["animations"][name] = {"frame_width": cw, "frame_height": ch, "frames": len(images), "columns": columns}
    print(f"{name}: {len(images)} frames {cw}x{ch}")


def components(alpha, downscale=8, threshold=24):
    h, w = alpha.shape
    small = alpha[: h - h % downscale, : w - w % downscale]
    small = small.reshape(h // downscale, downscale, w // downscale, downscale).max(axis=(1, 3)) > threshold
    sh, sw = small.shape
    labels = np.zeros(small.shape, dtype=np.int32)
    boxes = []
    for y in range(sh):
        for x in range(sw):
            if not small[y, x] or labels[y, x]:
                continue
            label = len(boxes) + 1
            queue = deque([(y, x)])
            labels[y, x] = label
            x0 = x1 = x
            y0 = y1 = y
            while queue:
                cy, cx = queue.popleft()
                x0, x1, y0, y1 = min(x0, cx), max(x1, cx), min(y0, cy), max(y1, cy)
                for ny in range(cy - 1, cy + 2):
                    for nx in range(cx - 1, cx + 2):
                        if 0 <= ny < sh and 0 <= nx < sw and small[ny, nx] and not labels[ny, nx]:
                            labels[ny, nx] = label
                            queue.append((ny, nx))
            boxes.append((x0 * downscale, y0 * downscale, (x1 + 1) * downscale, (y1 + 1) * downscale))
    return boxes


def cut_sheet(key, filename, scale, manifest):
    img = Image.open(SOURCE / "Mossy Assets" / "Mossy Tileset" / filename).convert("RGBA")
    alpha = np.array(img)[:, :, 3]
    boxes = components(alpha)
    boxes = [b for b in boxes if (b[2] - b[0]) * (b[3] - b[1]) >= 64 * 64]
    boxes.sort(key=lambda b: (b[1] // 256, b[0]))
    for i, b in enumerate(boxes):
        piece = img.crop(b)
        size = (max(1, round(piece.width * scale)), max(1, round(piece.height * scale)))
        piece = piece.resize(size, Image.LANCZOS)
        name = f"{key}_{i:02d}"
        piece.save(OUT / "pieces" / f"{name}.png", optimize=True)
        manifest["pieces"][name] = {"width": size[0], "height": size[1]}
    print(f"{key}: {len(boxes)} pieces")


def main():
    (OUT / "pieces").mkdir(parents=True, exist_ok=True)
    manifest = {"animations": {}, "pieces": {}}

    wizard_images = []
    for folder in ANIMATIONS.values():
        wizard_images += [Image.open(f).convert("RGBA") for f in frames_in(folder)]
    wizard_box = union_bbox(wizard_images)
    for name, folder in ANIMATIONS.items():
        pack_animation(name, frames_in(folder), 128, 20, manifest, wizard_box)

    plants = SOURCE / "Plant Animations"
    for name, folder in PLANTS.items():
        files = frames_in(plants / folder)
        step = 2 if len(files) > 40 else 1
        pack_animation(name, files, 160, 45, manifest, None, step)

    scales = {"tiles": 0.25, "platforms": 0.25, "decor": 0.25, "hanging": 0.25, "hills": 0.25, "background": 0.25}
    for key, filename in SHEETS.items():
        cut_sheet(key, filename, scales[key], manifest)

    (OUT / "manifest.json").write_text(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
