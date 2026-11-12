#!/usr/bin/env python3
"""Pack a SIMPEngine project into the Android app's assets.

    python3 Android/tools/pack_project.py <project folder> [--label "My Game"]

Copies the project (project.simp, assets/, input.yaml, ...) and the engine fonts into
Android/app/src/main/assets/files and writes simp_files.txt, the list the player unpacks
into the app's private storage at first start (and again whenever the list changes).
"""
import argparse
import re
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
APP = ROOT / "Android" / "app" / "src" / "main"
SKIP_DIRS = {".git", "build", ".idea", "__pycache__"}
SKIP_SUFFIXES = {".saving", ".sav"}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("project", type=Path)
    parser.add_argument("--label", help="name shown under the app icon (default: the project name)")
    args = parser.parse_args()

    project = args.project.resolve()
    if not (project / "project.simp").is_file():
        sys.exit(f"{project} has no project.simp")

    target = APP / "assets"
    if target.exists():
        shutil.rmtree(target)
    files = target / "files"
    files.mkdir(parents=True)

    listed = []

    def copy(source: Path, relative_root: Path, prefix: str = ""):
        for path in sorted(source.rglob("*")):
            relative = path.relative_to(source)
            if any(part in SKIP_DIRS for part in relative.parts) or not path.is_file():
                continue
            if path.suffix in SKIP_SUFFIXES or path.name == ".simpbuild":
                continue
            destination = files / prefix / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, destination)
            listed.append((Path(prefix) / relative).as_posix())

    copy(project, project)
    copy(ROOT / "assets" / "fonts", ROOT, "engine/fonts")

    (target / "simp_files.txt").write_text("\n".join(sorted(listed)) + "\n")

    name = args.label
    if not name:
        match = re.search(r"^\s*name:\s*(.+)$", (project / "project.simp").read_text(), re.M)
        name = match.group(1).strip().strip("'\"") if match else "Game"
    orientation = re.search(r"^\s*orientation:\s*(\w+)", (project / "project.simp").read_text(), re.M)
    value = {"portrait": "sensorPortrait", "auto": "fullSensor"}.get(orientation.group(1) if orientation else "", "sensorLandscape")
    (ROOT / "Android" / "simp.properties").write_text(f"orientation={value}\n")
    strings = APP / "res" / "values" / "strings.xml"
    strings.write_text(f'<resources>\n    <string name="app_name">{name}</string>\n</resources>\n')
    print(f"packed {len(listed)} files from {project} as '{name}'")


if __name__ == "__main__":
    main()
