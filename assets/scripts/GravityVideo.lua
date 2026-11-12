local Gravity = {}

local LOOP = true

local BG = {9, 12, 26}
local PANEL = {20, 26, 50}
local INK = {240, 244, 255}
local MUTED = {140, 152, 184}
local CYAN = {88, 206, 255}
local TEAL = {62, 214, 176}
local CORAL = {255, 126, 96}
local GOLD = {255, 206, 96}
local VIOLET = {172, 142, 255}
local PINK = {255, 128, 190}
local OCEAN = {40, 110, 210}
local LAND = {70, 190, 120}

local tl = Timeline.New({
    {"title", 4.5},
    {"fall", 9},
    {"accel", 12},
    {"newton", 12},
    {"earth", 11},
    {"orbit", 16},
    {"curve", 9},
    {"outro", 6.5},
})
local TOTAL = tl:Total()

local FADE = 1

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

local function eio(u)
    if u < 0.5 then
        return 4 * u * u * u
    end
    local v = -2 * u + 2
    return 1 - v * v * v / 2
end

local function eout(u)
    local v = 1 - u
    return 1 - v * v * v
end

local function eback(u)
    local c1 = 1.70158
    local c3 = c1 + 1
    local v = u - 1
    return 1 + c3 * v * v * v + c1 * v * v
end

local function a255(a)
    if a == nil then
        a = 1
    end
    return math.floor(255 * clamp01(a * FADE) + 0.5)
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

local function rect(x, y, w, h, c, a, rot)
    Draw.Rect(x, y, w, h, rgba(c, a))
end

local function rrect(x, y, w, h, r, c, a)
    Draw.RoundRect(x, y, w, h, r, rgba(c, a))
end

local function pline(pts, w, c, a, closed)
    local r, g, b, al = rgba(c, a)
    Draw.Polyline(pts, w, r, g, b, al, closed and true or false)
end

local function poly(pts, c, a)
    Draw.Polygon(pts, rgba(c, a))
end

local function text(str, x, cy, size, c, a, align)
    if a ~= nil and a * FADE <= 0.004 then
        return
    end
    local r, g, b, al = rgba(c or INK, a)
    Draw.Text(str, x, cy + size * 0.52, size, r, g, b, al, align or "center")
end

local function tw(str, size)
    local w = Draw.MeasureText(str, size)
    return w
end

local function glow(x, y, r, c, a)
    local steps = r > 300 and 28 or 12
    for i = 1, steps do
        local k = i / steps
        circle(x, y, r * (1.9 - 1.5 * k), c, a * (0.7 / steps) * k)
    end
end

local function ball(x, y, r, c, a)
    a = a or 1
    glow(x, y, r * 1.6, c, a)
    circle(x, y, r, mix(c, {0, 0, 0}, 0.25), a)
    circle(x - r * 0.12, y + r * 0.12, r * 0.86, c, a)
    circle(x - r * 0.35, y + r * 0.38, r * 0.34, mix(c, {255, 255, 255}, 0.55), a * 0.8)
end

local function panel(x, y, w, h, a, border)
    rrect(x, y, w, h, 22, {0, 0, 0}, 0.35 * (a or 1))
    rrect(x, y, w - 4, h - 4, 20, PANEL, 0.94 * (a or 1))
    if border then
        Draw.RoundRect(x, y, w, h, 22, rgba(border, 0.0))
    end
end

local function formula(parts, cx, cy, size, a)
    local widths = {}
    local total = 0
    for i, p in ipairs(parts) do
        widths[i] = tw(p[1], size)
        total = total + widths[i]
    end
    local x = cx - total / 2
    for i, p in ipairs(parts) do
        text(p[1], x, cy, size, p[2] or INK, a, "left")
        if p.strike then
            local sx0, sx1 = x - 2, x + widths[i] + 2
            local k = clamp01(p.strike)
            line(sx0, cy - size * 0.38, lerp(sx0, sx1, k), cy + size * 0.38, 5, CORAL, a)
        end
        x = x + widths[i]
    end
    return total
end

local function sci(mant, exp, unit, x, cy, size, c, a)
    local s1 = mant .. " \195\151 10"
    local w1 = tw(s1, size)
    text(s1, x, cy, size, c, a, "left")
    text(exp, x + w1 + 2, cy + size * 0.38, size * 0.6, c, a, "left")
    local w2 = tw(exp, size * 0.6)
    if unit ~= "" then
        text(unit, x + w1 + w2 + 10, cy, size, c, a, "left")
    end
end

local stars = {}
do
    local seed = 12345
    local function rnd()
        seed = (seed * 1103515245 + 12345) % 2147483648
        return seed / 2147483648
    end
    for i = 1, 170 do
        stars[i] = {x = (rnd() - 0.5) * 2100, y = (rnd() - 0.5) * 1200, r = 0.8 + rnd() * 2.0, p = rnd() * 6.28, s = 0.6 + rnd() * 1.8}
    end
end

local GM_ORBIT = 250 * 250 * 230
local ORBIT_R = 230
local ORBIT_PLANET = {-250, 40}
local PLANET_R = 170
local SHOT_FACTORS = {0.55, 0.8, 1.0, 1.13}
local SHOT_COLORS = {CORAL, PINK, TEAL, VIOLET}
local shots = {}

local function buildShots()
    for k, factor in ipairs(SHOT_FACTORS) do
        local x, y = 0, ORBIT_R
        local vx, vy = factor * 250, 0
        local pts = {x, y}
        local dt = 1 / 240
        local t = 0
        local a0 = 2 / ORBIT_R - (factor * 250) ^ 2 / GM_ORBIT
        local period = 2 * math.pi * math.sqrt((1 / a0) ^ 3 * GM_ORBIT) / math.sqrt(GM_ORBIT) * math.sqrt(GM_ORBIT)
        period = 2 * math.pi * math.sqrt(((1 / a0)) ^ 3 / GM_ORBIT)
        local tEnd = math.min(period * 1.06, 14)
        local hit = false
        local count = 0
        while t < tEnd do
            local r2 = x * x + y * y
            local r = math.sqrt(r2)
            local ax, ay = -GM_ORBIT * x / (r2 * r), -GM_ORBIT * y / (r2 * r)
            vx = vx + ax * dt
            vy = vy + ay * dt
            x = x + vx * dt
            y = y + vy * dt
            t = t + dt
            count = count + 1
            if math.sqrt(x * x + y * y) <= PLANET_R + 4 then
                hit = true
                pts[#pts + 1] = x
                pts[#pts + 1] = y
                break
            end
            if count % 4 == 0 then
                pts[#pts + 1] = x
                pts[#pts + 1] = y
            end
        end
        shots[k] = {pts = pts, hit = hit, n = #pts / 2}
    end
end

local function shotPoint(shot, p)
    local idx = 1 + p * (shot.n - 1)
    local i = math.floor(idx)
    local f = idx - i
    if i >= shot.n then
        return shot.pts[2 * shot.n - 1], shot.pts[2 * shot.n], shot.n
    end
    local x0, y0 = shot.pts[2 * i - 1], shot.pts[2 * i]
    local x1, y1 = shot.pts[2 * i + 1], shot.pts[2 * i + 2]
    return lerp(x0, x1, f), lerp(y0, y1, f), i
end

local function drawTrail(shot, p, c, a, width)
    local idx = 1 + p * (shot.n - 1)
    local last = math.floor(idx)
    if last < 2 then
        return
    end
    local chunks = 5
    local per = math.max(2, math.ceil(last / chunks))
    local first = 1
    for k = 1, chunks do
        local from = (k - 1) * per
        local to = math.min(last, k * per + 1)
        if from >= last then
            break
        end
        local pts = {}
        for i = math.max(1, from), to do
            pts[#pts + 1] = ORBIT_PLANET[1] + shot.pts[2 * i - 1]
            pts[#pts + 1] = ORBIT_PLANET[2] + shot.pts[2 * i]
        end
        if #pts >= 4 then
            pline(pts, width, c, a * (0.25 + 0.75 * k / chunks), false)
        end
    end
end

function Gravity:OnCreate()
    self.t = 0
    local start = os.getenv("GRAVITY_START")
    if start then
        self.t = tonumber(start) or 0
    end
    buildShots()
    self.camera = Scene.Find("Camera")
    if self.camera then
        self.camera:GetCamera():SetFollow("")
        self.camera:SetPosition(0, 0)
        self.camera:GetCamera():SetZoom(1)
    end
end

function Gravity:OnUpdate(dt)
    self.t = self.t + dt
    if self.t >= TOTAL then
        self.t = LOOP and 0 or TOTAL - 0.001
    end
end

function Gravity:DrawBackdrop(t)
    FADE = 1
    Draw.SetZ(-100)
    rect(0, 0, 4000, 2400, BG, 1)
    glow(-760, 380, 900, VIOLET, 0.55)
    glow(820, -420, 1000, CYAN, 0.4)
    Draw.SetZ(-95)
    for _, s in ipairs(stars) do
        local tw2 = 0.55 + 0.45 * math.sin(t * s.s + s.p)
        circle(s.x, s.y, s.r, INK, 0.12 + 0.35 * tw2 * (s.r / 2.8))
    end
end

local function chrome(t, name, index, label, kicker, headline, sub)
    local a = tl:Window(t, name, 0.5)
    if a <= 0 then
        return
    end
    FADE = 1
    Draw.SetZ(40)
    rect(0, -470, 1960, 230, BG, 0.55 * a)
    rect(0, -352, 1960, 3, CYAN, 0.12 * a)
    text(string.format("%02d", index) .. "   " .. label, -900, 495, 26, GOLD, a, "left")
    if kicker then
        text(kicker, 0, -392, 26, GOLD, a)
    end
    text(headline, 0, -442, 54, INK, a)
    if sub then
        text(sub, 0, -503, 28, MUTED, a)
    end
end

local function caption(t, name, items)
    local from, to = tl:Span(name)
    local u = prog(t, from, to)
    local a = tl:Window(t, name, 0.5)
    if a <= 0 then
        return
    end
    local pick = items[#items]
    local idx = #items
    for i, item in ipairs(items) do
        if u < item.to then
            pick = item
            idx = i
            break
        end
    end
    local start = idx == 1 and 0 or items[idx - 1].to
    local fin = pick.to
    local local_a = math.min(prog(u, start, start + 0.04), 1 - prog(u, fin - 0.03, fin))
    if idx == #items then
        local_a = prog(u, start, start + 0.04)
    end
    if idx == 1 then
        local_a = 1 - prog(u, fin - 0.03, fin)
        local_a = math.min(local_a, prog(u, 0, 0.04))
    end
    FADE = 1
    Draw.SetZ(60)
    text(pick.head, 0, -442, 54, INK, a * clamp01(local_a))
    if pick.sub then
        text(pick.sub, 0, -503, 28, MUTED, a * clamp01(local_a))
    end
end

function Gravity:DrawTitle(t)
    local a = tl:Window(t, "title", 0.6)
    if a <= 0 then
        return
    end
    local from = 0
    local s = t - from
    FADE = a
    Draw.SetZ(0)
    local k = eback(prog(s, 0.2, 1.4))
    local py = lerp(-1000, -330, eout(prog(s, 0, 2.2)))
    local pcy = py - 1030
    glow(0, pcy, 1150, CYAN, 0.6)
    circle(0, pcy, 1000, mix(OCEAN, {0, 0, 0}, 0.45), 1)
    circle(30, pcy + 24, 990, mix(OCEAN, {0, 0, 0}, 0.2), 1)
    for i = 1, 4 do
        ring(0, pcy, 1000 + i * 12, 3, CYAN, 0.3 / i)
    end
    local ang = s * 0.9
    local ox, oy = 640 * math.cos(ang + 2.4), -20 + 170 * math.sin(ang + 2.4)
    local pts = {}
    for i = 0, 60 do
        local aa = ang + 2.4 - i * 0.025
        pts[#pts + 1] = 640 * math.cos(aa)
        pts[#pts + 1] = -20 + 170 * math.sin(aa)
    end
    pline(pts, 4, GOLD, 0.4, false)
    ball(ox, oy, 16, GOLD, 1)
    text("F = G m\226\130\129 m\226\130\130 / r\194\178", 0, 330, 70, INK, 0.07 * prog(s, 1, 2.5))
    local size = 190 * k
    if size > 4 then
        text("GRAVITY", 0, 130, size, INK, clamp01(k))
    end
    text("the force that shapes the universe", 0, -40, 44, MUTED, prog(s, 1.8, 2.8))
    text("a short, visual explanation", 0, -105, 30, CYAN, 0.8 * prog(s, 2.2, 3.2))
end

local function groundBall(ts, y0, yg, g, e)
    if ts <= 0 then
        return y0, 0, false
    end
    local t1 = math.sqrt(2 * (y0 - yg) / g)
    if ts < t1 then
        return y0 - 0.5 * g * ts * ts, g * ts, false
    end
    local v = g * t1 * e
    local rest = ts - t1
    for i = 1, 5 do
        local tb = 2 * v / g
        if rest < tb then
            return yg + v * rest - 0.5 * g * rest * rest, v - g * rest, true
        end
        rest = rest - tb
        v = v * e
    end
    return yg, 0, true
end

function Gravity:DrawFall(t)
    local a = tl:Window(t, "fall", 0.6)
    if a <= 0 then
        return
    end
    local from = tl:Span("fall")
    local s = t - from
    FADE = a
    local gY = -300
    local g = 1100
    local y0 = 320
    local drop = 1.7
    local ts = s - drop
    Draw.SetZ(0)
    rrect(0, gY - 76, 1700, 120, 16, PANEL, 0.9)
    line(-850, gY - 16, 850, gY - 16, 4, CYAN, 0.55)
    for i = -8, 8 do
        line(i * 100, gY - 16, i * 100 - 30, gY - 70, 2, CYAN, 0.1)
    end
    local balls = {
        {x = -200, r = 56, c = CORAL, name = "heavy", mass = "10 kg"},
        {x = 200, r = 26, c = CYAN, name = "light", mass = "1 kg"},
    }
    local landT = math.sqrt(2 * (y0 - (gY + 56)) / g)
    local tLand = drop + landT
    Draw.SetZ(10)
    local bar = 1 - prog(s, drop - 0.1, drop + 0.15)
    if bar > 0 then
        rrect(0, y0 + 80, 560, 14, 7, MUTED, 0.7 * bar)
        line(-200, y0 + 73, -200, y0 + 20, 2, MUTED, 0.5 * bar)
        line(200, y0 + 73, 200, y0 + 20, 2, MUTED, 0.5 * bar)
    end
    for _, b in ipairs(balls) do
        local yg = gY + b.r
        local y = groundBall(ts, y0, yg, g, 0.32)
        for k = 1, 14 do
            local tsg = ts - k * 0.075
            if tsg > 0 and tsg < landT + 0.01 then
                local yy = groundBall(tsg, y0, yg, g, 0.32)
                circle(b.x, yy, b.r, b.c, 0.2 * (1 - k / 15))
            end
        end
        ball(b.x, y, b.r, b.c, 1)
        text(b.name, b.x, y0 + b.r + 40 + 30 * 0, 32, b.c, bar * 0.95)
        text(b.mass, b.x, y0 + b.r + 5 + 0, 26, MUTED, bar * 0.9)
        local hitK = prog(ts, landT, landT + 0.5)
        if hitK > 0 and hitK < 1 then
            ring(b.x, gY - 8, b.r * (1 + 2.4 * hitK), 5 * (1 - hitK), b.c, 1 - hitK)
        end
    end
    Draw.SetZ(20)
    local clock = clamp01(math.max(ts, 0))
    local shown = math.min(math.max(ts, 0), landT)
    panel(0, 335, 520, 96, a)
    text(string.format("t = %.2f s", shown), 0, 335, 64, ts >= landT and GOLD or INK, 1)
    local boom = prog(ts, landT, landT + 0.3)
    if boom > 0 then
        text("They land together.", 0, 175, 58, GOLD, boom)
        text("Galileo, 1589", 0, 118, 30, MUTED, boom)
    end
end

function Gravity:DrawAccel(t)
    local a = tl:Window(t, "accel", 0.6)
    if a <= 0 then
        return
    end
    local from = tl:Span("accel")
    local s = t - from
    FADE = a
    local g = 9.8
    local px = 6.2
    local top = 330
    local bx = -520
    local tau = 4 * prog(s, 1.2, 9.6)
    local dist = 0.5 * g * tau * tau
    local v = g * tau
    Draw.SetZ(0)
    line(bx, top + 30, bx, top - px * 78.4 - 40, 2, MUTED, 0.25)
    for i = 0, 4 do
        local d = 0.5 * g * i * i
        local y = top - px * d
        local passed = tau >= i
        local ca = passed and 1 or 0.35
        line(bx - 210, y, bx + 70, y, 2, CYAN, 0.35 * ca)
        local lab = i == 0 and "start" or string.format("%d s", i)
        text(lab, bx - 262, y, 28, passed and INK or MUTED, ca, "right")
        if i > 0 then
            text(string.format("%.1f m", d), bx - 150, y, 26, passed and CYAN or MUTED, ca, "left")
        end
    end
    local y = top - px * dist
    Draw.SetZ(10)
    for k = 1, 16 do
        local tg = tau - k * 0.06
        if tg > 0 then
            circle(bx, top - px * 0.5 * g * tg * tg, 22, CORAL, 0.18 * (1 - k / 17))
        end
    end
    ball(bx, y, 24, CORAL, 1)
    local arrow = v * 4.6
    if arrow > 6 then
        Draw.Arrow(bx + 40, y, bx + 40 + arrow, y, 7, rgba(GOLD, 1))
        text(string.format("%.1f m/s", v), bx + 40 + arrow + 12, y + 34, 26, GOLD, 1, "left")
    end
    Draw.SetZ(20)
    local px0 = 470
    local k1 = prog(s, 0.4, 1.2)
    panel(px0, 235, 760, 230, a * k1)
    text("speed grows steadily", px0, 305, 28, MUTED, k1)
    formula({{"v", GOLD}, {" = g \195\151 t", INK}}, px0, 245, 84, k1)
    text("+9.8 m/s every second", px0, 180, 30, GOLD, k1 * 0.9)
    local k2 = prog(s, 3.0, 4.0)
    panel(px0, -5, 760, 210, a * k2)
    text("distance grows with the square of time", px0, 60, 28, MUTED, k2)
    formula({{"h", CYAN}, {" = \194\189 \195\151 g \195\151 t\194\178", INK}}, px0, 0, 74, k2)
    local k3 = prog(s, 5.0, 6.0)
    panel(px0, -190, 760, 170, a * k3)
    text(string.format("t = %.2f s", tau), px0 - 240, -150, 32, INK, k3, "left")
    text(string.format("v = %.1f m/s", v), px0 - 240, -195, 32, GOLD, k3, "left")
    text(string.format("h = %.1f m", dist), px0 + 60, -150, 32, CYAN, k3, "left")
    text("g = 9.8 m/s\194\178", px0 + 60, -195, 32, TEAL, k3, "left")
end

function Gravity:DrawNewton(t)
    local a = tl:Window(t, "newton", 0.6)
    if a <= 0 then
        return
    end
    local from, to = tl:Span("newton")
    local u = prog(t, from, to)
    local s = t - from
    FADE = a
    local m1 = 1 + eio(prog(u, 0.30, 0.42))
    local m2 = 1
    local r = 1 + eio(prog(u, 0.55, 0.68)) + eio(prog(u, 0.78, 0.92)) * 0.5
    local unit = 300
    local cx, cy = -330, 40
    local x1 = cx - r * unit / 2
    local x2 = cx + r * unit / 2
    local appear = eout(prog(u, 0.02, 0.12))
    local F = m1 * m2 / (r * r)
    local rad1 = 40 * m1 ^ (1 / 3) * appear
    local rad2 = 40 * m2 ^ (1 / 3) * appear
    Draw.SetZ(0)
    local fa = clamp01(prog(u, 0.08, 0.16))
    formula({{"F", GOLD}, {" = G \195\151 ", INK}, {"m", VIOLET}, {"\226\130\129", VIOLET}, {" \195\151 ", INK}, {"m", TEAL}, {"\226\130\130", TEAL}, {" / ", INK}, {"r\194\178", CYAN}}, cx, 380, 80, fa)
    Draw.SetZ(5)
    local dim = prog(u, 0.1, 0.2)
    line(x1, cy - 110, x2, cy - 110, 3, CYAN, 0.7 * dim)
    line(x1, cy - 95, x1, cy - 125, 3, CYAN, 0.7 * dim)
    line(x2, cy - 95, x2, cy - 125, 3, CYAN, 0.7 * dim)
    text(string.format("r = %.1f", r), (x1 + x2) / 2, cy - 150, 32, CYAN, dim)
    Draw.SetZ(10)
    local len = math.min(150 * F, 330) * appear
    if len > 4 then
        Draw.Arrow(x1 + rad1 + 6, cy, x1 + rad1 + 6 + len, cy, 8, rgba(GOLD, 1))
        Draw.Arrow(x2 - rad2 - 6, cy, x2 - rad2 - 6 - len, cy, 8, rgba(GOLD, 1))
    end
    ball(x1, cy, rad1, VIOLET, appear)
    ball(x2, cy, rad2, TEAL, appear)
    text(string.format("m\226\130\129 = %d", math.floor(m1 + 0.5)), x1, cy + rad1 + 44, 34, VIOLET, appear)
    text("m\226\130\130 = 1", x2, cy + rad2 + 44, 34, TEAL, appear)
    local readout = prog(u, 0.14, 0.2)
    text(string.format("pull = %.2f \195\151 the original", F), cx, cy - 215, 40, GOLD, readout)
    Draw.SetZ(20)
    local cxp, cyp, cw, ch = 640, -40, 460, 440
    panel(cxp, cyp, cw + 60, ch + 60, a * prog(u, 0.05, 0.15))
    local gx0, gy0 = cxp - cw / 2 + 30, cyp - ch / 2 + 30
    local function mapx(rr)
        return gx0 + (rr - 0.4) / 2.9 * (cw - 60)
    end
    local function mapy(ff)
        return gy0 + clamp01(ff / 6.5) * (ch - 70)
    end
    local ca = prog(u, 0.08, 0.2)
    line(gx0, gy0, gx0 + cw - 40, gy0, 3, MUTED, 0.6 * ca)
    line(gx0, gy0, gx0, gy0 + ch - 50, 3, MUTED, 0.6 * ca)
    text("distance r", gx0 + (cw - 40) / 2, gy0 - 36, 26, MUTED, ca)
    text("pull F", gx0 - 4, gy0 + ch - 36, 26, MUTED, ca, "left")
    local pts = {}
    for i = 0, 60 do
        local rr = 0.55 + i / 60 * 2.65
        pts[#pts + 1] = mapx(rr)
        pts[#pts + 1] = mapy(m1 * m2 / (rr * rr))
    end
    pline(pts, 5, CYAN, 0.9 * ca, false)
    local dx, dy = mapx(r), mapy(F)
    glow(dx, dy, 30, GOLD, ca)
    circle(dx, dy, 10, GOLD, ca)
    text("1/r\194\178", mapx(1.0), mapy(2.6) + 8, 34, CYAN, ca * 0.8)
end

function Gravity:DrawEarth(t)
    local a = tl:Window(t, "earth", 0.6)
    if a <= 0 then
        return
    end
    local from, to = tl:Span("earth")
    local s = t - from
    FADE = a
    local ex, ey, er = -420, -1580, 1340
    Draw.SetZ(0)
    for i = 6, 1, -1 do
        ring(ex, ey, er + i * 18, 6, CYAN, 0.05 * (7 - i))
    end
    circle(ex, ey, er, mix(OCEAN, {0, 0, 0}, 0.25), 1)
    circle(ex + 40, ey + 60, er - 20, OCEAN, 1)
    local lands = {{-150, 1245, 70}, {240, 1225, 80}, {-560, 1130, 60}, {560, 1120, 70}}
    for _, l in ipairs(lands) do
        circle(ex + l[1], ey + l[2], l[3], LAND, 0.95)
    end
    local cycle = 2.6
    local cyc = (s - 0.8) % cycle
    local started = s >= 0.8
    local by0, byg = 300, -190
    local fallT = 1.5
    local k = prog(cyc, 0, fallT)
    local y = lerp(by0, byg, k * k)
    if not started then
        y = by0
    end
    Draw.SetZ(10)
    local mlab = prog(s, 0.4, 1.2)
    ball(ex, y, 30, CORAL, 1)
    text("m", ex + 56, y + 8, 40, INK, mlab, "left")
    if started and cyc < fallT then
        Draw.Arrow(ex, y - 44, ex, y - 44 - 50 - 80 * k, 7, rgba(GOLD, 1))
        text("F = m g", ex + 60, y - 100, 34, GOLD, 1, "left")
    end
    text("Earth", ex, -300, 40, INK, 0.9)
    Draw.SetZ(20)
    local x0 = 480
    local rows = {
        {y = 350, note = "Earth pulls on a mass m", parts = function(p)
            return {{"F", GOLD}, {" = G \195\151 M \195\151 m / R\194\178", INK}}
        end},
        {y = 245, note = "Newton's second law: force = mass \195\151 acceleration", parts = function(p)
            return {{"F", GOLD}, {" = m \195\151 g", INK}}
        end},
        {y = 140, note = "so the two must be equal", parts = function(p)
            return {{"m", CORAL, strike = p}, {" \195\151 g = G \195\151 M \195\151 ", INK}, {"m", CORAL, strike = p}, {" / R\194\178", INK}}
        end},
        {y = 30, note = "the mass m cancels out", parts = function(p)
            return {{"g", TEAL}, {" = G \195\151 M / R\194\178", TEAL}}
        end},
    }
    local starts = {1.0, 2.4, 3.8, 5.6}
    for i, row in ipairs(rows) do
        local k2 = prog(s, starts[i], starts[i] + 0.7)
        if k2 > 0 then
            panel(x0, row.y, 860, 100, a * k2)
            text(row.note, x0, row.y + 34, 22, MUTED, k2)
            formula(row.parts(i == 3 and prog(s, 5.0, 5.6) or 0), x0, row.y - 14, i == 4 and 62 or 54, k2)
        end
    end
    local kn = prog(s, 6.6, 7.4)
    if kn > 0 then
        text("G", x0 - 360, -110, 34, VIOLET, kn, "left")
        sci("= 6.674", "-11", "", x0 - 320, -110, 34, MUTED, kn)
        text("M", x0 - 360, -158, 34, VIOLET, kn, "left")
        sci("= 5.97", "24", "kg", x0 - 320, -158, 34, MUTED, kn)
        text("R", x0 - 360, -206, 34, VIOLET, kn, "left")
        sci("= 6.371", "6", "m", x0 - 320, -206, 34, MUTED, kn)
    end
    local kr = eback(prog(s, 8.2, 9.2))
    if kr > 0.01 then
        local pop = clamp01(kr)
        text("g \226\137\136 9.81 m/s\194\178", x0 + 140, -160, 68 * math.max(kr, 0.01), GOLD, pop)
    end
end

function Gravity:DrawPlanet(px, py, a)
    glow(px, py, PLANET_R * 1.6, CYAN, 0.8 * a)
    for i = 5, 1, -1 do
        ring(px, py, PLANET_R + i * 9, 5, CYAN, 0.05 * (6 - i) * a)
    end
    circle(px, py, PLANET_R, mix(OCEAN, {0, 0, 0}, 0.3), a)
    circle(px + 10, py + 10, PLANET_R - 10, OCEAN, a)
    circle(px - 50, py + 70, 52, LAND, a * 0.95)
    circle(px + 40, py + 30, 40, LAND, a * 0.95)
    circle(px + 70, py - 60, 36, LAND, a * 0.95)
    circle(px - 70, py - 40, 44, LAND, a * 0.95)
    circle(px - 20, py - 100, 34, LAND, a * 0.95)
end

function Gravity:DrawOrbit(t)
    local a = tl:Window(t, "orbit", 0.6)
    if a <= 0 then
        return
    end
    local from, to = tl:Span("orbit")
    local s = t - from
    FADE = a
    local px, py = ORBIT_PLANET[1], ORBIT_PLANET[2]
    Draw.SetZ(0)
    self:DrawPlanet(px, py, 1)
    local launch = 1.0
    local slot = 3.0
    local current = 0
    for k = 1, 4 do
        if s >= launch + (k - 1) * slot then
            current = k
        end
    end
    Draw.SetZ(8)
    local hx, hy = px, py + PLANET_R
    poly({hx - 46, hy - 12, hx + 46, hy - 12, hx + 14, hy + 20, hx - 14, hy + 20}, {90, 100, 130}, 1)
    rect(hx + 22, hy + 24, 62, 20, {170, 180, 210}, 1)
    rect(hx + 62, hy + 24, 24, 24, {120, 130, 160}, 1)
    Draw.SetZ(12)
    for k = 1, 4 do
        local shot = shots[k]
        local c = SHOT_COLORS[k]
        local startK = launch + (k - 1) * slot
        local p = prog(s, startK, startK + slot * 0.95)
        if k < current then
            p = 1
        end
        if p > 0 then
            local dim = (k == current) and 1 or 0.55
            if k == 3 and current >= 3 then
                dim = (k == current) and 1 or 0.85
            end
            drawTrail(shot, p, c, dim, k == 3 and 6 or 5)
            local bx, by = shotPoint(shot, p)
            if k == current and p < 1 then
                glow(px + bx, py + by, 22, c, 1)
                circle(px + bx, py + by, 9, INK, 1)
            end
            if p >= 1 and shot.hit then
                local ex, ey = px + shot.pts[2 * shot.n - 1], py + shot.pts[2 * shot.n]
                local burst = prog(s, startK + slot * 0.95, startK + slot * 0.95 + 0.5)
                if k == current and burst < 1 then
                    ring(ex, ey, 10 + 40 * burst, 4 * (1 - burst), c, 1 - burst)
                end
            end
        end
        local flash = prog(s, startK, startK + 0.35)
        if k == current and flash < 1 then
            glow(px + 96, py + PLANET_R + 36, 40, GOLD, 1 - flash)
        end
    end
    local labels = {"0.55", "0.80", "1.00", "1.13"}
    Draw.SetZ(30)
    local lx = 560
    for k = 1, 4 do
        local startK = launch + (k - 1) * slot
        local ka = prog(s, startK, startK + 0.5)
        if ka > 0 then
            local yy = 330 - (k - 1) * 70
            circle(lx - 250, yy, 12, SHOT_COLORS[k], ka)
            text(string.format("v = %s \195\151 v", labels[k]), lx - 220, yy, 34, k == current and INK or MUTED, ka, "left")
            local ow = tw(string.format("v = %s \195\151 v", labels[k]), 34)
            text("orbit", lx - 220 + ow + 6, yy - 10, 18, MUTED, ka, "left")
        end
    end
    local kf = prog(s, 12.8, 13.6)
    if kf > 0 then
        panel(560, -90, 700, 260, a * kf)
        formula({{"v", TEAL}, {"orbit", MUTED}, {" = \226\136\154( G M / r )", INK}}, 560, -20, 56, kf)
        text("at the surface of the Earth:", 560, -90, 28, MUTED, kf)
        text("about 7.9 km/s  (28,000 km/h)", 560, -140, 36, GOLD, kf)
    end
end

local function gridHeight(x, z, depth)
    local r = math.sqrt(x * x + z * z)
    return -depth * 0.16 / math.sqrt(r * r + 0.09)
end

local function project(x, z, h, cy0)
    return x * 560, cy0 + z * 560 * 0.3 + h * 560 * 0.8
end

function Gravity:DrawCurve(t)
    local a = tl:Window(t, "curve", 0.6)
    if a <= 0 then
        return
    end
    local from, to = tl:Span("curve")
    local s = t - from
    FADE = a
    local depth = eio(prog(s, 0.6, 3.0))
    local cy0 = 40
    local lines = 21
    local span = 1.5
    Draw.SetZ(0)
    for pass = 1, 2 do
        for i = 0, lines - 1 do
            local f = -span + 2 * span * i / (lines - 1)
            local pts = {}
            for j = 0, 50 do
                local g = -span + 2 * span * j / 50
                local x, z
                if pass == 1 then
                    x, z = g, f
                else
                    x, z = f, g
                end
                local sx, sy = project(x, z, gridHeight(x, z, depth), cy0)
                pts[#pts + 1] = sx
                pts[#pts + 1] = sy
            end
            local depthA = pass == 1 and (0.25 + 0.75 * (f + span) / (2 * span)) or 0.55
            local col = mix(CYAN, VIOLET, depth * 0.6)
            pline(pts, 2.5, col, 0.55 * depthA, false)
        end
    end
    local omega = 1.9
    local phi = (s - 2.4) * omega
    local R = 0.75
    local orbitOn = prog(s, 2.4, 3.2)
    local mx, my = project(0, 0, gridHeight(0, 0, depth), cy0)
    my = my + 40
    local bx, bz = R * math.cos(phi), R * math.sin(phi)
    local bsx, bsy = project(bx, bz, gridHeight(bx, bz, depth), cy0)
    bsy = bsy + 16
    local function drawMass()
        Draw.SetZ(12)
        glow(mx, my, 90, GOLD, depth)
        ball(mx, my, 48 * clamp01(depth * 3), GOLD, 1)
    end
    local function drawBall()
        if orbitOn <= 0 then
            return
        end
        Draw.SetZ(14)
        local pts = {}
        for k = 0, 60 do
            local ph = phi - k * 0.035
            local tx, tz = R * math.cos(ph), R * math.sin(ph)
            local sx, sy = project(tx, tz, gridHeight(tx, tz, depth), cy0)
            pts[#pts + 1] = sx
            pts[#pts + 1] = sy + 16
        end
        pline(pts, 5, TEAL, 0.5 * orbitOn, false)
        ball(bsx, bsy, 17, TEAL, orbitOn)
    end
    if bz < 0 then
        drawBall()
        drawMass()
    else
        drawMass()
        drawBall()
    end
    Draw.SetZ(30)
    local k1 = prog(s, 0.4, 1.4)
    local k2 = prog(s, 3.4, 4.4)
    text("Einstein, 1915", -640, 440, 30, GOLD, k1, "left")
    text("Mass bends space.", -640, 385, 54, INK, k1, "left")
    text("Things then simply follow the curve.", -640, 320, 40, MUTED, k2, "left")
end

function Gravity:DrawOutro(t)
    local a = tl:Window(t, "outro", 0.8)
    if a <= 0 then
        return
    end
    local from, to = tl:Span("outro")
    local s = t - from
    FADE = a
    Draw.SetZ(10)
    text("Gravity, in three lines", 0, 400, 66, INK, prog(s, 0, 0.8))
    local cards = {
        {x = -620, c = TEAL, title = "everything falls alike", parts = {{"g", TEAL}, {" = 9.8 m/s\194\178", INK}}, note = "near the Earth's surface"},
        {x = 0, c = GOLD, title = "every mass pulls every mass", parts = {{"F", GOLD}, {" = G m\226\130\129m\226\130\130 / r\194\178", INK}}, note = "twice as far, a quarter as strong"},
        {x = 620, c = VIOLET, title = "orbits are endless falling", parts = {{"v", VIOLET}, {" = \226\136\154( G M / r )", INK}}, note = "fall fast enough, and you miss"},
    }
    for i, c in ipairs(cards) do
        local k = eout(prog(s, 0.6 + (i - 1) * 0.5, 1.5 + (i - 1) * 0.5))
        local yoff = (1 - k) * -60
        panel(c.x, 40 + yoff, 580, 420, a * k)
        rect(c.x, 40 + yoff + 190, 520, 6, c.c, k * 0.9)
        text(c.title, c.x, 130 + yoff, 34, c.c, k)
        formula(c.parts, c.x, 30 + yoff, 56, k)
        text(c.note, c.x, -80 + yoff, 28, MUTED, k)
    end
    text("Mass pulls. Space bends. Everything falls.", 0, -290, 46, INK, prog(s, 2.8, 3.8))
end

function Gravity:DrawHud(t)
    FADE = 1
    Draw.SetZ(45)
    local u = t / TOTAL
    rect(0, -538, 1920, 6, PANEL, 0.7)
    rect(-960 + 960 * u, -538, 1920 * u, 6, CYAN, 0.9)
    chrome(t, "fall", 1, "FALLING", nil, "Drop two things. Which lands first?", "heavy or light, big or small")
    chrome(t, "accel", 2, "ACCELERATION", nil, "Falling gets faster, at a steady rate", "the same push, every single second")
    local nw = {
        {to = 0.28, head = "Every mass pulls on every other mass", sub = "with equal force, in opposite directions"},
        {to = 0.5, head = "Double a mass: double the pull", sub = "the force scales with m\226\130\129 \195\151 m\226\130\130"},
        {to = 0.76, head = "Double the distance: a quarter of the pull", sub = "the inverse-square law"},
        {to = 1.01, head = "Triple the distance: a ninth", sub = "gravity weakens fast, but never reaches zero"},
    }
    chrome(t, "newton", 3, "NEWTON'S LAW", nil, "", nil)
    caption(t, "newton", nw)
    chrome(t, "earth", 4, "WHERE 9.8 COMES FROM", nil, "Earth's own mass sets g", "and the falling mass cancels out of the maths")
    local ob = {
        {to = 0.2, head = "Fire a cannonball from a mountain", sub = "gravity always pulls it down"},
        {to = 0.4, head = "Faster: it lands farther away", sub = "and the ground curves away beneath it"},
        {to = 0.62, head = "Fast enough: it keeps missing the ground", sub = "that is an orbit \226\128\148 falling around the Earth"},
        {to = 0.8, head = "Faster still: a stretched ellipse", sub = "swinging far out, then diving back in"},
        {to = 1.01, head = "Orbiting is falling, and missing", sub = "the Moon and the space station do exactly this"},
    }
    chrome(t, "orbit", 5, "ORBITS", nil, "", nil)
    caption(t, "orbit", ob)
    chrome(t, "curve", 6, "THE DEEPER PICTURE", nil, "Mass tells space how to curve", "space tells mass how to move")
end

function Gravity:OnRender()
    local t = self.t
    self:DrawBackdrop(t)
    self:DrawTitle(t)
    self:DrawFall(t)
    self:DrawAccel(t)
    self:DrawNewton(t)
    self:DrawEarth(t)
    self:DrawOrbit(t)
    self:DrawCurve(t)
    self:DrawOutro(t)
    self:DrawHud(t)
end

return Gravity
