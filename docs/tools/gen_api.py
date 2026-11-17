#!/usr/bin/env python3
"""Fills the generated parts of docs/API.md: every Lua function (from the script editor's
signature table) and the Motion preset catalogue (from Engine/Animation/MotionPresets.cpp)."""
import re
from collections import OrderedDict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DOC = ROOT / "docs/API.md"
SIGS = ROOT / "Editor/ScriptEditor/LuaSignatureData.inc"
MOTION = ROOT / "Engine/Engine/Animation/MotionPresets.cpp"
SKIP = {"math", "string", "table", "os", "io", "debug", "coroutine", "utf8", "package"}


def api_block():
    groups = OrderedDict()
    for callee, params in re.findall(r'^\{"([^"]+)", "([^"]*)"\}', SIGS.read_text(), re.M):
        sep = ":" if ":" in callee else "."
        if sep not in callee:
            continue
        owner, name = callee.split(sep, 1)
        if owner in SKIP:
            continue
        groups.setdefault((owner, sep), []).append(f"{name}({params})")
    tables = [k for k in groups if k[1] == "."]
    handles = [k for k in groups if k[1] == ":"]
    lines = []
    for title, keys in (("Global tables", tables), ("Handles (methods)", handles)):
        lines.append(f"**{title}**\n")
        for owner, sep in sorted(keys, key=lambda k: k[0].lower()):
            lines.append(f"- `{owner}{sep}` " + " · ".join(groups[(owner, sep)]))
        lines.append("")
    return "\n".join(lines).rstrip() + "\n"


def motion_block():
    src = MOTION.read_text()
    shapes = re.findall(r'\{"(\w+)", \[\]\(float r\)', src)
    styles = re.findall(r'\{"(\w+)", "([^"]+)", [\d.]+f\},', src)
    shape_row = f"`<Shape><Style>` with shape {', '.join(shapes)} and style " + ", ".join(f"{s} ({d})" for s, d in styles)
    by_cat = OrderedDict()
    for name, cat, desc in re.findall(r'Add\(list, "(\w+)", "(\w+)", "([^"]+)"', src):
        by_cat.setdefault(cat, []).append((name, desc))
    for name, frm, to in re.findall(r'\{"(Morph\w+)", (\d), (\d)\}', src):
        by_cat.setdefault("Morph", []).append((name, f"{shapes[int(frm)]} morphs into a {shapes[int(to)]}"))
    families = re.findall(r'\{"(\w+)", "([^"]+)", [\d.]+f, (true|false)\},', src)
    exits = ", ".join(f"{s} ({d})" for s, d, e in families if e == "true")
    loops = ", ".join(f"{s} ({d})" for s, d, e in families if e == "false")
    lines = ["| Category | Presets |", "|---|---|", f"| Shapes | {shape_row} |",
             f"| Exits | `<Shape><Style>`, same shapes, style {exits} |", f"| Loops | `<Shape><Style>`, same shapes, style {loops} |"]
    for cat, items in by_cat.items():
        lines.append(f"| {cat} | " + "; ".join(f"**{n}** {d.rstrip('.')}" for n, d in items) + " |")
    return "\n".join(lines) + "\n"


def replace(text, tag, body):
    return re.sub(rf"(<!-- {tag}:BEGIN -->\n).*?(<!-- {tag}:END -->)", lambda m: m.group(1) + body + m.group(2), text, flags=re.S)


def main():
    text = DOC.read_text()
    text = replace(text, "API", api_block())
    text = replace(text, "MOTION", motion_block())
    DOC.write_text(text)
    print(f"updated {DOC.relative_to(ROOT)} ({len(text.splitlines())} lines)")


if __name__ == "__main__":
    main()
