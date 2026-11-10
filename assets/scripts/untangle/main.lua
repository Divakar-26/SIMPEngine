local Untangle = {}

local INK = {58, 56, 72}
local MUTED = {150, 146, 158}
local CARD = {255, 255, 255}
local SAND = {246, 242, 231}
local SAGE = {142, 166, 107}
local SAGE_INK = {96, 126, 70}
local SAGE_SOFT = {226, 236, 212}
local ROSE = {216, 162, 162}
local ROSE_INK = {204, 92, 104}
local BLUSH = {255, 226, 224}
local SHADOW = {120, 100, 70}

local CX, CY = 0, -40
local RADIUS = 410
local NODE = 19

local function rgba(c, a)
    return c[1], c[2], c[3], math.floor(math.max(0, math.min(1, a or 1)) * 255)
end

local function mix(a, b, t)
    return {a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t, a[3] + (b[3] - a[3]) * t}
end

local function text(str, x, cy, size, c, a, align)
    Draw.Text(str, x, cy + size * 0.52, size, c[1], c[2], c[3], math.floor((a or 1) * 255), align or "center")
end

local function orient(a, b, c)
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x)
end

local function crosses(a, b, c, d)
    local d1, d2 = orient(c, d, a), orient(c, d, b)
    local d3, d4 = orient(a, b, c), orient(a, b, d)
    return ((d1 > 0 and d2 < 0) or (d1 < 0 and d2 > 0)) and ((d3 > 0 and d4 < 0) or (d3 < 0 and d4 > 0))
end

local function shuffle(t)
    for i = #t, 2, -1 do
        local j = math.random(i)
        t[i], t[j] = t[j], t[i]
    end
    return t
end

local SIZES = {{3, 3}, {4, 3}, {4, 4}, {5, 4}, {5, 5}, {6, 5}, {6, 6}}

function Untangle:OnCreate()
    self.time = 0
    self.level = 1
    self.petals = {}
    self:StartLevel()
end

function Untangle:StartLevel()
    Tween.KillAll()
    local size = SIZES[math.min(self.level, #SIZES)]
    local cols, rows = size[1], size[2]
    self.nodes = {}
    self.edges = {}
    self.dragging = nil
    self.won = false
    self.winAlpha = 0
    self.solved = 0
    self.petals = {}
    self.intro = 0

    local spacing = math.min(150, 560 / math.max(cols - 1, rows - 1))
    for r = 0, rows - 1 do
        for c = 0, cols - 1 do
            self.nodes[#self.nodes + 1] = {
                x = CX, y = CY, scale = 0, hover = 0, lift = 0, pop = 1,
                sx = CX + (c - (cols - 1) / 2) * spacing,
                sy = CY + ((rows - 1) / 2 - r) * spacing,
            }
        end
    end

    local function id(c, r) return r * cols + c + 1 end
    local seen = {}
    local function link(a, b)
        local k = math.min(a, b) * 1000 + math.max(a, b)
        if not seen[k] then
            seen[k] = true
            self.edges[#self.edges + 1] = {a = a, b = b, heat = 0, glow = 0}
        end
    end
    local optional = {}
    for r = 0, rows - 1 do
        for c = 0, cols - 1 do
            if c < cols - 1 then
                if math.random() < 0.8 then link(id(c, r), id(c + 1, r)) else optional[#optional + 1] = {id(c, r), id(c + 1, r)} end
            end
            if r < rows - 1 then
                if math.random() < 0.8 then link(id(c, r), id(c, r + 1)) else optional[#optional + 1] = {id(c, r), id(c, r + 1)} end
            end
            if c < cols - 1 and r < rows - 1 and math.random() < 0.65 then
                if math.random() < 0.5 then
                    link(id(c, r), id(c + 1, r + 1))
                else
                    link(id(c + 1, r), id(c, r + 1))
                end
            end
        end
    end
    local degree = {}
    for _, e in ipairs(self.edges) do
        degree[e.a] = (degree[e.a] or 0) + 1
        degree[e.b] = (degree[e.b] or 0) + 1
    end
    for _, pair in ipairs(optional) do
        if (degree[pair[1]] or 0) < 2 or (degree[pair[2]] or 0) < 2 then
            link(pair[1], pair[2])
            degree[pair[1]] = (degree[pair[1]] or 0) + 1
            degree[pair[2]] = (degree[pair[2]] or 0) + 1
        end
    end

    local targets
    for _ = 1, 30 do
        local order = {}
        for i = 1, #self.nodes do
            order[i] = i
        end
        shuffle(order)
        targets = {}
        for k, i in ipairs(order) do
            local a = (k - 1) / #order * math.pi * 2 + 0.3
            targets[i] = {x = CX + math.cos(a) * 300, y = CY + math.sin(a) * 300}
        end
        for i, n in ipairs(self.nodes) do
            n.x, n.y = targets[i].x, targets[i].y
        end
        if self:CountCrossings() >= #self.edges / 2 then
            break
        end
    end

    for i, n in ipairs(self.nodes) do
        local tx, ty = targets[i].x, targets[i].y
        n.x, n.y = CX, CY
        Tween.Value(0, 1, 0.75, "easeOutBack", function(v)
            n.x = CX + (tx - CX) * v
            n.y = CY + (ty - CY) * v
        end, {delay = 0.25 + i * 0.03})
        Tween.Value(0, 1, 0.5, "easeOutBack", function(v) n.scale = v end, {delay = 0.25 + i * 0.03})
    end
    Tween.Value(0, 1, 0.6, "easeOutCubic", function(v) self.intro = v end)
end

function Untangle:CountCrossings()
    local count = 0
    for _, e in ipairs(self.edges) do
        e.crossing = false
    end
    for i = 1, #self.edges do
        local e = self.edges[i]
        for j = i + 1, #self.edges do
            local f = self.edges[j]
            if e.a ~= f.a and e.a ~= f.b and e.b ~= f.a and e.b ~= f.b then
                if crosses(self.nodes[e.a], self.nodes[e.b], self.nodes[f.a], self.nodes[f.b]) then
                    e.crossing = true
                    f.crossing = true
                    count = count + 1
                end
            end
        end
    end
    return count
end

function Untangle:ShowMe()
    if self.won then
        return
    end
    for i, n in ipairs(self.nodes) do
        local fx, fy = n.x, n.y
        Tween.Value(0, 1, 0.9, "easeInOutCubic", function(v)
            n.x = fx + (n.sx - fx) * v
            n.y = fy + (n.sy - fy) * v
        end, {delay = i * 0.025})
    end
end

function Untangle:Win()
    self.won = true
    for k, e in ipairs(self.edges) do
        Tween.Value(0, 1, 0.5, "easeOutCubic", function(v) e.glow = v end, {delay = k * 0.02})
    end
    for i, n in ipairs(self.nodes) do
        Tween.Value(1.45, 1, 0.5, "easeOutBack", function(v) n.pop = v end, {delay = 0.15 + i * 0.03})
    end
    Tween.Value(0, 1, 0.6, "easeOutCubic", function(v) self.winAlpha = v end, {delay = 0.6})
    for _ = 1, 60 do
        self:AddPetal(true)
    end
end

function Untangle:AddPetal(burst)
    local colors = {ROSE, BLUSH, SAGE, SAGE_SOFT, {240, 200, 170}}
    self.petals[#self.petals + 1] = {
        x = (math.random() - 0.5) * 1800,
        y = burst and 560 + math.random() * 500 or 580,
        vx = (math.random() - 0.5) * 60,
        vy = -(80 + math.random() * 110),
        rot = math.random() * 360,
        vr = (math.random() - 0.5) * 220,
        size = 7 + math.random() * 8,
        sway = math.random() * 6.28,
        color = colors[math.random(#colors)],
    }
end

function Untangle:Buttons()
    return {
        {kind = "next", x = -780, y = 300, w = 200, h = 64, label = self.won and "Next level" or "Shuffle"},
        {kind = "show", x = -780, y = 220, w = 200, h = 64, label = "Show me"},
    }
end

function Untangle:OnUpdate(dt)
    self.time = self.time + dt
    local mx, my = self.entity:GetMouseWorld()
    local inView = Input.IsMouseInViewport()

    self.hoverNode = nil
    self.hoverButton = nil
    if inView then
        local best = 38 * 38
        for i, n in ipairs(self.nodes) do
            local d = (n.x - mx) ^ 2 + (n.y - my) ^ 2
            if d < best then
                best = d
                self.hoverNode = i
            end
        end
        for _, b in ipairs(self:Buttons()) do
            if math.abs(mx - b.x) <= b.w / 2 and math.abs(my - b.y) <= b.h / 2 then
                self.hoverButton = b
            end
        end
    end

    if inView and Input.IsMouseJustPressed(Key.MOUSE_LEFT) then
        if self.hoverButton then
            if self.hoverButton.kind == "show" then
                self:ShowMe()
            elseif self.won then
                self.level = self.level + 1
                self:StartLevel()
            else
                self:StartLevel()
            end
        elseif self.hoverNode and not self.won then
            local n = self.nodes[self.hoverNode]
            self.dragging = self.hoverNode
            self.grabX, self.grabY = n.x - mx, n.y - my
        end
    end

    if self.dragging then
        local n = self.nodes[self.dragging]
        if Input.IsMouseDown(Key.MOUSE_LEFT) then
            local x, y = mx + self.grabX, my + self.grabY
            local dx, dy = x - CX, y - CY
            local d = math.sqrt(dx * dx + dy * dy)
            local limit = RADIUS - NODE - 14
            if d > limit then
                x, y = CX + dx / d * limit, CY + dy / d * limit
            end
            n.x, n.y = x, y
        else
            self.dragging = nil
            Tween.Value(1.3, 1, 0.4, "easeOutBack", function(v) n.pop = v end)
        end
    end

    if Input.IsKeyJustPressed(Key.SPACE) and self.won then
        self.level = self.level + 1
        self:StartLevel()
    end

    self.crossings = self:CountCrossings()
    for _, e in ipairs(self.edges) do
        local target = e.crossing and 1 or 0
        e.heat = e.heat + (target - e.heat) * math.min(1, dt * 12)
    end
    for i, n in ipairs(self.nodes) do
        local hover = (i == self.hoverNode and not self.won) and 1 or 0
        n.hover = n.hover + (hover - n.hover) * math.min(1, dt * 14)
        local lift = (i == self.dragging) and 1 or 0
        n.lift = n.lift + (lift - n.lift) * math.min(1, dt * 16)
    end

    if not self.won and not self.dragging and self.intro >= 1 and self.crossings == 0 then
        self:Win()
    end

    for k = #self.petals, 1, -1 do
        local p = self.petals[k]
        p.sway = p.sway + dt * 2
        p.x = p.x + (p.vx + math.sin(p.sway) * 40) * dt
        p.y = p.y + p.vy * dt
        p.rot = p.rot + p.vr * dt
        if p.y < -620 then
            table.remove(self.petals, k)
        end
    end
    if self.won and #self.petals < 40 and math.random() < dt * 14 then
        self:AddPetal(false)
    end
end

function Untangle:DrawTable()
    Draw.SetZ(-1)
    Draw.Circle(CX, CY - 12, RADIUS + 6, rgba(SHADOW, 0.06 * self.intro))
    Draw.Circle(CX, CY - 4, RADIUS + 1, rgba(SHADOW, 0.07 * self.intro))
    Draw.SetZ(-0.9)
    Draw.Circle(CX, CY, RADIUS, rgba(CARD, self.intro))
    Draw.Ring(CX, CY, RADIUS - 26, 1.5, rgba(SAND, self.intro))
end

function Untangle:DrawEdges()
    Draw.SetZ(0)
    for _, e in ipairs(self.edges) do
        local a, b = self.nodes[e.a], self.nodes[e.b]
        local color = mix(INK, ROSE_INK, e.heat)
        local alpha = 0.78 + 0.22 * e.heat
        local width = 6 + e.heat * 1.5
        if e.glow > 0 then
            color = mix(color, SAGE, e.glow)
            alpha = 1
            width = 6 + e.glow * 2
        end
        local visible = math.min(a.scale, b.scale)
        Draw.Line(a.x, a.y, b.x, b.y, width, color[1], color[2], color[3], math.floor(alpha * visible * 255))
    end
end

function Untangle:DrawNodes()
    for i, n in ipairs(self.nodes) do
        local s = n.scale * n.pop * (1 + 0.14 * n.hover + 0.18 * n.lift)
        if s > 0.01 then
            local r = NODE * s
            Draw.SetZ(1 + n.lift)
            Draw.Circle(n.x + 3 * n.lift, n.y - 6 - 8 * n.lift, r + 2, rgba(SHADOW, 0.12 + 0.12 * n.lift))
            Draw.SetZ(1.01 + n.lift)
            local ring = self.won and SAGE_INK or mix(INK, SAGE_INK, math.max(n.hover, n.lift))
            Draw.Circle(n.x, n.y, r, rgba(ring, 1))
            Draw.SetZ(1.02 + n.lift)
            Draw.Circle(n.x, n.y, r - 5 * s, rgba(self.won and SAGE_SOFT or CARD, 1))
            if n.lift > 0.05 then
                Draw.Circle(n.x, n.y, (r - 5 * s) * 0.42, rgba(SAGE, n.lift))
            end
        end
    end
end

function Untangle:DrawHud()
    Draw.SetZ(3)
    local left = -880
    text("Untangle", left, 455, 64, INK, 1, "left")
    text("Drag the dots until no lines cross.", left + 4, 404, 24, MUTED, 1, "left")

    text("Level " .. self.level, 880, 455, 40, INK, 1, "right")
    local crossings = self.crossings or 0
    local label = crossings == 1 and "1 crossing" or (crossings .. " crossings")
    text(self.won and "untangled" or label, 880, 408, 26, crossings == 0 and SAGE_INK or ROSE_INK, 1, "right")

    for _, b in ipairs(self:Buttons()) do
        local primary = b.kind == "next" and self.won
        local fill = primary and SAGE or CARD
        if self.hoverButton == b then
            fill = mix(fill, primary and INK or SAGE, 0.14)
        end
        Draw.SetZ(2.9)
        Draw.RoundRect(b.x, b.y - 5, b.w + 4, b.h + 4, 18, rgba(SHADOW, 0.08))
        Draw.SetZ(3)
        Draw.RoundRect(b.x, b.y, b.w, b.h, 16, rgba(fill, 1))
        Draw.SetZ(3.1)
        text(b.label, b.x, b.y, 24, primary and CARD or INK, 1)
    end
end

function Untangle:DrawWin()
    local a = self.winAlpha
    if a <= 0.01 then
        return
    end
    Draw.SetZ(5)
    local rise = (1 - a) * 24
    text("Untangled", CX, CY + RADIUS + 40 - rise, 44, SAGE_INK, a)
    text("space or Next level", CX, -500 + rise, 22, MUTED, a)
end

function Untangle:DrawPetals()
    Draw.SetZ(8)
    for _, p in ipairs(self.petals) do
        Draw.Rect(p.x, p.y, p.size, p.size * 1.7, p.color[1], p.color[2], p.color[3], 230, p.rot)
    end
end

function Untangle:OnRender()
    self:DrawTable()
    self:DrawEdges()
    self:DrawNodes()
    self:DrawHud()
    self:DrawWin()
    self:DrawPetals()
end

return Untangle
