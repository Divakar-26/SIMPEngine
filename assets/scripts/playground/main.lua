local P = {}

local PANEL = {22, 26, 38}
local LINE = {48, 55, 76}
local TEXT = {222, 228, 242}
local DIM = {128, 137, 162}
local ACCENT = {104, 136, 255}
local PINK = {255, 118, 170}
local MINT = {86, 220, 170}

local GRAPH = {x0 = -300, y0 = -190, w = 400, h = 400, vmin = -0.4, vmax = 1.4}
local LIST = {x0 = -590, x1 = -340, top = 250, row = 26, rows = 20}

local SHAPES = {
    {"Circle", "Path.Circle(130)", function() return Path.Circle(130) end},
    {"Square", "Path.Rect(240, 240)", function() return Path.Rect(240, 240) end},
    {"Rounded", "Path.Rect(240, 240, 64)", function() return Path.Rect(240, 240, 64) end},
    {"Triangle", "Path.Polygon(3, 165)", function() return Path.Polygon(3, 165) end},
    {"Hexagon", "Path.Polygon(6, 145)", function() return Path.Polygon(6, 145) end},
    {"Star", "Path.Star(5, 165, 70)", function() return Path.Star(5, 165, 70) end},
    {"Heart", "Path.Heart(280)", function() return Path.Heart(280) end},
    {"Plus", "Path.Points({...}, true)", function()
        return Path.Points({-42, -135, 42, -135, 42, -42, 135, -42, 135, 42, 42, 42, 42, 135, -42, 135, -42, 42, -135, 42, -135, -42, -42, -42}, true)
    end},
    {"Ring", "Path.Circle(135):Append(Path.Circle(62))", function() return Path.Circle(135):Append(Path.Circle(62)) end},
    {"Blob", 'Path.Svg("M 0 -120 C ...", 280)', function()
        return Path.Svg("M 0 -120 C 95 -130 145 -40 120 30 C 100 112 20 140 -55 115 C -135 85 -150 -10 -110 -70 C -80 -116 -40 -119 0 -120 Z", 280)
    end},
}

local function inside(x, y, r)
    return x >= r[1] and x <= r[3] and y >= r[2] and y <= r[4]
end

local function clamp(v, a, b)
    return math.max(a, math.min(b, v))
end

local function text(s, x, y, size, color, align, alpha)
    Draw.Text(s, x, y + size * 0.62, size, color[1], color[2], color[3], alpha or 255, align or "left")
end

local function box(r, color, radius, alpha)
    local w, h = r[3] - r[1], r[4] - r[2]
    Draw.RoundRect((r[1] + r[3]) / 2, (r[2] + r[4]) / 2, w, h, radius or 8, color[1], color[2], color[3], alpha or 255)
end

function P:OnCreate()
    self.names = {"custom"}
    for _, n in ipairs(Curves.Names()) do
        self.names[#self.names + 1] = n
    end
    self.selected = 1
    for i, n in ipairs(self.names) do
        if n == "easeOutBack" then
            self.selected = i
        end
    end
    self.scroll = math.max(0, self.selected - 6)
    self.cubic = {0.25, 0.1, 0.25, 1.0}
    self.duration = 0.9
    self.clock = 0
    self.mode = "curves"
    self.drag = nil
    self.auto = false
    self.autoClock = 0
    self.fill = true
    self.stroke = true
    self.showPoints = false
    self.fromShape = 1
    self.toShape = 1
    self.scrub = 0

    self.tabs = {
        {name = "Curves", mode = "curves", rect = {-590, 286, -470, 320}},
        {name = "Morph", mode = "morph", rect = {-460, 286, -340, 320}},
    }
    self.durationSlider = {x0 = -300, x1 = 100, y = -300, min = 0.2, max = 3.0}
    self.replay = {160, -316, 300, -284}
    self.shapeButtons = {}
    for i = 1, #SHAPES do
        local col = (i - 1) % 5
        local row = math.floor((i - 1) / 5)
        local x0 = 180 + col * 84
        local y1 = 250 - row * 46
        self.shapeButtons[i] = {x0, y1 - 38, x0 + 76, y1}
    end
    self.prevCurve = {180, 98, 220, 134}
    self.nextCurve = {550, 98, 590, 134}
    self.morphDuration = {x0 = 190, x1 = 580, y = 40, min = 0.2, max = 3.0}
    self.toggles = {
        {name = "Auto", key = "auto", rect = {180, -40, 278, -6}},
        {name = "Fill", key = "fill", rect = {286, -40, 384, -6}},
        {name = "Stroke", key = "stroke", rect = {392, -40, 490, -6}},
        {name = "Points", key = "showPoints", rect = {498, -40, 590, -6}},
    }
    self.scrubSlider = {x0 = 190, x1 = 580, y = -130, min = 0, max = 1}

    self:BuildCurvesTab()
    self:BuildMorphTab()
    self:SetMode("curves")
end

function P:Curve()
    if self.selected == 1 then
        local c = self.cubic
        return Curves.Cubic(c[1], c[2], c[3], c[4])
    end
    return self.names[self.selected]
end

function P:CurveCode()
    if self.selected == 1 then
        local c = self.cubic
        return string.format("Curves.Cubic(%.2f, %.2f, %.2f, %.2f)", c[1], c[2], c[3], c[4])
    end
    return '"' .. self.names[self.selected] .. '"'
end

function P:BuildCurvesTab()
    self.curveRoot = Scene.Spawn("CurvesTab", 0, 0)
    self.square = Scene.Spawn("ScaleRotate", 250, 30, self.curveRoot)
    self.square:SetZIndex(2)
    local sv = self.square:Add("Vector")
    sv:SetRect(70, 70, 14)
    sv:SetFill(ACCENT[1], ACCENT[2], ACCENT[3])

    self.fader = Scene.Spawn("Opacity", 375, 30, self.curveRoot)
    self.fader:SetZIndex(2)
    local fv = self.fader:Add("Vector")
    fv:SetCircle(38)
    fv:SetFill(PINK[1], PINK[2], PINK[3])

    self.mini = Scene.Spawn("MiniMorph", 500, 30, self.curveRoot)
    self.mini:SetZIndex(2)
    local mv = self.mini:Add("Vector")
    mv:SetCircle(38)
    mv:SetMorphTarget(Path.Star(5, 48, 20))
    mv:SetFill(MINT[1], MINT[2], MINT[3])
end

function P:RestartPreviews()
    self.clock = 0
    for _, e in ipairs({self.square, self.fader, self.mini}) do
        Tween.Kill(e)
    end
    self.square:SetScale(1, 1)
    self.square:SetRotation(0)
    self.fader:GetVector():SetFill(PINK[1], PINK[2], PINK[3], 255)
    self.mini:GetVector():SetMorph(0)
    local curve = self:Curve()
    local opts = {loop = true, yoyo = true}
    Tween.To(self.square, {scale = {1.5, 1.5}, rotation = 180}, self.duration, curve, opts)
    Tween.To(self.fader, {alpha = 30}, self.duration, curve, opts)
    Tween.To(self.mini, {morph = 1}, self.duration, curve, opts)
end

function P:BuildMorphTab()
    self.morphRoot = Scene.Spawn("MorphTab", 0, 0)
    self.shape = Scene.Spawn("MorphShape", -150, 10, self.morphRoot)
    self.shape:SetZIndex(2)
    local v = self.shape:Add("Vector")
    v:SetPath(SHAPES[1][3]())
    v:SetFill(ACCENT[1], ACCENT[2], ACCENT[3])
    v:SetStroke(TEXT[1], TEXT[2], TEXT[3])
    v:SetStrokeWidth(6)
    v:SetJoin("round")
end

function P:SetMode(mode)
    self.mode = mode
    self.curveRoot:SetPosition(mode == "curves" and 0 or 10000, 0)
    self.morphRoot:SetPosition(mode == "morph" and 0 or 10000, 0)
    if mode == "curves" then
        self:RestartPreviews()
    end
end

function P:Select(index)
    self.selected = clamp(index, 1, #self.names)
    if self.selected <= self.scroll then
        self.scroll = self.selected - 1
    elseif self.selected > self.scroll + LIST.rows then
        self.scroll = self.selected - LIST.rows
    end
    if self.mode == "curves" then
        self:RestartPreviews()
    end
end

function P:MorphTo(index)
    local v = self.shape:GetVector()
    self.fromShape = self.toShape
    self.toShape = index
    v:MorphTo(SHAPES[index][3](), self.duration, self:Curve())
    Tween.FromTo(self.shape, {scale = {0.94, 0.94}}, {scale = {1, 1}}, 0.35, "easeOutBack")
end

function P:Scrub(t)
    local v = self.shape:GetVector()
    Tween.Kill(self.shape, "morph")
    v:SetPath(SHAPES[self.fromShape][3]())
    v:SetMorphTarget(SHAPES[self.toShape][3]())
    v:SetMorph(t)
end

function P:SliderValue(s, mx)
    return s.min + clamp((mx - s.x0) / (s.x1 - s.x0), 0, 1) * (s.max - s.min)
end

function P:SliderRect(s)
    return {s.x0 - 10, s.y - 14, s.x1 + 10, s.y + 14}
end

function P:GraphPoint(t, v)
    local g = GRAPH
    return g.x0 + t * g.w, g.y0 + (v - g.vmin) / (g.vmax - g.vmin) * g.h
end

function P:GraphValue(x, y)
    local g = GRAPH
    return clamp((x - g.x0) / g.w, 0, 1), clamp(g.vmin + (y - g.y0) / g.h * (g.vmax - g.vmin), -0.6, 1.6)
end

function P:OnUpdate(dt)
    self.clock = self.clock + dt
    local mx, my = self.entity:GetMouseWorld()
    local inView = Input.IsMouseInViewport()
    local pressed = inView and Input.IsMouseJustPressed(Key.MOUSE_LEFT)
    local down = Input.IsMouseDown(Key.MOUSE_LEFT)

    if Input.IsKeyJustPressed(Key.TAB) then
        self:SetMode(self.mode == "curves" and "morph" or "curves")
    end
    if Input.IsKeyJustPressed(Key.UP) then
        self:Select(self.selected - 1)
    elseif Input.IsKeyJustPressed(Key.DOWN) then
        self:Select(self.selected + 1)
    end
    if Input.IsKeyJustPressed(Key.SPACE) then
        if self.mode == "curves" then
            self:RestartPreviews()
        else
            self:MorphTo(self.toShape % #SHAPES + 1)
        end
    end

    if pressed then
        for _, tab in ipairs(self.tabs) do
            if inside(mx, my, tab.rect) and self.mode ~= tab.mode then
                self:SetMode(tab.mode)
                return
            end
        end
    end

    if not down then
        if (self.drag == "duration" or self.drag == "p1" or self.drag == "p2") and self.mode == "curves" then
            self:RestartPreviews()
        end
        self.drag = nil
    end

    if self.mode == "curves" then
        self:UpdateCurves(mx, my, pressed, down, inView)
    else
        self:UpdateMorph(mx, my, pressed, down, dt)
    end
end

function P:UpdateCurves(mx, my, pressed, down, inView)
    local listRect = {LIST.x0, LIST.top - LIST.rows * LIST.row, LIST.x1, LIST.top}
    if inView and inside(mx, my, listRect) then
        local wheel = Input.GetMouseWheel()
        if wheel ~= 0 then
            self.scroll = clamp(self.scroll - math.floor(wheel * 2), 0, #self.names - LIST.rows)
        end
        if pressed then
            local index = self.scroll + math.floor((LIST.top - my) / LIST.row) + 1
            if index >= 1 and index <= #self.names then
                self:Select(index)
            end
        end
    end

    if pressed and self.selected == 1 then
        local c = self.cubic
        local ax, ay = self:GraphPoint(c[1], c[2])
        local bx, by = self:GraphPoint(c[3], c[4])
        if Math.Distance(mx, my, ax, ay) < 16 then
            self.drag = "p1"
        elseif Math.Distance(mx, my, bx, by) < 16 then
            self.drag = "p2"
        end
    end
    if pressed and inside(mx, my, self:SliderRect(self.durationSlider)) then
        self.drag = "duration"
    end
    if pressed and inside(mx, my, self.replay) then
        self:RestartPreviews()
    end

    if down and self.drag == "p1" then
        self.cubic[1], self.cubic[2] = self:GraphValue(mx, my)
    elseif down and self.drag == "p2" then
        self.cubic[3], self.cubic[4] = self:GraphValue(mx, my)
    elseif down and self.drag == "duration" then
        self.duration = self:SliderValue(self.durationSlider, mx)
    end
end

function P:UpdateMorph(mx, my, pressed, down, dt)
    if pressed then
        for i, r in ipairs(self.shapeButtons) do
            if inside(mx, my, r) then
                self.auto = false
                self:MorphTo(i)
            end
        end
        if inside(mx, my, self.prevCurve) then
            self:Select(self.selected - 1)
        elseif inside(mx, my, self.nextCurve) then
            self:Select(self.selected + 1)
        end
        for _, t in ipairs(self.toggles) do
            if inside(mx, my, t.rect) then
                self[t.key] = not self[t.key]
                local v = self.shape:GetVector()
                v:SetFillEnabled(self.fill)
                v:SetStrokeEnabled(self.stroke)
                self.autoClock = 0
            end
        end
        if inside(mx, my, self:SliderRect(self.morphDuration)) then
            self.drag = "morphDuration"
        elseif inside(mx, my, self:SliderRect(self.scrubSlider)) then
            self.drag = "scrub"
            self.auto = false
        end
    end

    if down and self.drag == "morphDuration" then
        self.duration = self:SliderValue(self.morphDuration, mx)
    elseif down and self.drag == "scrub" then
        self.scrub = self:SliderValue(self.scrubSlider, mx)
        self:Scrub(self.scrub)
    end

    if self.auto then
        self.autoClock = self.autoClock + dt
        if self.autoClock > self.duration + 0.7 then
            self.autoClock = 0
            self:MorphTo(self.toShape % #SHAPES + 1)
        end
    end
end

function P:DrawButton(r, label, active, hover)
    local color = active and ACCENT or (hover and LINE or PANEL)
    box(r, color, 8)
    text(label, (r[1] + r[3]) / 2, (r[2] + r[4]) / 2, 17, active and {255, 255, 255} or TEXT, "center")
end

function P:DrawSlider(s, value, label, format)
    local t = (value - s.min) / (s.max - s.min)
    Draw.Line(s.x0, s.y, s.x1, s.y, 6, LINE[1], LINE[2], LINE[3])
    Draw.Line(s.x0, s.y, s.x0 + (s.x1 - s.x0) * t, s.y, 6, ACCENT[1], ACCENT[2], ACCENT[3])
    Draw.Circle(s.x0 + (s.x1 - s.x0) * t, s.y, 11, 255, 255, 255)
    text(label, s.x0, s.y + 30, 16, DIM)
    text(string.format(format, value), s.x1, s.y + 30, 16, TEXT, "right")
end

function P:OnRender()
    local mx, my = self.entity:GetMouseWorld()
    Draw.SetZ(1)
    for _, tab in ipairs(self.tabs) do
        self:DrawButton(tab.rect, tab.name, self.mode == tab.mode, inside(mx, my, tab.rect))
    end
    text("Animation Playground", 590, 303, 24, TEXT, "right")
    text("Tab switches pages, Up/Down picks a curve, Space replays", 590, 270, 14, DIM, "right")

    if self.mode == "curves" then
        self:RenderCurves(mx, my)
    else
        self:RenderMorph(mx, my)
    end
end

function P:RenderCurves(mx, my)
    box({LIST.x0 - 8, LIST.top - LIST.rows * LIST.row - 8, LIST.x1 + 8, LIST.top + 8}, PANEL, 12)
    for row = 1, LIST.rows do
        local index = self.scroll + row
        local name = self.names[index]
        if name then
            local yTop = LIST.top - (row - 1) * LIST.row
            local r = {LIST.x0, yTop - LIST.row + 2, LIST.x1, yTop - 1}
            if index == self.selected then
                box(r, ACCENT, 6)
            elseif inside(mx, my, r) then
                box(r, LINE, 6)
            end
            text(name, LIST.x0 + 10, (r[2] + r[4]) / 2, 16, index == self.selected and {255, 255, 255} or TEXT)
        end
    end
    local total = #self.names
    local barH = LIST.rows * LIST.row * LIST.rows / total
    local barY = LIST.top - (LIST.rows * LIST.row - barH) * self.scroll / math.max(1, total - LIST.rows) - barH / 2
    Draw.Line(LIST.x1 + 3, barY + barH / 2 - 3, LIST.x1 + 3, barY - barH / 2 + 3, 4, DIM[1], DIM[2], DIM[3], 160)

    local g = GRAPH
    box({g.x0 - 12, g.y0 - 12, g.x0 + g.w + 12, g.y0 + g.h + 12}, PANEL, 14)
    local _, y0 = self:GraphPoint(0, 0)
    local _, y1 = self:GraphPoint(0, 1)
    Draw.Line(g.x0, y0, g.x0 + g.w, y0, 2, LINE[1], LINE[2], LINE[3])
    Draw.Line(g.x0, y1, g.x0 + g.w, y1, 2, LINE[1], LINE[2], LINE[3])
    for i = 1, 3 do
        local x = g.x0 + g.w * i / 4
        Draw.Line(x, g.y0, x, g.y0 + g.h, 1, LINE[1], LINE[2], LINE[3], 120)
    end
    text("0", g.x0 - 16, y0, 13, DIM, "right")
    text("1", g.x0 - 16, y1, 13, DIM, "right")
    local ax, ay = self:GraphPoint(0, 0)
    local bx, by = self:GraphPoint(1, 1)
    Draw.Line(ax, ay, bx, by, 2, DIM[1], DIM[2], DIM[3], 90)

    local curve = self:Curve()
    local pts = {}
    for i = 0, 120 do
        local t = i / 120
        local x, y = self:GraphPoint(t, Curves.Evaluate(curve, t))
        pts[#pts + 1] = x
        pts[#pts + 1] = y
    end
    Draw.Polyline(pts, 4, ACCENT[1], ACCENT[2], ACCENT[3])

    if self.selected == 1 then
        local c = self.cubic
        local p1x, p1y = self:GraphPoint(c[1], c[2])
        local p2x, p2y = self:GraphPoint(c[3], c[4])
        Draw.Line(ax, ay, p1x, p1y, 2, PINK[1], PINK[2], PINK[3])
        Draw.Line(bx, by, p2x, p2y, 2, PINK[1], PINK[2], PINK[3])
        Draw.Circle(p1x, p1y, 10, PINK[1], PINK[2], PINK[3])
        Draw.Circle(p2x, p2y, 10, PINK[1], PINK[2], PINK[3])
        text("drag the pink handles", g.x0 + g.w / 2, g.y0 + g.h + 30, 14, DIM, "center")
    end

    local cycle = self.duration + 0.6
    local phase = self.clock % cycle
    local t = clamp(phase / self.duration, 0, 1)
    local v = Curves.Evaluate(curve, t)
    local mxp, myp = self:GraphPoint(t, v)
    Draw.Line(mxp, g.y0, mxp, g.y0 + g.h, 1, TEXT[1], TEXT[2], TEXT[3], 60)
    Draw.Circle(mxp, myp, 9, 255, 255, 255)

    text("Tween.To(e, {x = 300}, " .. string.format("%.2f", self.duration) .. ", " .. self:CurveCode() .. ")", g.x0 + g.w / 2, g.y0 - 45, 15, MINT, "center")
    self:DrawSlider(self.durationSlider, self.duration, "Duration", "%.2f s")

    box({150, -240, 600, 250}, PANEL, 14)
    text("Position", 175, 225, 15, DIM)
    Draw.Line(190, 175, 560, 175, 4, LINE[1], LINE[2], LINE[3])
    Draw.Circle(190 + 370 * v, 175, 16, ACCENT[1], ACCENT[2], ACCENT[3])
    text("Scale + rotate", 250, 100, 14, DIM, "center")
    text("Opacity", 375, 100, 14, DIM, "center")
    text("Morph", 500, 100, 14, DIM, "center")
    text("real tweens, loop + yoyo", 375, -40, 13, DIM, "center")

    text("Color", 175, -90, 15, DIM)
    local r = ACCENT[1] + (PINK[1] - ACCENT[1]) * clamp(v, 0, 1)
    local gg = ACCENT[2] + (PINK[2] - ACCENT[2]) * clamp(v, 0, 1)
    local b = ACCENT[3] + (PINK[3] - ACCENT[3]) * clamp(v, 0, 1)
    Draw.RoundRect(375, -150, 370, 50, 12, r, gg, b, 255)

    local hover = inside(mx, my, self.replay)
    self:DrawButton(self.replay, "Replay", false, hover)
end

function P:RenderMorph(mx, my)
    box({-590, -300, 150, 260}, PANEL, 18)
    box({170, -300, 600, 260}, PANEL, 18)

    for i, r in ipairs(self.shapeButtons) do
        self:DrawButton(r, SHAPES[i][1], i == self.toShape, inside(mx, my, r))
    end

    self:DrawButton(self.prevCurve, "<", false, inside(mx, my, self.prevCurve))
    self:DrawButton(self.nextCurve, ">", false, inside(mx, my, self.nextCurve))
    text(self.names[self.selected], 385, 116, 20, TEXT, "center")
    text("Curve", 190, 152, 15, DIM)

    self:DrawSlider(self.morphDuration, self.duration, "Duration", "%.2f s")
    for _, t in ipairs(self.toggles) do
        self:DrawButton(t.rect, t.name, self[t.key], inside(mx, my, t.rect))
    end

    local morph = self.shape:GetVector():GetMorph()
    self:DrawSlider(self.scrubSlider, self.drag == "scrub" and self.scrub or clamp(morph, 0, 1), "Scrub (drag)", "%.2f")
    text(SHAPES[self.fromShape][1] .. "  to  " .. SHAPES[self.toShape][1], 385, -200, 18, TEXT, "center")
    text("morph = " .. string.format("%.3f", morph), 385, -232, 15, MINT, "center")
    text("vector:MorphTo(" .. SHAPES[self.toShape][2] .. ", " .. string.format("%.2f", self.duration) .. ", " .. self:CurveCode() .. ")", -220, -270, 14, MINT, "center")

    if self.showPoints then
        local v = self.shape:GetVector()
        for i = 0, 63 do
            local x, y = v:GetPointAt(i / 64)
            if x then
                Draw.SetZ(5)
                Draw.Circle(x, y, 3.5, PINK[1], PINK[2], PINK[3])
            end
        end
        Draw.SetZ(1)
    end
end

return P
