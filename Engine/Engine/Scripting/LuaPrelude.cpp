#include "LuaModules.h"

#include "LuaHelpers.h"

#include <Engine/Core/Log.h>

#include <cstring>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *PreludeSource = R"LUA(
local progress = Math.Progress

Video = {Time = 0, Duration = nil, Driven = false, Recording = false}

function Video.Step(t, dt)
    if Video.Driven then
        return Video.Time
    end
    return (t or 0) + dt
end

Suggest = {}

local function editDistance(a, b)
    local la, lb = #a, #b
    if la == 0 then
        return lb
    end
    if lb == 0 then
        return la
    end
    local previous, current = {}, {}
    for j = 0, lb do
        previous[j] = j
    end
    for i = 1, la do
        current[0] = i
        local ca = a:byte(i)
        for j = 1, lb do
            local cost = (ca == b:byte(j)) and 0 or 1
            current[j] = math.min(previous[j] + 1, current[j - 1] + 1, previous[j - 1] + cost)
        end
        previous, current = current, previous
    end
    return previous[lb]
end

function Suggest.Closest(name, candidates)
    local wanted = tostring(name):lower()
    local best, bestDistance = nil, math.huge
    for _, candidate in ipairs(candidates) do
        local d = editDistance(wanted, tostring(candidate):lower())
        if d < bestDistance then
            best, bestDistance = candidate, d
        end
    end
    if best ~= nil and bestDistance <= math.max(2, math.floor(#wanted / 3)) then
        return best
    end
    return nil
end

function Suggest.Hint(name, candidates)
    local best = Suggest.Closest(name, candidates)
    if best == nil then
        return ""
    end
    return " (did you mean '" .. tostring(best) .. "'?)"
end

function Math.Integrate(fn, from, to, steps)
    steps = steps or 200
    if steps % 2 == 1 then
        steps = steps + 1
    end
    local h = (to - from) / steps
    local sum = fn(from) + fn(to)
    for i = 1, steps - 1 do
        sum = sum + fn(from + i * h) * ((i % 2 == 1) and 4 or 2)
    end
    return sum * h / 3
end

function Draw.Layer(z, fn)
    local previous = Draw.GetZ()
    Draw.SetZ(z)
    local ok, err = pcall(fn)
    Draw.SetZ(previous)
    if not ok then
        error(err, 0)
    end
end

Timeline = {}
Timeline.__index = Timeline

function Timeline.New(list)
    local timeline = setmetatable({names = {}, spans = {}, total = 0}, Timeline)
    if list ~= nil then
        for _, item in ipairs(list) do
            timeline:Add(item[1], item[2])
        end
    end
    return timeline
end

function Timeline:Add(name, seconds)
    if self.spans[name] ~= nil then
        error("Timeline.Add: segment '" .. tostring(name) .. "' already exists", 2)
    end
    if type(seconds) ~= "number" or seconds < 0 then
        error("Timeline.Add: seconds must be a number >= 0", 2)
    end
    local from = self.total
    self.spans[name] = {from, from + seconds}
    self.names[#self.names + 1] = name
    self.total = from + seconds
    return self
end

function Timeline:Span(name)
    local span = self.spans[name]
    if span == nil then
        error("Timeline: unknown segment '" .. tostring(name) .. "'" .. Suggest.Hint(name, self.names), 2)
    end
    return span[1], span[2]
end

function Timeline:Progress(t, name)
    local from, to = self:Span(name)
    return progress(t, from, to)
end

function Timeline:Window(t, name, fade)
    local from, to = self:Span(name)
    return Math.Window(t, from, to, fade)
end

function Timeline:Eased(t, name, curve)
    local u = self:Progress(t, name)
    if curve == nil then
        return u
    end
    if type(curve) == "string" then
        return Ease[curve](u)
    end
    return curve(u)
end

function Timeline:Active(t, name)
    local from, to = self:Span(name)
    return t >= from and t < to
end

function Timeline:Current(t)
    for _, name in ipairs(self.names) do
        local span = self.spans[name]
        if t < span[2] then
            return name, progress(t, span[1], span[2])
        end
    end
    return self.names[#self.names], self.names[1] and 1 or 0
end

function Timeline:Total()
    return self.total
end

function Effect.Chain(target, list, options)
    local handles = {}
    for _, item in ipairs(list) do
        local name, opts = item, nil
        if type(item) == "table" then
            name, opts = item[1], item[2]
        end
        handles[#handles + 1] = Effect.Play(target, name, opts)
    end
    return Tween.Sequence(handles, options)
end

function Effect.Stagger(targets, name, each, options)
    local handles = {}
    for i, target in ipairs(targets) do
        local opts = {}
        if options then
            for key, value in pairs(options) do
                opts[key] = value
            end
        end
        opts.delay = (opts.delay or 0) + (i - 1) * (each or 0.1)
        handles[#handles + 1] = Effect.Play(target, name, opts)
    end
    return handles
end

local effectCache = {}
setmetatable(Effect, {
    __index = function(_, key)
        local text = tostring(key)
        local name = text:sub(1, 1):lower() .. text:sub(2)
        local fn = effectCache[name]
        if fn == nil then
            if Effect.Info(name) == nil then
                error("Effect: unknown effect '" .. text .. "'" .. Suggest.Hint(text, Effect.Names()) .. " (see Effect.Names())", 2)
            end
            local play = Effect.Play
            fn = function(target, options)
                return play(target, name, options)
            end
            effectCache[name] = fn
        end
        return fn
    end
})

Backdrop = {}

function Backdrop.Fill(cx, cy, zoom, color)
    local size = 9000 * (zoom or 1)
    Draw.Rect(cx, cy, size, size, color or "bg")
end

function Backdrop.Grid(cx, cy, zoom, o)
    o = o or {}
    zoom = zoom or 1
    local step = o.step or 90
    local halfWidth, halfHeight = (o.halfWidth or 1400) * zoom, (o.halfHeight or 620) * zoom
    local major = o.major or 5
    local width = (o.width or 1.2) * zoom
    local color = o.color or "ink"
    local minorColor = Color.WithAlpha(color, o.minorAlpha or 0.035)
    local majorColor = Color.WithAlpha(color, o.majorAlpha or 0.07)
    local x = math.floor((cx - halfWidth) / step) * step
    while x <= cx + halfWidth do
        local isMajor = math.floor(x / step + 0.5) % major == 0
        Draw.Line(x, cy - halfHeight, x, cy + halfHeight, width, isMajor and majorColor or minorColor)
        x = x + step
    end
    local y = math.floor((cy - halfHeight) / step) * step
    while y <= cy + halfHeight do
        local isMajor = math.floor(y / step + 0.5) % major == 0
        Draw.Line(cx - halfWidth, y, cx + halfWidth, y, width, isMajor and majorColor or minorColor)
        y = y + step
    end
end

local easeCache = {}
Ease = setmetatable({}, {
    __index = function(_, name)
        local key = tostring(name)
        local fn = easeCache[key]
        if fn == nil then
            local curve = Curves[key] or Curves["ease" .. key:sub(1, 1):upper() .. key:sub(2)]
            if curve == nil or type(curve) == "function" then
                local names = {}
                for _, n in ipairs(Curves.Names()) do
                    if #n > 4 and n:sub(1, 4) == "ease" then
                        n = n:sub(5, 5):lower() .. n:sub(6)
                    end
                    names[#names + 1] = n
                end
                error("Ease: unknown curve '" .. key .. "'" .. Suggest.Hint(key, names) .. " (see Curves.Names())", 2)
            end
            local evaluate = Curves.Evaluate
            fn = function(t)
                return evaluate(curve, t)
            end
            easeCache[key] = fn
        end
        return fn
    end
})
)LUA";

        constexpr const char *RigSource = R"LUA(
CameraRig = {}
CameraRig.__index = CameraRig

local function toPose(p)
    return {p[1] or p.x or 0, p[2] or p.y or 0, p[3] or p.zoom or 1}
end

local function resolveCurve(curve)
    if curve == nil then
        return Ease.inOutCubic
    end
    if type(curve) == "string" then
        return Ease[curve]
    end
    return curve
end

function CameraRig.New(timeline, start)
    return setmetatable({timeline = timeline, start = toPose(start or {0, 0, 1}), steps = {}, sway = nil}, CameraRig)
end

function CameraRig:Move(segment, to, curve, options)
    self.timeline:Span(segment)
    local previous = self.steps[#self.steps]
    local target = toPose(to)
    self.steps[#self.steps + 1] = {
        segment = segment,
        from = previous and previous.finish or self.start,
        to = target,
        finish = target,
        curve = resolveCurve(curve),
        bump = options and options.bump or 0,
    }
    return self
end

function CameraRig:Track(segment, fn)
    self.timeline:Span(segment)
    local x, y, zoom = fn(1)
    self.steps[#self.steps + 1] = {segment = segment, fn = fn, finish = {x, y, zoom}}
    return self
end

function CameraRig:Sway(x, y, zoom)
    self.sway = {x or 5, y or 4, zoom or 0.004}
    return self
end

function CameraRig:At(t)
    local pose = self.start
    local current = nil
    for _, step in ipairs(self.steps) do
        local from, to = self.timeline:Span(step.segment)
        if t < from then
            break
        end
        if t <= to then
            local u = Math.Progress(t, from, to)
            if step.fn then
                local x, y, zoom = step.fn(u)
                current = {x, y, zoom}
            else
                local k = step.curve(u)
                current = {
                    Math.Lerp(step.from[1], step.to[1], k),
                    Math.Lerp(step.from[2], step.to[2], k),
                    Math.Lerp(step.from[3], step.to[3], k) + step.bump * math.sin(math.pi * k),
                }
            end
            break
        end
        pose = step.finish
    end
    local p = current or pose
    local x, y, zoom = p[1], p[2], p[3]
    local sway = self.sway
    if sway then
        x = x + sway[1] * zoom * math.sin(t * 0.55)
        y = y + sway[2] * zoom * math.sin(t * 0.71 + 1.3)
        zoom = zoom * (1 + sway[3] * math.sin(t * 0.43))
    end
    return x, y, zoom
end

function CameraRig:Apply(t, camera)
    local x, y, zoom = self:At(t)
    if camera ~= nil and camera:IsValid() then
        camera:SetPosition(x, y)
        camera:GetCamera():SetZoom(zoom)
    end
    return x, y, zoom
end

Captions = {}
Captions.__index = Captions

local captionDefaults = {
    background = "#0e111e", title = "#ffce60", text = "#ecf1fc", muted = "#8a94b0", bar = "#ffce60",
    titleSize = 22, textSize = 36, fade = 0.45, inset = 0.25, progress = true, footer = nil,
}

function Captions.New(timeline, options)
    local captions = setmetatable({timeline = timeline, items = {}, options = {}}, Captions)
    for key, value in pairs(captionDefaults) do
        captions.options[key] = value
    end
    if options then
        for key, value in pairs(options) do
            captions.options[key] = value
        end
    end
    return captions
end

function Captions:Add(from, to, title, text)
    self.timeline:Span(from)
    self.timeline:Span(to)
    self.items[#self.items + 1] = {from = from, to = to, title = title, text = text or ""}
    return self
end

function Captions:Visible(t)
    local o = self.options
    local list = {}
    for _, item in ipairs(self.items) do
        local from = self.timeline:Span(item.from)
        local _, last = self.timeline:Span(item.to)
        local a, b = from + o.inset, last - o.inset
        local alpha = Math.Progress(t, a, a + o.fade) * (1 - Math.Progress(t, b - o.fade, b))
        if alpha > 0.01 then
            list[#list + 1] = {title = item.title, text = item.text, alpha = alpha}
        end
    end
    return list
end

function Captions:Draw(t, cx, cy, scale)
    cx, cy, scale = cx or 0, cy or 0, scale or 1
    local o = self.options
    Draw.Layer(60, function()
        for _, c in ipairs(self:Visible(t)) do
            local titleW = Draw.MeasureText(c.title, o.titleSize * scale)
            local textW = Draw.MeasureText(c.text, o.textSize * scale)
            local w = math.max(titleW, textW) + 120 * scale
            local y = cy - 540 * scale + 104 * scale - (1 - c.alpha) * 16 * scale
            Draw.RoundRect(cx, y, w, 104 * scale, 26 * scale, Color.WithAlpha(o.background, c.alpha * 0.82))
            Draw.Label(c.title, cx, y + 24 * scale, o.titleSize * scale, Color.WithAlpha(o.title, c.alpha), "center")
            Draw.Label(c.text, cx, y - 14 * scale, o.textSize * scale, Color.WithAlpha(o.text, c.alpha), "center")
        end
        if o.progress then
            local width = 1500 * scale
            local y = cy - 540 * scale + 24 * scale
            local p = Math.Clamp01(t / math.max(self.timeline:Total(), 0.0001))
            Draw.Line(cx - width / 2, y, cx + width / 2, y, 4 * scale, Color.WithAlpha("#ffffff", 0.12))
            Draw.Line(cx - width / 2, y, cx - width / 2 + width * p, y, 4 * scale, Color.WithAlpha(o.bar, 0.9))
            Draw.Circle(cx - width / 2 + width * p, y, 7 * scale, o.bar)
        end
        if o.footer then
            Draw.Label(o.footer, cx, cy + 540 * scale - 38 * scale, o.titleSize * scale, Color.WithAlpha(o.muted, 0.55), "center")
        end
    end)
end
)LUA";

        constexpr const char *PlotSource = R"LUA(
Axes = {}
Axes.__index = Axes

local function outCubic(u)
    local v = 1 - u
    return 1 - v * v * v
end

local function ticks(min, max, step, includeZero)
    local list = {}
    if step == nil or step <= 0 then
        return list
    end
    local first = math.ceil(min / step - 1e-9)
    local last = math.floor(max / step + 1e-9)
    for i = first, last do
        local v = i * step
        if includeZero or math.abs(v) > 1e-9 then
            list[#list + 1] = v
        end
    end
    return list
end

local function fade(color, alpha)
    return Color.WithAlpha(color, alpha)
end

function Axes.New(o)
    local ax = setmetatable({}, Axes)
    ax.ox, ax.oy = o.origin[1], o.origin[2]
    ax.sx, ax.sy = o.scale[1], o.scale[2]
    ax.xmin, ax.xmax = o.x[1], o.x[2]
    ax.ymin, ax.ymax = o.y[1], o.y[2]
    ax.xticks = o.xticks or ticks(ax.xmin, ax.xmax, o.x[3], true)
    ax.yticks = o.yticks or ticks(ax.ymin, ax.ymax, o.y[3], false)
    ax.color = o.color or "ink"
    ax.tickColor = o.tickColor or "muted"
    ax.width = o.width or 3.5
    ax.labels = o.labels or {x = "x", y = "y"}
    ax.fontSize = o.fontSize or 28
    return ax
end

function Axes:P(x, y)
    return self.ox + self.sx * x, self.oy + self.sy * y
end

function Axes:Draw(p, zoom)
    p = p or 1
    zoom = zoom or 1
    local w = self.width * zoom
    local ink = self.color
    local ex = outCubic(Math.Progress(p, 0, 0.7))
    local ey = outCubic(Math.Progress(p, 0.1, 0.8))
    local ta = Math.Progress(p, 0.6, 1)
    local x0, x1 = self:P(self.xmin, 0), self:P(self.xmax, 0)
    local _, y0 = self:P(0, self.ymin)
    local _, y1 = self:P(0, self.ymax)
    local ink8 = fade(ink, 0.8)
    Draw.Line(x0, self.oy, Math.Lerp(x0, x1, ex), self.oy, w, ink8)
    Draw.Line(self.ox, y0, self.ox, Math.Lerp(y0, y1, ey), w, ink8)
    local hx, hy = 36 * zoom, 20 * zoom
    if ex > 0.98 then
        Draw.Line(x1, self.oy, x1 - hx, self.oy + hy, w, ink8)
        Draw.Line(x1, self.oy, x1 - hx, self.oy - hy, w, ink8)
    end
    if ey > 0.98 then
        Draw.Line(self.ox, y1, self.ox - hy, y1 - hx, w, ink8)
        Draw.Line(self.ox, y1, self.ox + hy, y1 - hx, w, ink8)
    end
    if ta <= 0 then
        return
    end
    local muted = self.tickColor
    local size = self.fontSize
    for _, v in ipairs(self.xticks) do
        local px = self:P(v, 0)
        Draw.Line(px, self.oy - 9, px, self.oy + 9, w * 0.7, fade(ink, ta * 0.7))
        Draw.Label(tostring(v), px, self.oy - 40, size, fade(muted, ta), "center")
    end
    for _, v in ipairs(self.yticks) do
        local _, py = self:P(0, v)
        Draw.Line(self.ox - 9, py, self.ox + 9, py, w * 0.7, fade(ink, ta * 0.7))
        Draw.Label(tostring(v), self.ox - 36, py, size, fade(muted, ta), "center")
    end
    if self.labels.x then
        Draw.Label(self.labels.x, x1 + 32, self.oy, size * 1.3, fade(muted, ta), "center")
    end
    if self.labels.y then
        Draw.Label(self.labels.y, self.ox, y1 + 38, size * 1.3, fade(muted, ta), "center")
    end
end

local function sample(axes, fn, from, to, samples, xr)
    local points = {}
    local span = to - from
    local steps = samples - 1
    local k = math.floor((xr - from) / span * steps + 1e-9)
    for i = 0, k do
        local x = from + span * i / steps
        local px, py = axes:P(x, fn(x))
        points[#points + 1] = px
        points[#points + 1] = py
    end
    if xr - (from + span * k / steps) > 1e-6 then
        local px, py = axes:P(xr, fn(xr))
        points[#points + 1] = px
        points[#points + 1] = py
    end
    return points
end

Plot = {}
Plot.__index = Plot

function Axes:Plot(fn, from, to, o)
    o = o or {}
    return setmetatable({
        axes = self, fn = fn, from = from, to = to,
        samples = o.samples or 361, color = o.color or "cyan", width = o.width or 7,
    }, Plot)
end

function Plot:Points(xr)
    xr = Math.Clamp(xr or self.to, self.from, self.to)
    if xr >= self.to - 1e-9 then
        self.full = self.full or sample(self.axes, self.fn, self.from, self.to, self.samples, self.to)
        return self.full
    end
    return sample(self.axes, self.fn, self.from, self.to, self.samples, xr)
end

function Plot:Value(x)
    return self.fn(x)
end

function Plot:At(x)
    return self.axes:P(x, self.fn(x))
end

function Plot:Draw(xr, zoom)
    zoom = zoom or 1
    local points = self:Points(xr)
    if #points < 4 then
        return
    end
    Draw.Polyline(points, self.width * 3.1 * zoom, fade(self.color, 0.1))
    Draw.Polyline(points, self.width * zoom, self.color)
    Draw.Polyline(points, self.width * 0.34 * zoom, fade("ink", 0.55))
end

Area = {}
Area.__index = Area

function Axes:Area(fn, from, to, o)
    o = o or {}
    return setmetatable({axes = self, fn = fn, from = from, to = to, samples = o.samples or 121, color = o.color or "teal"}, Area)
end

function Area:Polygon(xr)
    xr = Math.Clamp(xr or self.to, self.from, self.to)
    local ax = self.axes
    local left, base = ax:P(self.from, 0)
    local right = ax:P(xr, 0)
    local points = {left, base}
    for _, v in ipairs(sample(ax, self.fn, self.from, self.to, self.samples, xr)) do
        points[#points + 1] = v
    end
    points[#points + 1] = right
    points[#points + 1] = base
    return points
end

function Area:Draw(xr, alpha)
    local points = self:Polygon(xr)
    if #points >= 6 then
        Draw.Polygon(points, fade(self.color, alpha or 0.3))
    end
end

Riemann = {}
Riemann.__index = Riemann

function Axes:Riemann(fn, from, to, o)
    o = o or {}
    return setmetatable({
        axes = self, fn = fn, from = from, to = to, mode = o.mode or "left",
        color1 = o.from or "coral", color2 = o.to or "gold", outline = o.outline,
    }, Riemann)
end

local function samplePoint(mode, left, dx)
    if mode == "right" then
        return left + dx
    elseif mode == "mid" then
        return left + dx / 2
    end
    return left
end

function Riemann:Sum(n)
    local dx = (self.to - self.from) / n
    local total = 0
    for i = 0, n - 1 do
        total = total + self.fn(samplePoint(self.mode, self.from + i * dx, dx))
    end
    return total * dx
end

function Riemann:Rects(n, o)
    o = o or {}
    local ax = self.axes
    local dx = (self.to - self.from) / n
    local xlo, xhi = self.from, self.to
    if o.window then
        xlo, xhi = o.window[1], o.window[2]
    end
    local first = math.max(1, math.floor((xlo - self.from) / dx + 1e-6) + 1)
    local last = math.min(n, math.ceil((xhi - self.from) / dx - 1e-6))
    local list = {}
    local _, base = ax:P(0, 0)
    local u = o.u == nil and 1 or o.u
    local splitting = o.from ~= nil and o.from ~= n and u < 1
    local dxFrom = splitting and (self.to - self.from) / o.from or nil
    for i = first, last do
        local left = self.from + (i - 1) * dx
        local h = self.fn(samplePoint(self.mode, left, dx))
        if splitting then
            local parent = math.ceil(i * o.from / n)
            local parentLeft = self.from + (parent - 1) * dxFrom
            h = Math.Lerp(self.fn(samplePoint(self.mode, parentLeft, dxFrom)), h, u)
        end
        if o.grow then
            h = h * o.grow(i)
        end
        local x0, top = ax:P(left, h)
        local x1 = ax:P(left + dx, 0)
        local height = top - base
        if height > 0.5 then
            list[#list + 1] = {
                x0 = x0, x1 = x1, top = top, base = base, h = height, index = i,
                t = (left + dx / 2 - self.from) / (self.to - self.from),
            }
        end
    end
    return list
end

function Riemann:Draw(n, o)
    o = o or {}
    local alpha = o.alpha or 1
    local zoom = o.zoom or 1
    local rects = self:Rects(n, o)
    local outline = o.outline
    if outline == nil then
        outline = self.outline
    end
    if outline == nil then
        outline = n <= 32
    end
    local fill = o.fillAlpha or (n <= 8 and 0.5 or (n <= 32 and 0.58 or 0.82))
    local gap = Math.Remap(math.log(n, 2), 5, 7, 0, 1, true) * (n <= 256 and 0.14 or 0.08)
    for _, r in ipairs(rects) do
        local color = Color.Mix(self.color1, self.color2, r.t)
        local width = (r.x1 - r.x0) * (1 - gap)
        Draw.Rect((r.x0 + r.x1) / 2, r.base + r.h / 2, width, r.h, fade(color, alpha * fill))
        if outline then
            Draw.Polyline({r.x0, r.base, r.x0, r.top, r.x1, r.top, r.x1, r.base}, 3 * zoom, fade(Color.Mix(color, "ink", 0.35), alpha * 0.95))
        end
    end
end

Glyphs = {}
Glyphs.__index = Glyphs

function Glyphs.New(text, size, align, font)
    local list, message = Path.TextGlyphs(text, size, align or "center", font)
    if list == nil then
        error("Glyphs.New: " .. tostring(message), 2)
    end
    local minX, minY, maxX, maxY = math.huge, math.huge, -math.huge, -math.huge
    for _, g in ipairs(list) do
        minX, minY = math.min(minX, g.minX), math.min(minY, g.minY)
        maxX, maxY = math.max(maxX, g.maxX), math.max(maxY, g.maxY)
    end
    return setmetatable({glyphs = list, size = size, bounds = {minX, minY, maxX, maxY}}, Glyphs)
end

function Glyphs:Count()
    return #self.glyphs
end

function Glyphs:Bounds()
    local b = self.bounds
    return b[1], b[2], b[3], b[4]
end

function Glyphs:Size()
    local b = self.bounds
    return b[3] - b[1], b[4] - b[2]
end

function Glyphs:Draw(x, y, progress, o)
    o = o or {}
    progress = Math.Clamp01(progress == nil and 1 or progress)
    if progress <= 0 then
        return
    end
    local b = self.bounds
    if o.middle ~= false then
        x, y = x - (b[1] + b[3]) / 2, y - (b[2] + b[4]) / 2
    end
    local color = o.color or "ink"
    local stroke = o.stroke
    if stroke == nil then
        stroke = color
    end
    local width = o.width or math.max(1.5, self.size * 0.03)
    local n = #self.glyphs
    local spread = o.stagger or 0.6
    local window = 1 - spread * (n - 1) / n
    local fillStart = o.fillStart or 0.6
    for i, g in ipairs(self.glyphs) do
        local offset = (i - 1) / n * spread
        local u = Math.Progress(progress, offset, offset + window)
        if u > 0 then
            local fillAlpha = (o.fill == false) and 0 or Math.Progress(u, fillStart, 1)
            Draw.Path(g.path, x, y, {
                fill = fillAlpha > 0 and Color.WithAlpha(color, fillAlpha) or false,
                stroke = stroke,
                width = width,
                trim = {0, u},
                scale = o.scale,
            })
        end
    end
end

function Axes:Tracer(fn, x, o)
    o = o or {}
    local alpha = o.alpha or 1
    local zoom = o.zoom or 1
    local px, py = self:P(x, fn(x))
    Draw.Dashed(px, self.oy, px, py, 2.6 * zoom, 9 * zoom, 8 * zoom, fade("ink", 0.55 * alpha))
    Draw.Dashed(self.ox, py, px, py, 2.6 * zoom, 9 * zoom, 8 * zoom, fade("ink", 0.4 * alpha))
    local pulse = 0.5 + 0.5 * math.sin((o.time or 0) * 8)
    Draw.Circle(px, py, 26 * zoom, fade("cyan", 0.16 * alpha * (0.6 + 0.4 * pulse)))
    Draw.Circle(px, py, 13 * zoom, fade("ink", alpha))
    Draw.Ring(px, py, 21 * zoom, 3 * zoom, fade("cyan", alpha))
    if o.labels ~= false then
        Draw.Label(string.format("x = %.2f", x), px + 16, self.oy + 34, 32, fade("ink", alpha), "left")
        Draw.Label(string.format("f(x) = %.2f", fn(x)), px + 20, py + 54, 34, fade("cyan", alpha), "center")
    end
end
)LUA";
    }

    namespace
    {
        void RunChunk(lua_State *L, const char *source, const char *name)
        {
            const int top = lua_gettop(L);
            if (luaL_loadbufferx(L, source, std::strlen(source), name, "t") != LUA_OK || lua_pcall(L, 0, 0, 0) != LUA_OK)
                CORE_ERROR("Lua built-in helpers failed to load ({}): {}", name, lua_tostring(L, -1));
            lua_settop(L, top);
        }
    }

    void RunPrelude(lua_State *L)
    {
        RunChunk(L, PreludeSource, "=builtin");
        RunChunk(L, RigSource, "=builtin.camera");
        RunChunk(L, PlotSource, "=builtin.plot");
    }
}
