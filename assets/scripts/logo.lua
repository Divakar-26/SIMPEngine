local Logo = {}

local CREAM = {255, 249, 214}
local BLUSH = {255, 220, 220}
local ROSE = {216, 162, 162}
local SAGE = {142, 166, 107}
local DEEP = {172, 108, 108}
local MOSS = {104, 126, 76}

local PEN_SPEED = 820
local STROKE = 30
local BASELINE = -30

local GLYPHS = {
    {advance = 162, flowers = {{1, {120, 184}, 30}}, strokes = {
        {{"M", 108, 168}, {"C", 95, 192, 76, 200, 58, 200}, {"C", 26, 200, 8, 180, 8, 152}, {"C", 8, 118, 40, 108, 62, 100},
         {"C", 88, 90, 112, 80, 112, 48}, {"C", 112, 18, 90, 0, 58, 0}, {"C", 32, 0, 12, 12, 4, 34}},
    }},
    {advance = 72, strokes = {
        {{"M", 10, 200}, {"L", 10, 0}},
    }},
    {advance = 196, strokes = {
        {{"M", 0, 0}, {"L", 0, 200}, {"L", 70, 44}, {"L", 140, 200}, {"L", 140, 0}},
    }},
    {advance = 166, strokes = {
        {{"M", 0, 0}, {"L", 0, 200}, {"L", 55, 200}, {"C", 95, 200, 115, 180, 115, 148}, {"C", 115, 116, 95, 96, 55, 96}, {"L", 0, 96}},
    }},
    {advance = 156, flowers = {{2, {104, 100}, 20}}, strokes = {
        {{"M", 105, 200}, {"L", 0, 200}, {"L", 0, 0}, {"L", 105, 0}},
        {{"M", 0, 100}, {"L", 84, 100}},
    }},
    {advance = 150, strokes = {
        {{"M", 0, 0}, {"L", 0, 130}, {"L", 0, 100}, {"C", 10, 122, 30, 132, 52, 132}, {"C", 85, 132, 100, 112, 100, 85}, {"L", 100, 0}},
    }},
    {advance = 150, flowers = {{2, {-10, -60}, 26}}, strokes = {
        {{"M", 100, 66}, {"C", 100, 93.6, 77.6, 116, 50, 116}, {"C", 22.4, 116, 0, 93.6, 0, 66},
         {"C", 0, 38.4, 22.4, 16, 50, 16}, {"C", 77.6, 16, 100, 38.4, 100, 66}},
        {{"M", 100, 130}, {"L", 100, -20}, {"C", 100, -58, 80, -76, 50, -76}, {"C", 26, -76, 10, -66, 4, -50}},
    }},
    {advance = 72, rose = {10, 182}, strokes = {
        {{"M", 10, 130}, {"L", 10, 0}},
    }},
    {advance = 150, strokes = {
        {{"M", 0, 0}, {"L", 0, 130}, {"L", 0, 100}, {"C", 10, 122, 30, 132, 52, 132}, {"C", 85, 132, 100, 112, 100, 85}, {"L", 100, 0}},
    }},
    {advance = 120, flowers = {{1, {114, 18}, 24}}, strokes = {
        {{"M", 4, 68}, {"L", 100, 68}, {"C", 100, 106, 78, 132, 52, 132}, {"C", 22, 132, 2, 105, 2, 66},
         {"C", 2, 26, 24, 0, 54, 0}, {"C", 76, 0, 92, 10, 99, 28}},
    }},
}

local function lerp(a, b, t)
    return {a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t}
end

local function cubic(p0, c1, c2, p1, u)
    local a, b, c = lerp(p0, c1, u), lerp(c1, c2, u), lerp(c2, p1, u)
    local d, e = lerp(a, b, u), lerp(b, c, u)
    return lerp(d, e, u), a, b, c, d, e
end

local function color(c, a)
    return c[1], c[2], c[3], math.floor(math.max(0, math.min(1, a)) * 255)
end

local function buildStroke(commands, ox, oy)
    local path = Path.New()
    local segments = {}
    local pen = nil
    local total = 0
    for _, cmd in ipairs(commands) do
        if cmd[1] == "M" then
            pen = {ox + cmd[2], oy + cmd[3]}
            path:MoveTo(cmd[2], cmd[3])
        elseif cmd[1] == "L" then
            local p1 = {ox + cmd[2], oy + cmd[3]}
            local len = math.sqrt((p1[1] - pen[1]) ^ 2 + (p1[2] - pen[2]) ^ 2)
            segments[#segments + 1] = {kind = "L", p0 = pen, p1 = p1, start = total, length = len}
            total = total + len
            path:LineTo(cmd[2], cmd[3])
            pen = p1
        else
            local c1 = {ox + cmd[2], oy + cmd[3]}
            local c2 = {ox + cmd[4], oy + cmd[5]}
            local p1 = {ox + cmd[6], oy + cmd[7]}
            local lut = {{0, 0}}
            local prev, len = pen, 0
            for i = 1, 48 do
                local p = cubic(pen, c1, c2, p1, i / 48)
                len = len + math.sqrt((p[1] - prev[1]) ^ 2 + (p[2] - prev[2]) ^ 2)
                lut[#lut + 1] = {i / 48, len}
                prev = p
            end
            segments[#segments + 1] = {kind = "C", p0 = pen, c1 = c1, c2 = c2, p1 = p1, start = total, length = len, lut = lut}
            total = total + len
            path:CubicTo(cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7])
            pen = p1
        end
    end
    return path, segments, total
end

local function paramAt(segment, distance)
    local lut = segment.lut
    for i = 2, #lut do
        if lut[i][2] >= distance then
            local a, b = lut[i - 1], lut[i]
            local span = b[2] - a[2]
            return a[1] + (b[1] - a[1]) * (span > 0 and (distance - a[2]) / span or 0)
        end
    end
    return 1
end

function Logo:Spawn(name, x, y, parent)
    local e = Scene.Spawn(name, x, y, parent)
    if not parent then
        self.spawned[#self.spawned + 1] = e
    end
    return e
end

function Logo:OnCreate()
    self.time = 0
    self.spawned = {}
    self:Build()
    self:Play()
end

function Logo:Build()
    local width = 0
    for _, glyph in ipairs(GLYPHS) do
        width = width + glyph.advance
    end
    local x = -(width - 40) / 2
    self.strokes = {}
    self.glyphs = {}
    for _, glyph in ipairs(GLYPHS) do
        local g = {strokes = {}, def = glyph, x = x}
        for _, commands in ipairs(glyph.strokes) do
            local path, segments, length = buildStroke(commands, x, BASELINE)
            local shadow = Scene.Spawn("Shadow", x + 5, BASELINE - 6)
            shadow:SetZIndex(0)
            local sv = shadow:Add("Vector")
            sv:SetPath(path)
            sv:SetFillEnabled(false)
            sv:SetStrokeWidth(STROKE)
            local ink = Scene.Spawn("Ink", x, BASELINE)
            ink:SetZIndex(1)
            local iv = ink:Add("Vector")
            iv:SetPath(path)
            iv:SetFillEnabled(false)
            iv:SetStrokeWidth(STROKE)
            local stroke = {ink = ink, inkV = iv, shadow = shadow, shadowV = sv, segments = segments, length = length, s = 0, guide = 0}
            self.strokes[#self.strokes + 1] = stroke
            g.strokes[#g.strokes + 1] = stroke
        end
        self.glyphs[#self.glyphs + 1] = g
        x = x + glyph.advance
    end

    self.tagline = Scene.Spawn("Tagline", 0, BASELINE - 150)
    self.tagline:SetZIndex(2)
    self.tag = self.tagline:Add("Text")
    self.tag:SetText("2 D    G A M E    E N G I N E")
    self.tag:SetFontSize(28)
    self.tag:SetAlign(1)
end

function Logo:Reset()
    Tween.KillAll()
    for _, e in ipairs(self.spawned) do
        if e:IsValid() then
            e:Destroy()
        end
    end
    self.spawned = {}
    for _, s in ipairs(self.strokes) do
        s.inkV:SetStroke(ROSE[1], ROSE[2], ROSE[3], 255)
        s.shadowV:SetStroke(DEEP[1], DEEP[2], DEEP[3], 255)
        s.inkV:SetTrim(0, 0)
        s.shadowV:SetTrim(0, 0)
        s.s, s.guide, s.active, s.done = 0, 0, false, false
    end
    self.tag:SetColor(SAGE[1], SAGE[2], SAGE[3], 0)
    self.tagline:SetPosition(0, BASELINE - 160)
    self.current = nil
    self.hearts = false
    self.heartTimer = 0
    self.veil = 1
end

function Logo:Play()
    self:Reset()
    local steps = {
        Tween.Value(1, 0, 0.6, "easeInOutSine", function(v) self.veil = v end),
        0.25,
    }
    for gi, g in ipairs(self.glyphs) do
        for si, stroke in ipairs(g.strokes) do
            steps[#steps + 1] = function()
                stroke.active = true
                self.current = stroke
            end
            steps[#steps + 1] = Tween.Value(0, 1, math.max(0.24, stroke.length / PEN_SPEED), "easeInOutSine", function(s)
                stroke.s = s
                stroke.inkV:SetTrim(0, s)
                stroke.shadowV:SetTrim(0, s)
            end)
            steps[#steps + 1] = function()
                stroke.active = false
                stroke.done = true
                for _, f in ipairs(g.def.flowers or {}) do
                    if f[1] == si then
                        self:Bloom(g.x + f[2][1], BASELINE + f[2][2], f[3])
                    end
                end
                if g.def.rose then
                    self:Bud(g.x + g.def.rose[1], BASELINE + g.def.rose[2])
                end
            end
            steps[#steps + 1] = si < #g.strokes and 0.04 or 0.08
        end
    end
    steps[#steps + 1] = function() self.current = nil end
    steps[#steps + 1] = 0.35
    steps[#steps + 1] = function() self:BloomRose() end
    steps[#steps + 1] = 1.0
    steps[#steps + 1] = function() self.hearts = true end
    steps[#steps + 1] = Tween.Parallel({
        Tween.Value(0, 255, 0.9, "easeOutCubic", function(a) self.tag:SetColor(SAGE[1], SAGE[2], SAGE[3], math.floor(a)) end),
        Tween.To(self.tagline, {y = BASELINE - 150}, 0.9, "easeOutCubic"),
    })
    steps[#steps + 1] = 3.4
    steps[#steps + 1] = Tween.Value(0, 1, 0.7, "easeInOutSine", function(v) self.veil = v end)
    steps[#steps + 1] = function() self:Play() end
    Tween.Sequence(steps)
end

function Logo:Petals(parent, count, radius, rx, ry, fill, outline, rotation)
    for k = 1, count do
        local angle = rotation + (k - 1) * 360 / count
        local r = math.rad(angle)
        local petal = Scene.Spawn("Petal", math.cos(r) * radius, math.sin(r) * radius, parent)
        petal:SetRotation(angle - 90)
        petal:SetZIndex(0.01)
        local v = petal:Add("Vector")
        v:SetPath(Path.Ellipse(rx, ry))
        v:SetFill(fill[1], fill[2], fill[3])
        v:SetStroke(outline[1], outline[2], outline[3])
        v:SetStrokeWidth(2.5)
    end
end

function Logo:Bloom(x, y, size)
    local flower = self:Spawn("Flower", x, y)
    flower:SetZIndex(3)
    flower:SetScale(0, 0)
    self:Petals(flower, 5, size * 0.55, size * 0.36, size * 0.56, BLUSH, ROSE, 18)
    local heart = Scene.Spawn("Center", 0, 0, flower)
    heart:SetZIndex(0.02)
    local hv = heart:Add("Vector")
    hv:SetCircle(size * 0.26)
    hv:SetFill(CREAM[1], CREAM[2], CREAM[3])
    hv:SetStroke(SAGE[1], SAGE[2], SAGE[3])
    hv:SetStrokeWidth(3)
    Tween.To(flower, {scale = {1, 1}}, 0.7, "easeOutBack")
    Tween.FromTo(flower, {rotation = -70}, {rotation = 0}, 0.9, "easeOutCubic")
end

function Logo:Bud(x, y)
    local bud = self:Spawn("Bud", x, y)
    bud:SetZIndex(3)
    bud:SetScale(0, 0)
    local v = bud:Add("Vector")
    v:SetCircle(STROKE * 0.55)
    v:SetFill(DEEP[1], DEEP[2], DEEP[3])
    self.bud = bud
    Tween.To(bud, {scale = {1, 1}}, 0.45, "easeOutBack")
end

function Logo:BloomRose()
    local stem = self.glyphs[8].strokes[1]
    local g = self.glyphs[8]
    local x, y = g.x + g.def.rose[1], BASELINE + g.def.rose[2]
    Tween.To(stem.ink, {strokeColor = {SAGE[1], SAGE[2], SAGE[3], 255}}, 0.6, "easeInOutSine")
    Tween.To(stem.shadow, {strokeColor = {MOSS[1], MOSS[2], MOSS[3], 255}}, 0.6, "easeInOutSine")

    for _, leaf in ipairs({{g.x + 10, BASELINE + 70, 28}, {g.x + 10, BASELINE + 44, 152}}) do
        local e = self:Spawn("Leaf", leaf[1], leaf[2])
        e:SetZIndex(0.5)
        e:SetRotation(leaf[3])
        e:SetScale(0, 0)
        local v = e:Add("Vector")
        v:SetPath(Path.New():MoveTo(0, 0):CubicTo(12, 15, 36, 15, 50, 0):CubicTo(36, -15, 12, -15, 0, 0):Close())
        v:SetFill(SAGE[1], SAGE[2], SAGE[3])
        v:SetStroke(MOSS[1], MOSS[2], MOSS[3])
        v:SetStrokeWidth(2.5)
        Tween.To(e, {scale = {1, 1}}, 0.6, "easeOutBack", {delay = 0.15})
    end

    if self.bud and self.bud:IsValid() then
        Tween.To(self.bud, {scale = {0, 0}}, 0.25, "easeInCubic")
    end

    local rose = self:Spawn("Rose", x, y + 6)
    rose:SetZIndex(3)
    rose:SetScale(0, 0)
    self:Petals(rose, 6, 17, 13, 18, ROSE, DEEP, 0)
    local head = Scene.Spawn("Head", 0, 0, rose)
    head:SetZIndex(0.02)
    local hv = head:Add("Vector")
    hv:SetCircle(19)
    hv:SetFill(BLUSH[1], BLUSH[2], BLUSH[3])
    hv:SetStroke(DEEP[1], DEEP[2], DEEP[3])
    hv:SetStrokeWidth(2.5)

    local points = {}
    for i = 0, 90 do
        local a = i / 90 * math.pi * 4.2
        local r = 2 + i / 90 * 13
        points[#points + 1] = {math.cos(a) * r, math.sin(a) * r}
    end
    local spiral = Scene.Spawn("Spiral", 0, 0, rose)
    spiral:SetZIndex(0.03)
    local sv = spiral:Add("Vector")
    sv:SetPath(Path.Points(points, false))
    sv:SetFillEnabled(false)
    sv:SetStroke(DEEP[1], DEEP[2], DEEP[3])
    sv:SetStrokeWidth(3.5)
    sv:SetTrim(0, 0)

    Tween.To(rose, {scale = {1, 1}}, 0.8, "easeOutBack", {delay = 0.15})
    Tween.FromTo(rose, {rotation = -140}, {rotation = 0}, 1.0, "easeOutCubic", {delay = 0.15})
    Tween.To(spiral, {trimEnd = 1}, 0.9, "easeInOutSine", {delay = 0.35})
    self.rosePos = {x, y + 6}
end

function Logo:Heart()
    local x, y
    if math.random() < 0.45 and self.rosePos then
        x, y = self.rosePos[1] + (math.random() - 0.5) * 30, self.rosePos[2] + 30
    else
        local g = self.glyphs[math.random(#self.glyphs)]
        x, y = g.x + 30 + math.random() * 60, BASELINE + 150 + math.random() * 60
    end
    local e = self:Spawn("Heart", x, y)
    e:SetZIndex(4)
    local size = 14 + math.random() * 12
    local v = e:Add("Vector")
    v:SetPath(Path.Heart(size))
    local fill = math.random() < 0.5 and BLUSH or ROSE
    v:SetStroke(DEEP[1], DEEP[2], DEEP[3])
    v:SetStrokeWidth(2)
    local phase = math.random() * math.pi * 2
    local rise = 150 + math.random() * 120
    local duration = 2.6 + math.random() * 1.2
    Tween.Value(0, 1, duration, "linear", function(t)
        local eased = 1 - (1 - t) * (1 - t)
        e:SetPosition(x + math.sin(phase + t * math.pi * 2.4) * 14, y + eased * rise)
        e:SetRotation(math.sin(phase + t * math.pi * 2.4) * 12)
        local alpha = math.min(1, t / 0.15) * math.min(1, (1 - t) / 0.4)
        v:SetFill(color(fill, alpha))
        v:SetStroke(color(DEEP, alpha * 0.8))
        local s = 0.6 + 0.4 * math.min(1, t / 0.2)
        e:SetScale(s, s)
    end, {onComplete = function()
        if e:IsValid() then
            e:Destroy()
        end
    end})
end

function Logo:OnUpdate(dt)
    self.time = self.time + dt
    for _, s in ipairs(self.strokes) do
        if s.active then
            s.guide = math.min(1, s.guide + dt * 7)
        elseif s.done then
            s.guide = math.max(0, s.guide - dt * 1.8)
        end
    end
    if self.hearts then
        self.heartTimer = self.heartTimer - dt
        if self.heartTimer <= 0 then
            self.heartTimer = 0.32 + math.random() * 0.3
            self:Heart()
        end
    end
end

function Logo:DrawGuides(stroke)
    local a = stroke.guide
    Draw.SetZ(6)
    for _, seg in ipairs(stroke.segments) do
        if seg.kind == "C" then
            Draw.Line(seg.c1[1], seg.c1[2], seg.c2[1], seg.c2[2], 2, color(SAGE, a * 0.4))
            Draw.Line(seg.p0[1], seg.p0[2], seg.c1[1], seg.c1[2], 3.5, color(SAGE, a))
            Draw.Line(seg.p1[1], seg.p1[2], seg.c2[1], seg.c2[2], 3.5, color(SAGE, a))
        else
            Draw.Line(seg.p0[1], seg.p0[2], seg.p1[1], seg.p1[2], 2, color(SAGE, a * 0.55))
        end
    end
    Draw.SetZ(7)
    for _, seg in ipairs(stroke.segments) do
        if seg.kind == "C" then
            for _, c in ipairs({seg.c1, seg.c2}) do
                Draw.Circle(c[1], c[2], 9, color(SAGE, a))
                Draw.Circle(c[1], c[2], 4, color(CREAM, a))
            end
        end
        local alpha = math.floor(a * 255)
        for _, p in ipairs({seg.p0, seg.p1}) do
            Draw.Rect(p[1], p[2], 18, 18, CREAM[1], CREAM[2], CREAM[3], alpha, 45)
            Draw.Rect(p[1], p[2], 12, 12, DEEP[1], DEEP[2], DEEP[3], alpha, 45)
        end
    end
end

function Logo:DrawConstruction(stroke)
    local distance = stroke.s * stroke.length
    local seg = stroke.segments[#stroke.segments]
    for _, candidate in ipairs(stroke.segments) do
        if distance <= candidate.start + candidate.length then
            seg = candidate
            break
        end
    end
    local local_d = math.max(0, math.min(seg.length, distance - seg.start))
    local pen
    Draw.SetZ(8)
    if seg.kind == "C" then
        local u = paramAt(seg, local_d)
        local p, a, b, c, d, e = cubic(seg.p0, seg.c1, seg.c2, seg.p1, u)
        pen = p
        Draw.Line(a[1], a[2], b[1], b[2], 3, color(SAGE, 0.95))
        Draw.Line(b[1], b[2], c[1], c[2], 3, color(SAGE, 0.95))
        Draw.Line(d[1], d[2], e[1], e[2], 3, color(DEEP, 0.95))
        for _, q in ipairs({a, b, c}) do
            Draw.Circle(q[1], q[2], 6, color(SAGE, 1))
        end
        for _, q in ipairs({d, e}) do
            Draw.Circle(q[1], q[2], 6, color(DEEP, 1))
        end
    else
        pen = lerp(seg.p0, seg.p1, seg.length > 0 and local_d / seg.length or 1)
    end
    Draw.SetZ(9)
    local pulse = 0.5 + 0.5 * math.sin(self.time * 9)
    Draw.Circle(pen[1], pen[2], 24 + pulse * 4, color(BLUSH, 0.55))
    Draw.Circle(pen[1], pen[2], 11, color(DEEP, 1))
    Draw.Circle(pen[1], pen[2], 4.5, color(CREAM, 1))
end

function Logo:OnRender()
    for _, s in ipairs(self.strokes) do
        if s.guide > 0.01 then
            self:DrawGuides(s)
        end
    end
    if self.current and self.current.active then
        self:DrawConstruction(self.current)
    end
    if self.veil > 0.001 then
        Draw.SetZ(50)
        Draw.Rect(0, 0, 4000, 3000, color(CREAM, self.veil))
    end
end

return Logo
