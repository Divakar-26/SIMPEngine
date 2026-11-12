#!/usr/bin/env python3
"""Generates Editor/ScriptEditor/LuaSignatureData.inc.

Sources, in priority order for parameter names:
  1. docs/SCRIPTING.md and docs/ANIMATION.md (names and optional markers)
  2. the C++ binding code (argument count, kinds, which are optional)
  3. the table of standard Lua library signatures below

Run from the repository root:  python3 Editor/ScriptEditor/tools/gen_signatures.py
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
SCRIPTING = ROOT / "Engine/Engine/Scripting"
import os
OUT = Path(os.environ.get("SIGNATURE_OUT", str(ROOT / "Editor/ScriptEditor/LuaSignatureData.inc")))

GLOBAL_TABLES = {
    "LogFunctions": "log", "InputFunctions": "Input", "TimeFunctions": "Time", "SceneFunctions": "Scene",
    "WindowFunctions": "Window", "MusicFunctions": "Music", "GameFunctions": "Game", "AudioFunctions": "Audio",
    "MathFunctions": "Math", "DrawFunctions": "Draw", "TweenFunctions": "Tween", "CurveFunctions": "Curves",
    "PathFunctions": "Path", "PhysicsFunctions": "Physics", "InputActionFunctions": "Input", "ColorFunctions": "Color", "EffectFunctions": "Effect",
}

STD = {
    "print": "...", "type": "value", "pairs": "table", "ipairs": "table", "next": "table, key?", "select": "index, ...",
    "tostring": "value", "tonumber": "value, base?", "pcall": "fn, ...", "xpcall": "fn, handler, ...", "error": "message, level?",
    "assert": "value, message?", "rawget": "table, key", "rawset": "table, key, value", "rawequal": "a, b", "rawlen": "value",
    "setmetatable": "table, metatable", "getmetatable": "value", "require": "module", "load": "chunk, chunkName?, mode?, env?",
    "loadfile": "filename?, mode?, env?", "dofile": "filename?", "collectgarbage": "option?, arg?", "warn": "message, ...",
    "math.abs": "x", "math.acos": "x", "math.asin": "x", "math.atan": "y, x?", "math.ceil": "x", "math.cos": "x", "math.deg": "x",
    "math.exp": "x", "math.floor": "x", "math.fmod": "x, y", "math.log": "x, base?", "math.max": "x, ...", "math.min": "x, ...",
    "math.modf": "x", "math.rad": "x", "math.random": "m?, n?", "math.randomseed": "x?, y?", "math.sin": "x", "math.sqrt": "x",
    "math.tan": "x", "math.tointeger": "x", "math.type": "x", "math.ult": "m, n", "math.pow": "x, y", "math.cosh": "x",
    "math.sinh": "x", "math.tanh": "x", "math.log10": "x", "math.frexp": "x", "math.ldexp": "m, e",
    "string.byte": "s, i?, j?", "string.char": "...", "string.dump": "fn, strip?", "string.find": "s, pattern, init?, plain?",
    "string.format": "format, ...", "string.gmatch": "s, pattern, init?", "string.gsub": "s, pattern, replacement, n?",
    "string.len": "s", "string.lower": "s", "string.match": "s, pattern, init?", "string.pack": "format, ...",
    "string.packsize": "format", "string.rep": "s, n, separator?", "string.reverse": "s", "string.sub": "s, i, j?",
    "string.unpack": "format, s, position?", "string.upper": "s",
    "table.concat": "list, separator?, i?, j?", "table.insert": "list, position?, value", "table.move": "source, from, to, target, destination?",
    "table.pack": "...", "table.remove": "list, position?", "table.sort": "list, comparator?", "table.unpack": "list, i?, j?",
    "os.clock": "", "os.date": "format?, time?", "os.difftime": "t2, t1", "os.execute": "command?", "os.exit": "code?, close?",
    "os.getenv": "name", "os.remove": "filename", "os.rename": "oldname, newname", "os.setlocale": "locale?, category?",
    "os.time": "table?", "os.tmpname": "",
    "io.close": "file?", "io.flush": "", "io.input": "file?", "io.lines": "filename?, ...", "io.open": "filename, mode?",
    "io.output": "file?", "io.popen": "program, mode?", "io.read": "...", "io.tmpfile": "", "io.type": "object", "io.write": "...",
    "debug.debug": "", "debug.gethook": "thread?", "debug.getinfo": "fn, what?", "debug.getlocal": "fn, index",
    "debug.getmetatable": "value", "debug.getregistry": "", "debug.getupvalue": "fn, index", "debug.getuservalue": "userdata, n?",
    "debug.sethook": "hook, mask, count?", "debug.setlocal": "level, index, value", "debug.setmetatable": "value, metatable",
    "debug.setupvalue": "fn, index, value", "debug.setuservalue": "userdata, value, n?", "debug.traceback": "message?, level?",
    "debug.upvalueid": "fn, n", "debug.upvaluejoin": "fn1, n1, fn2, n2", "debug.setcstacklimit": "limit",
    "coroutine.close": "co", "coroutine.create": "fn", "coroutine.isyieldable": "co?", "coroutine.resume": "co, ...",
    "coroutine.running": "", "coroutine.status": "co", "coroutine.wrap": "fn", "coroutine.yield": "...",
    "utf8.char": "...", "utf8.codepoint": "s, i?, j?, lax?", "utf8.codes": "s, lax?", "utf8.len": "s, i?, j?, lax?",
    "utf8.offset": "s, n, i?", "package.loadlib": "libname, funcname", "package.searchpath": "name, path, separator?, replacement?",
    "table.create": "size?, hashSize?",
}

KINDS = {"number": "number", "integer": "number", "string": "string", "lstring": "string", "any": "value", "boolean": "bool"}


def read(path):
    return path.read_text(encoding="utf-8", errors="replace")


def brace_body(text, start):
    depth = 0
    for i in range(start, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[start:i + 1]
    return text[start:]


def parse_cpp():
    funcs = {}
    arrays = []
    for path in sorted(SCRIPTING.rglob("*.cpp")):
        text = read(path)
        metas = dict(re.findall(r'constexpr const char \*(\w+)Meta\s*=\s*"SIMP\.([A-Za-z]+)"', text))
        for m in re.finditer(r"\bint\s+(\w+)\s*\(\s*lua_State\s*\*\s*\w*\s*\)\s*\{", text):
            body = brace_body(text, m.end() - 1)
            funcs.setdefault(m.group(1), (path.name, body))
        for m in re.finditer(r"const luaL_Reg\s+(\w+)\[\]\s*=\s*\{(.*?)\{\s*nullptr\s*,\s*nullptr\s*\}\s*\}\s*;", text, re.S):
            name = m.group(1)
            entries = [(a, re.split(r"<", b)[0].strip()) for a, b in re.findall(r'\{\s*"([^"]+)"\s*,\s*([^{}]+?)\s*\}', m.group(2))]
            if name in GLOBAL_TABLES:
                owner, sep = GLOBAL_TABLES[name], "."
            elif name.endswith("Methods"):
                base = name[:-7]
                if base == "Entity":
                    owner, sep = "Entity", ":"
                elif base in metas:
                    owner, sep = metas[base], ":"
                elif base == "Controller" and "Controller" in metas:
                    owner, sep = metas["Controller"], ":"
                else:
                    continue
            else:
                continue
            arrays.append((owner, sep, entries, path.name))
    return funcs, arrays


def analyze(body, method):
    first = 2 if method else 1
    seen = {}
    explicit = set()

    def note(index, kind, required, optional=False):
        if index < first:
            return
        if optional:
            explicit.add(index)
        cur = seen.get(index)
        if cur is None:
            seen[index] = [kind, required]
        else:
            if cur[0] == "value" and kind != "value":
                cur[0] = kind
            cur[1] = cur[1] or required

    for kind, idx in re.findall(r"luaL_check(number|integer|lstring|string|any)\s*\(\s*L\s*,\s*(\d+)", body):
        note(int(idx), KINDS[kind], True)
    for kind, idx in re.findall(r"luaL_opt(number|integer|lstring|string)\s*\(\s*L\s*,\s*(\d+)", body):
        note(int(idx), KINDS[kind], False, True)
    for typ, idx in re.findall(r"luaL_checktype\s*\(\s*L\s*,\s*(\d+)\s*,\s*LUA_T(\w+)", body):
        pass
    for idx, typ in re.findall(r"luaL_checktype\s*\(\s*L\s*,\s*(\d+)\s*,\s*LUA_T(\w+)", body):
        note(int(idx), {"TABLE": "table", "FUNCTION": "function"}.get(typ, "value"), True)
    for idx in re.findall(r"lua_toboolean\s*\(\s*L\s*,\s*(\d+)", body):
        note(int(idx), "bool", False)
    for idx in re.findall(r"lua_isnoneornil\s*\(\s*L\s*,\s*(\d+)", body):
        note(int(idx), "value", False, True)
    for idx in re.findall(r"CheckLiveEntity\s*\(\s*L\s*,\s*(\d+)", body):
        note(int(idx), "entity", True)
    for idx in re.findall(r"ToByte\s*\(\s*L\s*,\s*(\d+)", body):
        note(int(idx), "number", True)
    for idx in re.findall(r"ToByteOpt\s*\(\s*L\s*,\s*(\d+)", body):
        note(int(idx), "number", False, True)
    for idx in re.findall(r"ReadRgba\s*\(\s*L\s*,\s*(\d+)", body):
        i = int(idx)
        for k, n in enumerate(("r", "g", "b", "a")):
            note(i + k, "color:" + n, k < 3, k >= 3)
    for idx in re.findall(r"lua_(?:istable|isfunction|isstring|isnumber|isboolean)\s*\(\s*L\s*,\s*(\d+)", body):
        note(int(idx), "value", False)
    for idx in re.findall(r"lua_(?:type|tonumber|tolstring|tostring|tointeger|isinteger)\s*\(\s*L\s*,\s*(\d+)", body):
        note(int(idx), "value", False)
    skip = ("lua_", "luaL_", "CheckLiveEntity", "ToByte", "ToByteOpt", "ReadRgba", "PushEntity", "PushHandle", "PushRgba", "RequireScene")
    for name, idx in re.findall(r"\b([A-Za-z_]\w*)\s*(?:<[^<>]*>)?\s*\(\s*L\s*,\s*(\d+)\s*[,)]", body):
        if name.startswith(skip) or name in ("if", "for", "while", "return", "sizeof"):
            continue
        lower = name.lower()
        kind = "value"
        if any(t in lower for t in ("num", "float", "int", "byte", "double", "scalar")):
            kind = "number"
        elif any(t in lower for t in ("str", "text", "name")):
            kind = "string"
        elif "bool" in lower:
            kind = "bool"
        elif "path" in lower:
            kind = "path"
        elif "table" in lower or "points" in lower:
            kind = "table"
        elif "entity" in lower:
            kind = "entity"
        elif "curve" in lower:
            kind = "curve"
        note(int(idx), kind, "opt" not in lower, "opt" in lower)
    return [(i, *seen[i], i in explicit) for i in sorted(seen)]


def cpp_params(params, first):
    out = []
    if not params:
        return out
    last = params[-1][0]
    table = {p[0]: p for p in params}
    for i in range(first, last + 1):
        if i in table:
            _, kind, required, explicit = table[i]
            out.append((kind, required and not explicit, explicit))
        else:
            out.append(("value", False, False))
    return out


def split_args(text):
    parts, depth, cur = [], 0, ""
    for ch in text:
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
        if ch == "," and depth == 0:
            parts.append(cur)
            cur = ""
        else:
            cur += ch
    if cur.strip():
        parts.append(cur)
    return parts


def parse_doc_args(text):
    text = text.strip()
    if not text:
        return []
    result = []
    depth = 0
    token = ""
    optional_depth = []

    def flush(opt):
        nonlocal token
        name = token.strip()
        token = ""
        if not name:
            return
        name = re.split(r"\s*=\s*", name)[0].strip()
        if name.startswith("{") or name.startswith("("):
            name = "table"
        name = re.split(r"\s+or\s+", name)[0].strip()
        name = re.sub(r"[^A-Za-z0-9_.]", "", name)
        if name:
            result.append((name, opt))

    i = 0
    brace = 0
    while i < len(text):
        ch = text[i]
        if ch == "[" and brace == 0:
            flush(depth > 0)
            depth += 1
        elif ch == "]" and brace == 0:
            flush(depth > 0)
            depth -= 1
        elif ch == "{":
            brace += 1
            token += ch
        elif ch == "}":
            brace -= 1
            token += ch
        elif ch == "," and brace == 0:
            flush(depth > 0)
        else:
            token += ch
        i += 1
    flush(depth > 0)
    return result


def is_example(args):
    return bool(re.search(r'["\']|\{\s*\w+\s*=', args))


class Entries(dict):
    def put(self, key, args):
        if key not in self or (is_example(self[key]) and not is_example(args)):
            self[key] = args


def parse_docs():
    entries = Entries()
    owners_cell = re.compile(r"^\|\s*(.+?)\s*\|")
    entity_section = False
    table_sep = "*"
    for fname in ("SCRIPTING.md", "ANIMATION.md"):
        text = read(ROOT / "docs" / fname)
        in_fence = False
        for line in text.split("\n"):
            if line.strip().startswith("```"):
                in_fence = not in_fence
                continue
            if in_fence:
                continue
            if line.startswith("## "):
                entity_section = line.strip() == "## Entity methods"
            for span in re.findall(r"`([^`]+)`", line):
                for m in re.finditer(r"\b([A-Z][A-Za-z]*)([.:])([A-Za-z_]\w*)\(((?:[^()]|\([^()]*\))*)\)", span):
                    entries.put((m.group(1), m.group(2), m.group(3)), m.group(4))
            if line.startswith("| Global"):
                table_sep = "."
            elif line.startswith("| Handle"):
                table_sep = ":"
            elif line.startswith("| Method") or line.startswith("| Component |") or line.startswith("| Flutter"):
                table_sep = "*"
            row = owners_cell.match(line)
            if not row or line.startswith("|---"):
                continue
            cells = [c.strip() for c in line.strip().strip("|").split("|")]
            if len(cells) < 2:
                continue
            first = cells[0]
            if entity_section:
                owners = [("Entity", ":")]
                span_cells = [first]
            else:
                names = [n for n in re.findall(r"`([^`]+)`", first)] or [first]
                owners = []
                for n in names:
                    n = n.strip()
                    if re.match(r"^[A-Za-z]+$", n):
                        owners.append((n, None))
                span_cells = cells[1:]
            if not owners:
                continue
            for cell in span_cells:
                for fm in re.finditer(r"`([A-Za-z_]\w*)\(([^`]*?)\)`", cell):
                    for owner, sep in owners:
                        entries.put((owner, sep or table_sep, fm.group(1)), fm.group(2))
    return entries


def make_params(cpp, doc, method):
    count_cpp = len(cpp)
    names = None
    if doc is not None:
        parsed = parse_doc_args(doc)
        flat = [(n, o) for n, o in parsed]
        if not cpp and not flat:
            return []
        if cpp and len(flat) == count_cpp:
            names = flat
        elif flat and len(flat) > count_cpp:
            names = flat
            cpp = cpp + [(None, False, False)] * (len(flat) - len(cpp))
        elif flat and 0 < len(flat) < count_cpp:
            extra = [(f"arg{i + 1}", True) for i in range(len(flat), count_cpp)]
            names = flat + [(n, explicit) for (n, _), (_, _, explicit) in zip(extra, cpp[len(flat):])]
        elif not cpp and flat:
            names = flat
    result = []
    if names:
        for (name, optional), (kind, required, explicit) in zip(names, cpp or [(None, False, False)] * len(names)):
            result.append(name + ("?" if optional or explicit else ""))
        return result
    color_names = {"color:r": "r", "color:g": "g", "color:b": "b", "color:a": "a"}
    counter = {}
    for kind, required, explicit in cpp:
        if kind in color_names:
            name = color_names[kind]
        else:
            counter[kind] = counter.get(kind, 0) + 1
            name = kind if counter[kind] == 1 else f"{kind}{counter[kind]}"
        result.append(name + ("?" if explicit or (not required and kind not in color_names) or name == "a" else ""))
    return result


GENERIC = re.compile(r"^(value|number\d?|string\d?|bool\d?|table|entity|function|path|curve|arg\d+)(\?)?$")

OVERRIDES = {
    "Physics.SetGravity": "x, y", "Physics.SetSubsteps": "count", "Physics.SetIterations": "velocity, position?",
    "Physics.SetSleepEnabled": "enabled", "Physics.Raycast": "originX, originY, dirX, dirY, maxDistance?",
    "Physics.QueryPoint": "x, y", "Physics.QueryBox": "minX, minY, maxX, maxY", "Physics.QueryCircle": "x, y, radius",
    "Physics.CreateDistanceJoint": "a, b, ax?, ay?, bx?, by?, length?", "Physics.CreateRevoluteJoint": "a, b, x, y",
    "Physics.CreateWheelJoint": "a, b, x, y, axisX?, axisY?", "Physics.CreateWeldJoint": "a, b",
    "Body:CreateBox": "width, height, mass?", "Body:CreateCircle": "radius, mass?", "Body:CreateCapsule": "halfLength, radius, mass?",
    "Body:CreatePolygon": "sides, radius, mass?", "Body:CreatePoints": "points, cornerRadius?, mass?",
    "Body:SetVelocity": "x, y", "Body:AddImpulse": "x, y", "Body:AddImpulseAt": "impulseX, impulseY, pointX, pointY",
    "Body:AddForce": "x, y", "Body:AddTorque": "torque", "Body:SetSpin": "radiansPerSecond", "Body:Teleport": "x, y, degrees?",
    "Body:SetMass": "mass", "Body:SetBounce": "bounce", "Body:SetFriction": "static, moving?", "Body:SetDamping": "linear, angular?",
    "Body:SetSensor": "enabled", "Body:SetOneWay": "enabled, normalX?, normalY?", "Body:SetLocked": "rotation, position?",
    "Body:SetIgnoreGravity": "enabled", "Body:SetCanSleep": "enabled", "Body:SetLayer": "layer", "Body:SetMask": "bits",
    "Body:SetCollidesWith": "...", "Body:SetGroup": "group",
    "Joint:SetCollideConnected": "enabled", "Joint:SetLength": "length", "Joint:SetSoftness": "compliance",
    "Joint:SetLimits": "lower, upper", "Joint:SetMotor": "speed, maxTorque", "Joint:SetSpring": "hertz, damping?",
    "Text:SetAlign": "align", "Curves.Elastic": "mode, period?", "Timer:SetRepeat": "repeating", "Tween.IsTweening": "target, property?",
    "Vector:SetCircle": "radius", "Vector:SetHeart": "size", "Vector:SetFillEnabled": "enabled", "Vector:SetStrokeEnabled": "enabled",
    "Vector:SetMorph": "amount", "Vector:SetMorphTarget": "path", "Vector:SetCap": "cap", "Vector:SetStrokeWidth": "width",
    "CharacterController:SetMode": "mode", "WorldBounds:SetMode": "mode", "Audio.SetBusVolume": "bus, volume",
    "Audio.SetVolume": "voice, volume", "Audio.SetMasterVolume": "volume", "Math.Clamp": "value, min, max", "Math.Sign": "value",
    "VectorPath:MoveTo": "x, y", "VectorPath:LineTo": "x, y", "VectorPath:QuadTo": "cx, cy, x, y",
    "VectorPath:CubicTo": "c1x, c1y, c2x, c2y, x, y", "VectorPath:ArcTo": "cx, cy, radius, startDeg, endDeg",
    "VectorPath:Append": "path", "VectorPath:Translate": "dx, dy", "VectorPath:Scale": "sx, sy?",
    "VectorPath:Rotate": "degrees", "VectorPath:GetPointAt": "t",
    "Vector:SetDash": "length, gap?, offset?", "Vector:SetGradient": "x1, y1, x2, y2, r1, g1, b1, a1, r2, g2, b2, a2",
    "Vector:SetJoin": "join, miterLimit?", "Vector:SetPath": "path", "Vector:SetPolygon": "sides, radius",
    "Vector:SetRect": "width, height, radius?", "Vector:SetStar": "points, outer, inner?",
    "Vector:SetSvg": "source, fitSize?, applyStyle?", "Vector:SetTrim": "start, end, offset?",
    "Vector:MorphTo": "path, seconds, curve?, options?",
    "Tween:Curve": "curve", "Tween:Delay": "seconds", "Tween:OnComplete": "fn", "Tween:OnRepeat": "fn", "Tween:OnStart": "fn",
    "Tween:OnUpdate": "fn", "Tween:Then": "fn", "Tween:Repeat": "count?", "Tween:Seek": "t", "Tween:Speed": "speed", "Tween:Yoyo": "enabled?",
    "Tween.To": "target, props, seconds, curve?, options?", "Tween.From": "target, props, seconds, curve?, options?",
    "Tween.FromTo": "target, from, to, seconds, curve?, options?", "Tween.Value": "from, to, seconds, curve, fn, options?",
    "Tween.Spring": "target, props, options?", "Tween.Sequence": "items, options?", "Tween.Parallel": "items, options?",
    "Tween.Kill": "target, property?", "Tween.Stagger": "targets, props, seconds, curve?, each?, options?",
    "Draw.Circle": "x, y, radius, r, g, b, a?", "Draw.Ring": "x, y, radius, thickness, r, g, b, a?",
    "Draw.Rect": "x, y, w, h, r, g, b, a?, rotation?", "Draw.RoundRect": "x, y, w, h, radius, r, g, b, a?",
    "Draw.Polyline": "points, width, r, g, b, a?, closed?", "Draw.Bezier": "x0, y0, c1x, c1y, c2x, c2y, x1, y1, width, r, g, b, a?",
    "Draw.Text": "text, x, y, size, r, g, b, a?, align?", "Path.Points": "points, closed?",
    "Input.IsKeyDown": "key", "Input.IsKeyJustPressed": "key", "Input.IsKeyJustReleased": "key",
    "Input.GetAxis": "action | negativeKey, positiveKey", "Input.IsMouseDown": "button", "Input.IsMouseJustPressed": "button", "Input.IsMouseJustReleased": "button",
    "Game.Has": "name", "Game.Remove": "name", "Game.Set": "name, value", "Scene.SetLighting": "enabled",
    "Entity:SetScript": "path", "Trigger:Contains": "entity", "Curve:Transform": "t",
    "Curves.Cubic": "x1, y1, x2, y2", "Curves.Interval": "begin, end, curve?",
    "Draw.Line": "x1, y1, x2, y2, width, r, g, b, a?", "Draw.Polygon": "points, r, g, b, a?",
    "Draw.Path": "path, x, y, options?", "Tween.Along": "target, path, seconds, curve?, options?",
    "Draw.Dashed": "x1, y1, x2, y2, width, dash, gap, color, progress?", "Draw.Arrow": "x1, y1, x2, y2, width, color, head?",
    "Draw.MeasureText": "text, size, fontPath?", "Draw.Label": "text, x, centerY, size, color, align?",
    "Math.Clamp01": "value", "Math.Progress": "t, from, to", "Math.Remap": "value, inMin, inMax, outMin, outMax, clamp?",
    "Math.Smoothstep": "edge0, edge1, x", "Math.Window": "t, from, to, fade?",
    "Color.New": "r, g, b, a?", "Color.Hex": "hex, alpha?", "Color.Mix": "from, to, t", "Color.WithAlpha": "color, alpha",
    "Color.Unpack": "color, alpha?", "Color.Define": "name, color", "Color.Names": "", "Color.Get": "color",
    "Effect.Play": "target, name, options?", "Effect.Names": "category?", "Effect.Categories": "", "Effect.Info": "name",
}

LUA_DEFINED = {
    "Draw.Layer": "z, fn",
    "Timeline.New": "segments?", "Timeline.Add": "name, seconds", "Timeline.Span": "name", "Timeline.Progress": "t, name",
    "Timeline.Active": "t, name", "Timeline.Current": "t", "Timeline.Total": "",
    "CameraRig.New": "timeline, start?", "CameraRig.Move": "segment, pose, curve?, options?", "CameraRig.Track": "segment, fn",
    "CameraRig.Sway": "x?, y?, zoom?", "CameraRig.At": "t", "CameraRig.Apply": "t, camera?",
    "Captions.New": "timeline, options?", "Captions.Add": "from, to, title, text?", "Captions.Visible": "t",
    "Captions.Draw": "t, x?, y?, scale?",
    "Suggest.Closest": "name, candidates", "Suggest.Hint": "name, candidates", "Math.Integrate": "fn, from, to, steps?",
    "Timeline.Window": "t, name, fade?", "Timeline.Eased": "t, name, curve?",
    "Axes.New": "options", "Axes.P": "x, y", "Axes.Draw": "progress?, zoom?", "Axes.Plot": "fn, from, to, options?",
    "Axes.Area": "fn, from, to, options?", "Axes.Riemann": "fn, from, to, options?", "Axes.Tracer": "fn, x, options?",
    "Plot.Points": "xr?", "Plot.Value": "x", "Plot.At": "x", "Plot.Draw": "xr?, zoom?",
    "Area.Polygon": "xr?", "Area.Draw": "xr?, alpha?",
    "Video.Step": "t, dt", "Effect.Chain": "target, list, options?", "Effect.Stagger": "targets, name, each?, options?",
    "Backdrop.Fill": "x, y, zoom?, color?", "Backdrop.Grid": "x, y, zoom?, options?", "Path.TextGlyphs": "text, size, align?, fontPath?",
    "Glyphs.New": "text, size, align?, fontPath?", "Glyphs.Count": "", "Glyphs.Bounds": "", "Glyphs.Size": "", "Glyphs.Draw": "x, y, progress?, options?",
    "Riemann.Sum": "n", "Riemann.Rects": "n, options?", "Riemann.Draw": "n, options?",
}


def lower_camel(text):
    return text[:1].lower() + text[1:]


def polish(key, params):
    if key in OVERRIDES:
        return OVERRIDES[key]
    parts = [p.strip() for p in params.split(",")] if params else []
    member_name = re.split(r"[.:]", key)[-1]
    if len(parts) == 1 and re.match(r"^(v|s|n)\??$", parts[0]) and member_name.startswith("Set") and len(member_name) > 3:
        return lower_camel(member_name[3:]) + ("?" if parts[0].endswith("?") else "")
    if len(parts) == 1 and GENERIC.match(parts[0]):
        member = re.split(r"[.:]", key)[-1]
        mark = "?" if parts[0].endswith("?") else ""
        if member.startswith("Set") and len(member) > 3:
            return lower_camel(member[3:]) + mark
        if member.startswith("Is") and len(member) > 2:
            return lower_camel(member[2:]) + mark
        if member.startswith("Add") and len(member) > 3:
            return lower_camel(member[3:]) + mark
    return params


def main():
    funcs, arrays = parse_cpp()
    docs = parse_docs()

    generated = {}
    for owner, sep, entries, filename in arrays:
        for lua_name, cname in entries:
            info = funcs.get(cname)
            method = sep == ":"
            first = 2 if method else 1
            if info:
                cpp = cpp_params(analyze(info[1], method), first)
            elif lua_name.startswith("Set"):
                cpp = [("value", True, False)]
            else:
                cpp = []
            doc = docs.get((owner, sep, lua_name))
            if doc is None:
                doc = docs.get((owner, "*", lua_name))
            if doc is None and owner == "Entity" and sep == ":":
                doc = docs.get(("Entity", ":", lua_name))
            params = make_params(cpp, doc, method)
            generated[f"{owner}{sep}{lua_name}"] = polish(f"{owner}{sep}{lua_name}", ", ".join(params))

    for typ_owner in ("Sprite", "Camera", "Light", "Velocity", "Collision", "Shape", "Text", "Glow", "Particles", "Audio", "Parallax", "Body", "Trigger",
                      "Timer", "Path", "Health", "Tilemap", "Animation", "Vector", "CharacterController", "Damage", "Oscillator", "Spawner", "Follow",
                      "Pickup", "Checkpoint", "Respawn", "Music", "Pickable", "Button", "Teleporter", "CameraZone", "WorldBounds",
                      "Canvas", "UIRect", "UIPanel", "UILabel", "UIImage", "UIButton", "UILayout", "ShaderCanvas"):
        generated.setdefault(f"Entity:Get{typ_owner}", "")

    for key, params in STD.items():
        generated[key] = params
    for key, params in LUA_DEFINED.items():
        generated[key] = params

    lines = ["// Generated by Editor/ScriptEditor/tools/gen_signatures.py. Do not edit by hand."]
    for key in sorted(generated):
        lines.append('{"%s", "%s"},' % (key, generated[key].replace('"', '\\"')))
    OUT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"wrote {len(generated)} signatures to {OUT}")


if __name__ == "__main__":
    main()
