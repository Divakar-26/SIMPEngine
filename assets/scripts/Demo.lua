local Integration = {}

local LOOP = true

local BG = {14, 17, 30}
local PANEL = {25, 30, 52}
local INK = {236, 241, 252}
local MUTED = {138, 148, 176}
local CYAN = {88, 206, 255}
local TEAL = {62, 214, 176}
local CORAL = {255, 126, 96}
local GOLD = {255, 206, 96}
local VIOLET = {172, 142, 255}

local A, B = 0, 5
local OX, OY = -675, -330
local SX, SY = 270, 180
local BX, BY = 0, 1440
local TX, TY = 0, 1560

local STAGES = {4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048}
local WIN_B = 67 * (B - A) / 256
local SPLIT_DUR = {1.7, 1.4, 1.15, 1.0, 0.9, 0.8, 1.2, 1.1, 1.0}
local SPLIT_HOLD = {0.9, 0.6, 0.5, 0.4, 0.4, 1.0, 0.6, 0.5, 1.0}
local GLYPH_N = 110

local function f(x)
    return 1.8 + 1.2 * math.sin(1.2 * x) + 0.25 * x
end

local function antiderivative(x)
    return 1.8 * x - math.cos(1.2 * x) + 0.125 * x * x
end

local TRUE_AREA = antiderivative(B) - antiderivative(A)

local function px(x)
    return OX + SX * x
end

local function py(y)
    return OY + SY * y
end

local function clamp01(v)
    if v < 0 then
        return 0
    end
    if v > 1 then
        return 1
    end
    return v
end

local function lerp(a, b, t)
    return a + (b - a) * t
end

local function prog(t, a, b)
    return clamp01((t - a) / (b - a))
end

local function mix(a, b, t)
    return {lerp(a[1], b[1], t), lerp(a[2], b[2], t), lerp(a[3], b[3], t)}
end

local ease = {}

function ease.linear(u)
    return u
end

function ease.inOutCubic(u)
    if u < 0.5 then
        return 4 * u * u * u
    end
    local v = -2 * u + 2
    return 1 - v * v * v / 2
end

function ease.outCubic(u)
    local v = 1 - u
    return 1 - v * v * v
end

function ease.inOutSine(u)
    return -(math.cos(math.pi * u) - 1) / 2
end

function ease.outBack(u)
    local c1 = 1.70158
    local c3 = c1 + 1
    local v = u - 1
    return 1 + c3 * v * v * v + c1 * v * v
end

local T = {}
local TOTAL = 0

do
    local cur = 0
    local function add(name, dur)
        T[name] = {a = cur, b = cur + dur}
        cur = cur + dur
    end
    add("intro", 4.6)
    add("pan", 2.4)
    add("axes", 2.2)
    add("curve", 5.4)
    add("pull", 1.5)
    add("area", 4.6)
    add("big", 3.4)
    add("bigIn", 1.5)
    add("bigNote", 4.4)
    add("bigOut", 1.3)
    add("bigSum", 2.6)
    for i = 1, 6 do
        add("split" .. i, SPLIT_DUR[i])
        add("hold" .. i, SPLIT_HOLD[i])
    end
    add("diveIn", 1.8)
    for i = 7, 9 do
        add("split" .. i, SPLIT_DUR[i])
        add("hold" .. i, SPLIT_HOLD[i])
    end
    add("diveOut", 1.8)
    add("smooth", 3.4)
    add("panUp", 2.4)
    add("fSum", 3.8)
    add("fExplain", 3.2)
    add("fThin", 3.4)
    add("fMorph", 3.4)
    add("fResult", 4.2)
    add("outro", 3.4)
    add("fade", 1.0)
    TOTAL = cur
end

local function seg(t, name)
    local s = T[name]
    return prog(t, s.a, s.b)
end

local SUMS = {}
for k, n in ipairs(STAGES) do
    local dx = (B - A) / n
    local s = 0
    for i = 0, n - 1 do
        s = s + f(A + i * dx)
    end
    SUMS[k] = s * dx
end

local function a255(a)
    if a == nil then
        a = 1
    end
    return math.floor(255 * clamp01(a) + 0.5)
end

local function rgba(c, a)
    return c[1], c[2], c[3], a255(a)
end

local function line(x1, y1, x2, y2, w, c, a)
    Draw.Line(x1, y1, x2, y2, w, rgba(c, a))
end

local function circle(x, y, r, c, a)
    Draw.Circle(x, y, r, rgba(c, a))
end

local function ring(x, y, r, th, c, a)
    Draw.Ring(x, y, r, th, rgba(c, a))
end

local function rect(x, y, w, h, c, a)
    Draw.Rect(x, y, w, h, rgba(c, a))
end

local function rrect(x, y, w, h, r, c, a)
    Draw.RoundRect(x, y, w, h, r, rgba(c, a))
end

local function pline(pts, w, c, a, closed)
    Draw.Polyline(pts, w, c[1], c[2], c[3], a255(a), closed and true or false)
end

local function poly(pts, c, a)
    Draw.Polygon(pts, rgba(c, a))
end

local function text(str, x, cy, size, c, a, align)
    if a ~= nil and a <= 0.004 then
        return
    end
    Draw.Text(str, x, cy + size * 0.52, size, c[1], c[2], c[3], a255(a), align or "center")
end

local function tw(str, size)
    return #str * size * 0.56
end

local function dashed(x1, y1, x2, y2, w, dash, gap, c, a, p)
    local len = math.sqrt((x2 - x1) ^ 2 + (y2 - y1) ^ 2)
    if len < 0.5 then
        return
    end
    local reach = len * (p or 1)
    local period = dash + gap
    if reach / period > 90 then
        local k = reach / period / 90
        dash, gap, period = dash * k, gap * k, period * k
    end
    local ux, uy = (x2 - x1) / len, (y2 - y1) / len
    local d = 0
    while d < reach do
        local e = math.min(d + dash, reach)
        line(x1 + ux * d, y1 + uy * d, x1 + ux * e, y1 + uy * e, w, c, a)
        d = d + period
    end
end

local function arrow(x1, y1, x2, y2, w, c, a)
    line(x1, y1, x2, y2, w, c, a)
    local ang = math.atan(y2 - y1, x2 - x1)
    local hs = w * 4.2
    for _, s in ipairs({1, -1}) do
        local b = ang + math.pi + s * 0.5
        line(x2, y2, x2 + math.cos(b) * hs, y2 + math.sin(b) * hs, w, c, a)
    end
end

local function deltaGlyph(cx, cy, d, c, a)
    local pts = {cx - 0.4 * d, cy - 0.36 * d, cx + 0.4 * d, cy - 0.36 * d, cx, cy + 0.4 * d}
    pline(pts, d * 0.12, c, a, true)
end

local function deltaX(cx, cy, s, c, a)
    local left = cx - 0.72 * s
    deltaGlyph(left + 0.38 * s, cy, s * 0.82, c, a)
    text("x", left + 0.82 * s, cy, s, c, a, "left")
end

local function infinity(cx, cy, s, c, a, w)
    local pts = {}
    for i = 0, 48 do
        local t = i / 48 * math.pi * 2
        local d = 1 + math.sin(t) ^ 2
        pts[#pts + 1] = cx + s * math.cos(t) / d
        pts[#pts + 1] = cy + s * math.sin(t) * math.cos(t) / d
    end
    pline(pts, w, c, a, true)
end

local function approx(cx, cy, s, flat, c, a)
    for _, off in ipairs({0.17, -0.17}) do
        local pts = {}
        for i = 0, 16 do
            local u = i / 16
            pts[#pts + 1] = cx + (u - 0.5) * 0.9 * s
            pts[#pts + 1] = cy + off * s + (1 - flat) * 0.1 * s * math.sin(u * math.pi * 2)
        end
        pline(pts, s * 0.075, c, a, false)
    end
end

local function resample(pts, n)
    local lens, total = {0}, 0
    for i = 2, #pts do
        local dx, dy = pts[i][1] - pts[i - 1][1], pts[i][2] - pts[i - 1][2]
        total = total + math.sqrt(dx * dx + dy * dy)
        lens[i] = total
    end
    local out, j = {}, 2
    for k = 0, n - 1 do
        local target = total * k / (n - 1)
        while j < #pts and lens[j] < target do
            j = j + 1
        end
        local span = lens[j] - lens[j - 1]
        local u = span > 0 and clamp01((target - lens[j - 1]) / span) or 0
        out[#out + 1] = {lerp(pts[j - 1][1], pts[j][1], u), lerp(pts[j - 1][2], pts[j][2], u)}
    end
    return out
end

local function bezier(p0, p1, p2, p3, n, out)
    for i = 0, n do
        local t = i / n
        local m = 1 - t
        local a, b, c, d = m * m * m, 3 * m * m * t, 3 * m * t * t, t * t * t
        out[#out + 1] = {a * p0[1] + b * p1[1] + c * p2[1] + d * p3[1], a * p0[2] + b * p1[2] + c * p2[2] + d * p3[2]}
    end
end

local SIGMA_R, INTEG_R
do
    local sigma = {{0.95, 0.88}, {0.95, 1.0}, {0.05, 1.0}, {0.52, 0.5}, {0.05, 0.0}, {0.95, 0.0}, {0.95, 0.12}}
    local integ = {}
    bezier({0.06, 0.10}, {0.10, -0.04}, {0.38, -0.04}, {0.42, 0.22}, 18, integ)
    bezier({0.42, 0.22}, {0.47, 0.40}, {0.53, 0.60}, {0.58, 0.78}, 6, integ)
    bezier({0.58, 0.78}, {0.62, 1.04}, {0.90, 1.04}, {0.94, 0.90}, 18, integ)
    SIGMA_R = resample(sigma, GLYPH_N)
    INTEG_R = resample(integ, GLYPH_N)
end

local function glyphPoints(cx, cy, H, m, trim)
    local out = {}
    local wide = lerp(0.74, 0.5, m)
    local last = math.max(2, math.floor(GLYPH_N * trim + 0.5))
    for i = 1, last do
        local sp, ip = SIGMA_R[i], INTEG_R[i]
        out[#out + 1] = cx + (lerp(sp[1], ip[1], m) - 0.5) * H * wide
        out[#out + 1] = cy + (lerp(sp[2], ip[2], m) - 0.5) * H
    end
    return out
end

local function curveFlat(xr, np)
    local out = {}
    local span = B - A
    local steps = np - 1
    local k = math.floor((xr - A) / span * steps + 1e-9)
    local n = 0
    for i = 0, k do
        local x = A + span * i / steps
        out[n + 1], out[n + 2] = px(x), py(f(x))
        n = n + 2
    end
    if xr - (A + span * k / steps) > 1e-6 then
        out[n + 1], out[n + 2] = px(xr), py(f(xr))
    end
    return out
end

local CURVE_FULL = curveFlat(B, 361)
local AREA_FULL = curveFlat(B, 121)

local function areaPoly(xr)
    local pts = {px(A), OY}
    local c = xr >= B - 1e-6 and AREA_FULL or curveFlat(xr, 121)
    for i = 1, #c do
        pts[#pts + 1] = c[i]
    end
    pts[#pts + 1] = px(xr)
    pts[#pts + 1] = OY
    return pts
end

local function pose(x, y, z)
    return {x = x, y = y, z = z}
end

local function penPose(u)
    local xr = A + (B - A) * ease.inOutSine(u)
    return pose(0.78 * px(xr), -190 + 0.25 * (py(f(xr)) + 190), lerp(0.86, 0.92, u))
end

local WIDE = pose(0, -75, 1.02)
local WIDE2 = pose(-20, -65, 1.08)
local SLICE = pose(px(0.62) + 10, -285, 0.6)
local SLICE2 = pose(px(0.62) - 10, -280, 0.57)
local DIVE = pose(px(0.55), py(f(0.55)), 0.18)
local BOARD = pose(BX, BY, 1.0)
local BOARD_ZOOM = pose(BX, BY + 110, 0.8)

local PATH = {
    {"intro", pose(TX, TY - 40, 1.38), pose(TX, TY - 40, 1.14), ease.outCubic},
    {"pan", pose(TX, TY - 40, 1.14), penPose(0), ease.inOutCubic, 0.3},
    {"curve", fn = penPose},
    {"pull", penPose(1), WIDE, ease.inOutCubic},
    {"area", WIDE, WIDE2, ease.inOutSine},
    {"big", WIDE2, WIDE, ease.inOutSine},
    {"bigIn", WIDE, SLICE, ease.inOutCubic},
    {"bigNote", SLICE, SLICE2, ease.inOutSine},
    {"bigOut", SLICE2, WIDE, ease.inOutCubic},
    {"diveIn", WIDE, DIVE, ease.inOutCubic},
    {"diveOut", DIVE, WIDE, ease.inOutCubic},
    {"smooth", WIDE, WIDE2, ease.inOutSine},
    {"panUp", WIDE2, BOARD, ease.inOutCubic, 0.35},
    {"fMorph", BOARD, BOARD_ZOOM, ease.inOutCubic},
    {"fResult", BOARD_ZOOM, BOARD, ease.inOutCubic}
}

local CAPS = {
    {"axes", "pull", "STEP 1  -  A NON-LINEAR CURVE", "Its height f(x) changes at every point along x"},
    {"area", "area", "THE QUESTION", "How much area lies under the curve, between a and b?"},
    {"big", "big", "STEP 2  -  BIG SLICES", "Cut the area into a few wide slices"},
    {"bigIn", "bigIn", "ONE SLICE", "Each slice is a rectangle"},
    {"bigNote", "bigNote", "ONE SLICE", "Area of a slice = its height x its width"},
    {"bigOut", "bigSum", "STEP 2  -  ADD THEM UP", "A rough estimate: the rectangles miss the curve"},
    {"split1", "hold3", "STEP 3  -  THINNER SLICES", "Cut every slice in half, then again..."},
    {"split4", "hold6", "STEP 3  -  THINNER SLICES", "More slices, thinner slices, smaller error"},
    {"diveIn", "diveOut", "ZOOM IN", "Even at 256 slices the tops are tiny steps - keep cutting"},
    {"smooth", "smooth", "THE LIMIT", "Infinitely thin slices: the staircase becomes the curve"},
    {"panUp", "fSum", "STEP 4  -  THE FORMULA", "Write down the sum of all the slices"},
    {"fExplain", "fExplain", "EACH TERM", "The height of one slice times its width"},
    {"fThin", "fThin", "THE LIMIT", "Width shrinks toward zero, slice count toward infinity"},
    {"fMorph", "fMorph", "THE INTEGRAL", "The sum turns into the integral"},
    {"fResult", "fResult", "RESULT", "For our curve, from 0 to 5, the exact area is"}
}

function Integration:CameraPose(t)
    local last = pose(TX, TY - 40, 1.38)
    for _, e in ipairs(PATH) do
        local s = T[e[1]]
        if t < s.a then
            return last.x, last.y, last.z
        end
        local u = prog(t, s.a, s.b)
        local p
        if e.fn then
            p = e.fn(u)
        else
            local k = e[4](u)
            p = pose(lerp(e[2].x, e[3].x, k), lerp(e[2].y, e[3].y, k), lerp(e[2].z, e[3].z, k))
            if e[5] then
                p.z = p.z + e[5] * math.sin(math.pi * k)
            end
        end
        if t <= s.b then
            return p.x, p.y, p.z
        end
        last = e.fn and e.fn(1) or e[3]
    end
    return last.x, last.y, last.z
end

function Integration:StageAt(t)
    for i = 1, #STAGES - 1 do
        local s, h = T["split" .. i], T["hold" .. i]
        if t < s.a then
            return i, i, 1
        end
        if t < s.b then
            return i, i + 1, ease.inOutCubic(prog(t, s.a, s.b))
        end
        if t < h.b then
            return i + 1, i + 1, 1
        end
    end
    return #STAGES, #STAGES, 1
end

function Integration:ApplyCamera()
    local x, y, z = self:CameraPose(self.t)
    x = x + 5 * z * math.sin(self.t * 0.55)
    y = y + 4 * z * math.sin(self.t * 0.71 + 1.3)
    z = z * (1 + 0.004 * math.sin(self.t * 0.43))
    self.cx, self.cy, self.z = x, y, z
    if self.camera and self.camera:IsValid() then
        self.camera:SetPosition(x, y)
        self.camera:GetCamera():SetZoom(z)
    end
end

function Integration:OnCreate()
    self.t = 0
    self.paused = false
    self.camera = Scene.Find("Camera")
    if self.camera then
        self.camera:GetCamera():SetFollow("")
    else
        log.warn("Integration: add an entity named Camera with a primary Camera component")
    end
    self:ApplyCamera()
end

function Integration:OnUpdate(dt)
    if Input.IsKeyJustPressed(Key.R) then
        self.t = 0
    end
    if Input.IsKeyJustPressed(Key.SPACE) then
        self.paused = not self.paused
    end
    if Input.IsKeyJustPressed(Key.RIGHT) then
        self.t = math.min(TOTAL - 0.01, self.t + 3)
    end
    if Input.IsKeyJustPressed(Key.LEFT) then
        self.t = math.max(0, self.t - 3)
    end
    if not self.paused then
        self.t = self.t + math.min(dt, 0.1)
        if self.t >= TOTAL then
            self.t = LOOP and 0 or TOTAL - 0.001
        end
    end
    self:ApplyCamera()
end

function Integration:DrawBackdrop()
    local z, cx, cy = self.z, self.cx, self.cy
    Draw.SetZ(-100)
    rect(cx, cy, 9000 * z, 9000 * z, BG, 1)
    Draw.SetZ(-90)
    local s = 90
    local hw, hh = 1400 * z, 620 * z
    local x0 = math.floor((cx - hw) / s) * s
    local y0 = math.floor((cy - hh) / s) * s
    local w = 1.2 * z
    local x = x0
    while x <= cx + hw do
        local major = math.floor(x / s + 0.5) % 5 == 0
        line(x, cy - hh, x, cy + hh, w, INK, major and 0.07 or 0.035)
        x = x + s
    end
    local y = y0
    while y <= cy + hh do
        local major = math.floor(y / s + 0.5) % 5 == 0
        line(cx - hw, y, cx + hw, y, w, INK, major and 0.07 or 0.035)
        y = y + s
    end
end

function Integration:DrawTitle(t)
    if t > T.axes.a then
        return
    end
    local z = self.z
    Draw.SetZ(-50)
    local base = TY - 330
    for i = 0, 27 do
        local x = -840 + i * 62
        local u = ease.outCubic(prog(t, 0.4 + i * 0.05, 1.4 + i * 0.05))
        local h = (90 + 150 * (f((x + 840) / 1680 * 5) - 1.4) / 2.2) * u
        rect(x, base + h * 0.5, 46, h, mix(CORAL, GOLD, i / 27), 0.14)
    end
    Draw.SetZ(10)
    local drawn = ease.inOutSine(prog(t, 0.3, 2.0))
    local pts = glyphPoints(0, TY + 250, 330, 1, drawn)
    pline(pts, 22 * z, VIOLET, 0.25 * 1, false)
    pline(pts, 12, mix(VIOLET, GOLD, drawn), 1, false)
    local ta = prog(t, 1.2, 2.0)
    local ts = 190 * lerp(0.86, 1, ease.outBack(ta))
    text("Integration", 0, TY - 70, ts, INK, ta, "center")
    local ua = ease.outCubic(prog(t, 1.9, 2.7))
    line(-260 * ua, TY - 190, 260 * ua, TY - 190, 8, GOLD, ua)
    text("the area under a curve, one thin slice at a time", 0, TY - 260, 48, MUTED, prog(t, 2.4, 3.2), "center")
end

function Integration:DrawSlices(from, to, u, alpha, grow)
    if STAGES[to] <= 256 then
        self:SliceRange(from, to, u, A, B, alpha, grow)
    else
        self:SliceRange(7, 7, 1, WIN_B, B, alpha, nil)
        self:SliceRange(from, to, u, A, WIN_B, alpha, nil)
    end
end

function Integration:SliceRange(from, to, u, xlo, xhi, alpha, grow)
    local nTo, nFrom = STAGES[to], STAGES[from]
    local dxTo, dxFrom = (B - A) / nTo, (B - A) / nFrom
    local i0 = math.max(1, math.floor((xlo - A) / dxTo + 1e-6) + 1)
    local i1 = math.min(nTo, math.ceil((xhi - A) / dxTo - 1e-6))
    if i1 < i0 then
        return
    end
    local z = self.z
    local split = from ~= to and u < 1
    local outlineA = nTo <= 32 and 1 or (nTo == 64 and 1 - u or 0)
    local gapK = nTo <= 32 and 0 or (nTo == 64 and u or 1)
    local fillA = nTo <= 8 and 0.5 or (nTo <= 32 and 0.58 or (nTo == 64 and lerp(0.58, 0.82, u) or 0.82))
    local gapRatio = nTo <= 256 and 0.14 or 0.08
    for i = i0, i1 do
        local cl = A + (i - 1) * dxTo
        local h = f(cl)
        if split then
            h = lerp(f(A + (math.ceil(i / 2) - 1) * dxFrom), h, u)
        end
        if grow then
            h = h * grow(i)
        end
        local x0, x1 = px(cl), px(cl + dxTo)
        local top = py(h)
        local hh = top - OY
        if hh > 0.5 then
            local tt = (cl + dxTo * 0.5 - A) / (B - A)
            local w = x1 - x0
            Draw.Rect((x0 + x1) * 0.5, OY + hh * 0.5, w - w * gapRatio * gapK, hh,
                lerp(CORAL[1], GOLD[1], tt), lerp(CORAL[2], GOLD[2], tt), lerp(CORAL[3], GOLD[3], tt), a255(alpha * fillA))
            if outlineA > 0.01 then
                local oc = mix(mix(CORAL, GOLD, tt), INK, 0.35)
                pline({x0, OY, x0, top, x1, top, x1, OY}, 3 * z, oc, alpha * outlineA * 0.95, false)
            end
        end
    end
end

function Integration:SliceAreas()
    local out = {}
    local dx = (B - A) / 4
    for i = 1, 4 do
        out[i] = f(A + (i - 1) * dx) * dx
    end
    return out
end

function Integration:DrawGraph(t)
    if t < T.pan.a or t > T.panUp.b then
        return
    end
    local z = self.z
    local from, to, u = self:StageAt(t)
    local w3 = 3.5 * z

    Draw.SetZ(2)
    local axU = seg(t, "axes")
    if t >= T.axes.a then
        local e = ease.outCubic(prog(axU, 0, 0.7))
        local x0, x1 = px(-0.4), px(5.55)
        line(x0, OY, lerp(x0, x1, e), OY, w3, INK, 0.8)
        local ey = ease.outCubic(prog(axU, 0.1, 0.8))
        local y0, y1 = OY - 20, py(3.95)
        line(OX, y0, OX, lerp(y0, y1, ey), w3, INK, 0.8)
        if e > 0.98 then
            line(x1, OY, x1 - 18 * z * 2, OY + 10 * z * 2, w3, INK, 0.8)
            line(x1, OY, x1 - 18 * z * 2, OY - 10 * z * 2, w3, INK, 0.8)
        end
        if ey > 0.98 then
            line(OX, y1, OX - 10 * z * 2, y1 - 18 * z * 2, w3, INK, 0.8)
            line(OX, y1, OX + 10 * z * 2, y1 - 18 * z * 2, w3, INK, 0.8)
        end
        local ta = prog(axU, 0.6, 1)
        if ta > 0 then
            for i = 0, 5 do
                line(px(i), OY - 9, px(i), OY + 9, w3 * 0.7, INK, ta * 0.7)
                text(tostring(i), px(i), OY - 40, 28, MUTED, ta, "center")
            end
            for i = 1, 3 do
                line(OX - 9, py(i), OX + 9, py(i), w3 * 0.7, INK, ta * 0.7)
                text(tostring(i), OX - 36, py(i), 28, MUTED, ta, "center")
            end
            text("x", px(5.55) + 32, OY, 36, MUTED, ta, "center")
            text("y", OX, py(3.95) + 38, 36, MUTED, ta, "center")
        end
    end

    local curveU = seg(t, "curve")
    local xr = A + (B - A) * ease.inOutSine(curveU)
    if t >= T.curve.a then
        local pts = curveU >= 1 and CURVE_FULL or curveFlat(xr, 361)
        Draw.SetZ(3)
        pline(pts, 22 * z, CYAN, 0.1, false)
        pline(pts, 7 * z, CYAN, 1, false)
        pline(pts, 2.4 * z, INK, 0.55, false)
        local la = prog(t, T.curve.b - 1.2, T.curve.b - 0.4)
        text("y = f(x)", px(1.95) + 70, py(3.72), 44, CYAN, la, "center")
    end

    Draw.SetZ(0)
    local areaA = 0.26
    areaA = lerp(areaA, 0.1, prog(t, T.big.a, T.big.a + 1.2))
    areaA = lerp(areaA, 0.4, ease.inOutSine(prog(t, T.smooth.a, T.smooth.a + 1.6)))
    local sweep = ease.inOutCubic(prog(t, T.area.a + 1.4, T.area.a + 3.4))
    if t >= T.area.a + 1.4 then
        poly(areaPoly(A + (B - A) * sweep), TEAL, areaA)
    end

    if t >= T.area.a then
        Draw.SetZ(4)
        local bu = ease.outCubic(prog(t, T.area.a, T.area.a + 1.2))
        for _, v in ipairs({A, B}) do
            dashed(px(v), OY, px(v), py(f(v)), 3.2 * z, 14 * z, 10 * z, GOLD, 0.9, bu)
        end
        local ba = ease.outBack(prog(t, T.area.a + 0.8, T.area.a + 1.5))
        text("a", px(A) - 6, OY - 104, 54 * clamp01(ba), GOLD, ba, "center")
        text("b", px(B) + 6, OY - 104, 54 * clamp01(ba), GOLD, ba, "center")
        local qa = prog(t, T.area.a + 2.4, T.area.a + 3.2) * (1 - prog(t, T.big.a, T.big.a + 0.6))
        if qa > 0 then
            local bob = math.sin(t * 2.4) * 10
            text("?", px(2.5), py(1.0) + bob, 210, INK, qa * (0.55 + 0.25 * math.sin(t * 2.4)), "center")
        end
    end

    if t >= T.big.a and t < T.smooth.b then
        Draw.SetZ(1)
        local alpha = 1 - ease.inOutSine(prog(t, T.smooth.a + 0.4, T.smooth.a + 2.2))
        local grow = nil
        local starts = {}
        for i = 1, 4 do
            starts[i] = T.big.a + 0.4 + (i - 1) * 0.65
        end
        if to == 1 then
            grow = function(i)
                return ease.outBack(prog(t, starts[i] + 0.3, starts[i] + 1.05))
            end
        end
        self:DrawSlices(from, to, u, alpha, grow)

        local dotA = to == 1 and 1 or (from == 1 and to == 2 and 1 - u or 0)
        if dotA > 0.01 then
            Draw.SetZ(5)
            for i = 1, 4 do
                local xl = A + (i - 1) * (B - A) / 4
                local d = prog(t, starts[i], starts[i] + 0.4)
                if d > 0 then
                    dashed(px(xl), OY, px(xl), py(f(xl)), 2.6 * z, 10 * z, 8 * z, INK, 0.7 * dotA, ease.outCubic(d))
                    local pop = ease.outBack(d)
                    circle(px(xl), py(f(xl)), 12 * z * pop, INK, dotA)
                    ring(px(xl), py(f(xl)), 17 * z * pop, 3 * z, CORAL, dotA)
                end
            end
        end
    end

    Draw.SetZ(6)
    self:DrawSliceNotes(t)
    Draw.SetZ(7)
    self:DrawPen(t, curveU, xr)
    self:DrawReadout(t, from, to, u)
end

function Integration:DrawSliceNotes(t)
    if t < T.bigIn.a or t > T.split1.a + 0.5 then
        return
    end
    local z = self.z
    local areas = self:SliceAreas()
    local dx = (B - A) / 4
    local x0, x1 = px(A), px(A + dx)
    local top = py(f(A))
    local fade = 1 - prog(t, T.bigOut.a, T.bigOut.a + 0.7)
    local w3 = 4 * z

    local pw = ease.outCubic(prog(t, T.bigIn.a + 0.7, T.bigIn.a + 1.4)) * fade
    if pw > 0 then
        local yb = OY - 58
        line(x0, yb, x1, yb, w3, TEAL, pw)
        line(x0, yb + 12, x0, yb - 12, w3, TEAL, pw)
        line(x1, yb + 12, x1, yb - 12, w3, TEAL, pw)
        local la = ease.outBack(prog(t, T.bigIn.a + 1.0, T.bigIn.a + 1.7)) * fade
        deltaX((x0 + x1) / 2, yb - 62, 62 * clamp01(la), TEAL, la)
        text("width", (x0 + x1) / 2, yb - 128, 30, MUTED, la * 0.9, "center")
    end

    local ph = ease.outCubic(prog(t, T.bigNote.a, T.bigNote.a + 0.7)) * fade
    if ph > 0 then
        local xb = OX - 62
        line(xb, OY, xb, lerp(OY, top, ph), w3, CORAL, ph)
        line(xb - 12, OY, xb + 12, OY, w3, CORAL, ph)
        line(xb - 12, lerp(OY, top, ph), xb + 12, lerp(OY, top, ph), w3, CORAL, ph)
        local la = ease.outBack(prog(t, T.bigNote.a + 0.4, T.bigNote.a + 1.1)) * fade
        text("f(x)", xb - 22, (OY + top) / 2 + 20, 62 * clamp01(la), CORAL, la, "right")
        text("height", xb - 22, (OY + top) / 2 - 36, 30, MUTED, la * 0.9, "right")
    end

    local pa = prog(t, T.bigNote.a + 1.6, T.bigNote.a + 2.4) * fade
    if pa > 0 then
        local cx, cy = (x0 + x1) / 2, OY + (top - OY) * 0.5 + 30
        local s = 44
        local total = 4.36 * s
        local lx = cx - total / 2
        text("f(x)", lx, cy, s, CORAL, pa, "left")
        text("x", lx + 2.14 * s, cy, s, INK, pa, "left")
        deltaX(lx + 3.64 * s, cy, s, TEAL, pa)
        local pe = prog(t, T.bigNote.a + 2.5, T.bigNote.a + 3.2) * fade
        text(string.format("= %.2f", areas[1]), cx, cy - 64, 52, INK, pe, "center")
    end

    local ps = prog(t, T.bigOut.a, T.bigOut.a + 0.3)
    if ps > 0 then
        for i = 1, 4 do
            local xl = A + (i - 1) * dx
            local a = prog(t, T.bigSum.a + 0.1 + (i - 1) * 0.35, T.bigSum.a + 0.5 + (i - 1) * 0.35)
            local gone = prog(t, T.split1.a, T.split1.a + 0.4)
            local h = (py(f(xl)) - OY)
            text(string.format("%.2f", areas[i]), px(xl + dx / 2), OY + h * 0.5, 50, INK, a * (1 - gone), "center")
        end
    end
end

function Integration:DrawPen(t, curveU, xr)
    local pa = (t >= T.curve.a and t < T.area.a + 0.4) and (1 - prog(t, T.curve.b, T.curve.b + 0.7)) or 0
    if pa <= 0 then
        return
    end
    local z = self.z
    local x, y = px(xr), py(f(xr))
    dashed(x, OY, x, y, 2.6 * z, 9 * z, 8 * z, INK, 0.55 * pa)
    dashed(OX, y, x, y, 2.6 * z, 9 * z, 8 * z, INK, 0.4 * pa)
    local pulse = 0.5 + 0.5 * math.sin(t * 8)
    circle(x, y, 26 * z, CYAN, 0.16 * pa * (0.6 + 0.4 * pulse))
    circle(x, y, 13 * z, INK, pa)
    ring(x, y, 21 * z, 3 * z, CYAN, pa)
    text(string.format("x = %.2f", xr), x + 16, OY + 34, 32, INK, pa, "left")
    text(string.format("f(x) = %.2f", f(xr)), x + 20, y + 54, 34, CYAN, pa, "center")
end

function Integration:DrawReadout(t, from, to, u)
    local a = prog(t, T.big.a + 1.0, T.big.a + 1.8) * (1 - prog(t, T.panUp.a, T.panUp.a + 0.6))
    if a <= 0 then
        return
    end
    local cx, cy = 470, 262
    local n, sum
    if t < T.split1.a then
        local areas = self:SliceAreas()
        sum = 0
        for i = 1, 4 do
            local s0 = T.big.a + 0.4 + (i - 1) * 0.65
            sum = sum + areas[i] * ease.outCubic(prog(t, s0 + 0.3, s0 + 1.0))
        end
        n = 4
    else
        n = (u > 0 and from ~= to) and STAGES[to] or STAGES[from]
        sum = lerp(SUMS[from], SUMS[to], u)
    end
    local lim = ease.inOutSine(prog(t, T.smooth.a + 0.6, T.smooth.a + 2.4))
    sum = lerp(sum, TRUE_AREA, lim)
    local inf = prog(t, T.smooth.a + 0.6, T.smooth.a + 1.4)
    Draw.SetZ(8)
    rrect(cx, cy, 612, 210, 30, VIOLET, a * 0.5)
    rrect(cx, cy, 606, 204, 27, PANEL, a * 0.96)
    local xl, xr = cx - 264, cx + 264
    text("slices", xl, cy + 56, 32, MUTED, a, "left")
    text("sum of areas", xl, cy, 32, MUTED, a, "left")
    text("error", xl, cy - 56, 32, MUTED, a, "left")
    text(tostring(n), xr, cy + 56, 46, INK, a * (1 - inf), "right")
    if inf > 0 then
        infinity(xr - 34, cy + 56, 26, INK, a * inf, 5)
    end
    text(string.format("%.4f", sum), xr, cy, 46, GOLD, a, "right")
    text(string.format("%.4f", math.abs(TRUE_AREA - sum)), xr, cy - 56, 46, CORAL, a, "right")
end

function Integration:DrawBoard(t)
    if t < T.smooth.a + 1.0 then
        return
    end
    local z = self.z
    local S = 112
    local row = BY + 170
    local fs, fe, ft, fm, fr, fo = T.fSum, T.fExplain, T.fThin, T.fMorph, T.fResult, T.outro
    Draw.SetZ(10)

    local mAppr = ease.inOutCubic(prog(t, fm.a, fm.a + 1.0))
    local mGlyph = ease.inOutCubic(prog(t, fm.a + 0.4, fm.a + 2.0))
    local mLimit = ease.inOutCubic(prog(t, fm.a + 1.2, fm.a + 2.2))
    local mI = ease.inOutCubic(prog(t, fm.a + 0.8, fm.a + 1.6))
    local mD = ease.inOutCubic(prog(t, fm.a + 1.4, fm.a + 2.4))

    local aa = clamp01(ease.outBack(prog(t, fs.a + 0.1, fs.a + 0.8)))
    text("Area", -475, row - (1 - aa) * 40, S, INK, aa, "left")

    local ap = prog(t, fs.a + 0.5, fs.a + 1.0)
    local eqCol = mix(MUTED, INK, mAppr)
    approx(-155, row, S * 0.8, mAppr, eqCol, ap)

    local draw = ease.inOutSine(prog(t, fs.a + 0.8, fs.a + 2.0))
    local H = S * 1.9
    local gcol = mix(VIOLET, GOLD, mGlyph)
    local pts = glyphPoints(-10, row + 4, H, mGlyph, draw)
    pline(pts, 30 * 0.6, gcol, 0.14, false)
    pline(pts, 13, gcol, 1, false)

    local la = prog(t, fs.a + 1.8, fs.a + 2.4)
    text("n", lerp(-10, 72, mLimit), lerp(row + 146, row + 120, mLimit), 50, MUTED, la * (1 - mLimit), "center")
    text("i = 1", lerp(-10, -40, mLimit), lerp(row - 146, row - 130, mLimit), 44, MUTED, la * (1 - mLimit), "center")
    text("b", lerp(-10, 72, mLimit), lerp(row + 146, row + 120, mLimit), 56, GOLD, la * mLimit, "center")
    text("a", lerp(-10, -40, mLimit), lerp(row - 146, row - 130, mLimit), 56, GOLD, la * mLimit, "center")

    local pf = ease.outCubic(prog(t, fs.a + 2.0, fs.a + 2.7))
    local fy = row - (1 - pf) * 30
    local fx = 93
    local iw = 0.145 * S
    text("f(x", fx, fy, S, CORAL, pf, "left")
    text("i", fx + 1.27 * S, fy - 0.22 * S, S * 0.5, CORAL, pf * (1 - mI), "left")
    text(")", fx + 1.27 * S + iw * (1 - mI), fy, S, CORAL, pf, "left")

    local pd = ease.outCubic(prog(t, fs.a + 2.6, fs.a + 3.3))
    local dy = row - (1 - pd) * 30
    local left = 324 - 16 * mI
    deltaX(left + 0.72 * S * (1 - 0.3 * mD), dy, S * (1 - 0.3 * mD), TEAL, pd * (1 - mD))
    text("dx", left, dy, S, TEAL, pd * mD, "left")

    local hiC = math.sin(math.pi * prog(t, fe.a + 0.9, fe.a + 1.9))
    local hiT = math.sin(math.pi * prog(t, fe.a + 1.9, fe.a + 2.9))
    if hiC > 0 then
        rrect(fx + 105, row, 270, S * 1.05, 26, CORAL, 0.2 * hiC)
    end
    if hiT > 0 then
        rrect(left + 85, row, 215, S * 1.05, 26, TEAL, 0.2 * hiT)
    end

    self:DrawBoardPicture(t, row)

    local n1 = prog(t, ft.a + 0.2, ft.a + 0.8) * (1 - prog(t, fm.a, fm.a + 0.8))
    local n2 = prog(t, ft.a + 1.2, ft.a + 1.8) * (1 - prog(t, fm.a, fm.a + 0.8))
    if n1 > 0 then
        deltaX(190, BY + 20, 64, TEAL, n1)
        arrow(262, BY + 20, 352, BY + 20, 5, INK, n1)
        text("0", 395, BY + 20, 64, TEAL, n1, "center")
    end
    if n2 > 0 then
        text("n", 190, BY - 86, 64, GOLD, n2, "center")
        arrow(235, BY - 86, 325, BY - 86, 5, INK, n2)
        infinity(398, BY - 86, 40, GOLD, n2, 7)
    end

    local ra = prog(t, fr.a + 0.2, fr.a + 0.9)
    if ra > 0 then
        local num = TRUE_AREA * ease.outCubic(prog(t, fr.a + 0.8, fr.a + 3.0))
        local glow = 0.5 + 0.5 * math.sin(t * 3)
        rrect(0, BY - 105, 700, 200, 44, GOLD, ra * (0.18 + 0.12 * glow))
        rrect(0, BY - 105, 690, 190, 40, PANEL, ra * 0.97)
        text("from a = 0 to b = 5", 0, BY - 40, 36, MUTED, ra, "center")
        text(string.format("%.3f", num), 0, BY - 128, 130, GOLD, ra, "center")
    end

    local oa = prog(t, fo.a + 0.2, fo.a + 1.0)
    if oa > 0 then
        text("Integration is adding up", 0, BY - 300, 58, INK, oa, "center")
        text("infinitely many, infinitely thin slices", 0, BY - 368, 58, GOLD, prog(t, fo.a + 0.7, fo.a + 1.5), "center")
    end
end

function Integration:DrawBoardPicture(t, row)
    local fe, ft, fm, fr = T.fExplain, T.fThin, T.fMorph, T.fResult
    local pa = prog(t, fe.a, fe.a + 0.8) * (1 - prog(t, fm.a + 0.2, fm.a + 1.0))
    if pa <= 0 then
        return
    end
    local z = self.z
    local base = BY - 300
    local left, right = -640, -140
    local function yc(x)
        return base + 70 + 62 * math.sin((x - left) / (right - left) * 2.6)
    end
    local curve = {}
    for i = 0, 40 do
        local x = left + (right - left) * i / 40
        curve[#curve + 1] = x
        curve[#curve + 1] = yc(x)
    end
    local wide = lerp(170, 9, ease.inOutCubic(prog(t, ft.a, ft.a + 2.6)))
    local x0 = left + 30
    local gr = ease.outCubic(prog(t, fe.a + 0.3, fe.a + 1.0))

    line(left - 20, base, right + 20, base, 3 * z, INK, pa * 0.6)
    local tile = prog(t, fe.a + 2.5, fe.a + 3.1)
    if tile > 0 then
        local k = math.floor((right - (x0 + wide)) / wide)
        for i = 1, math.min(k, 60) do
            local xl = x0 + i * wide
            local h = yc(xl) - base
            local gap = math.min(2.5, wide * 0.15)
            rect(xl + wide / 2, base + h / 2, wide - gap, h, mix(CORAL, GOLD, i / math.max(k, 1)), 0.38 * pa * tile)
        end
    end
    local h0 = (yc(x0) - base) * gr
    rect(x0 + wide / 2, base + h0 / 2, wide - math.min(2.5, wide * 0.15), h0, CORAL, 0.8 * pa)
    pline(curve, 5 * z, CYAN, pa, false)

    local hb = ease.outCubic(prog(t, fe.a + 0.9, fe.a + 1.5)) * pa
    if hb > 0 then
        local xb = x0 - 20
        local top = base + (yc(x0) - base)
        line(xb, base, xb, top, 3.5 * z, CORAL, hb)
        line(xb - 9, base, xb + 9, base, 3.5 * z, CORAL, hb)
        line(xb - 9, top, xb + 9, top, 3.5 * z, CORAL, hb)
        text("f(x)", xb - 18, (base + top) / 2, 44, CORAL, hb, "right")
    end
    local wb = ease.outCubic(prog(t, fe.a + 1.8, fe.a + 2.4)) * pa
    if wb > 0 then
        local yb = base - 34
        line(x0, yb, x0 + wide, yb, 3.5 * z, TEAL, wb)
        line(x0, yb + 9, x0, yb - 9, 3.5 * z, TEAL, wb)
        line(x0 + wide, yb + 9, x0 + wide, yb - 9, 3.5 * z, TEAL, wb)
        local sw = prog(wide, 60, 30)
        deltaX(x0 + wide / 2, yb - 52, 50, TEAL, wb * (1 - sw))
        text("dx", x0 + wide / 2, yb - 52, 50, TEAL, wb * sw, "center")
    end
end

function Integration:DrawHud(t)
    local z, cx, cy = self.z, self.cx, self.cy
    Draw.SetZ(60)
    for _, c in ipairs(CAPS) do
        local a = T[c[1]].a + 0.25
        local b = T[c[2]].b - 0.25
        local vis = prog(t, a, a + 0.45) * (1 - prog(t, b - 0.45, b))
        if vis > 0.01 then
            local w = math.max(tw(c[3], 22), tw(c[4], 36)) * z + 120 * z
            local y = cy - 540 * z + 104 * z - (1 - vis) * 16 * z
            rrect(cx, y, w, 104 * z, 26 * z, BG, vis * 0.82)
            text(c[3], cx, y + 24 * z, 22 * z, GOLD, vis, "center")
            text(c[4], cx, y - 14 * z, 36 * z, INK, vis, "center")
        end
    end
    local bw = 1500 * z
    local by = cy - 540 * z + 24 * z
    line(cx - bw / 2, by, cx + bw / 2, by, 4 * z, INK, 0.12)
    local p = t / TOTAL
    line(cx - bw / 2, by, cx - bw / 2 + bw * p, by, 4 * z, GOLD, 0.9)
    circle(cx - bw / 2 + bw * p, by, 7 * z, GOLD, 1)
    text("HOW INTEGRATION WORKS", cx, cy + 540 * z - 38 * z, 22 * z, MUTED, 0.55, "center")

    if t >= T.diveIn.a and t < T.diveOut.b then
        local a = prog(t, T.diveIn.a + 0.8, T.diveIn.a + 1.4) * (1 - prog(t, T.diveOut.a, T.diveOut.a + 0.5))
        local from, to, u = self:StageAt(t)
        local n = (u > 0 and from ~= to) and STAGES[to] or STAGES[from]
        local y = cy + 540 * z - 110 * z
        rrect(cx, y, 330 * z, 80 * z, 22 * z, BG, a * 0.85)
        text("slices  n = " .. n, cx, y, 40 * z, GOLD, a, "center")
    end

    local fade = 1 - prog(t, 0, 0.9)
    fade = math.max(fade, prog(t, T.fade.a, T.fade.b))
    if fade > 0.002 then
        Draw.SetZ(100)
        rect(cx, cy, 9000 * z, 9000 * z, BG, fade)
    end
end

function Integration:OnRender()
    local t = self.t
    self:DrawBackdrop()
    self:DrawTitle(t)
    self:DrawGraph(t)
    self:DrawBoard(t)
    self:DrawHud(t)
end

return Integration
