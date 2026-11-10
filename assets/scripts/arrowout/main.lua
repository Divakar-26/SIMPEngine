local Game = {}

Game.size = 19
Game.cell = 37
Game.seed = 11
Game.maxHearts = 8
Game.lineWidth = 6

local DIRS = {
    {dc = 1, dr = 0, angle = 0},
    {dc = 0, dr = -1, angle = 90},
    {dc = -1, dr = 0, angle = 180},
    {dc = 0, dr = 1, angle = 270},
}

local INK = {58, 52, 66}
local ACCENT = {214, 88, 122}
local RED = {226, 70, 90}
local DOT = {214, 200, 196}
local CONFETTI = {{214, 88, 122}, {255, 200, 205}, {142, 166, 107}, {244, 196, 120}, {216, 162, 162}}

local function key(c, r)
    return r * 100 + c
end

function Game:OnCreate()
    self.camera = Scene.Find("Camera")
    self:Build()
end

function Game:Spawn(name, x, y, parent)
    local e = Scene.Spawn(name, x or 0, y or 0, parent)
    self.spawned[#self.spawned + 1] = e
    return e
end

function Game:CellToWorld(c, r)
    local half = (self.size + 1) / 2
    return (c - half) * self.cell, (half - r) * self.cell
end

function Game:WorldToCell(x, y)
    local half = (self.size + 1) / 2
    local c = math.floor(x / self.cell + half + 0.5)
    local r = math.floor(half - y / self.cell + 0.5)
    if not self:InGrid(c, r) then
        return nil
    end
    return c, r
end

function Game:InGrid(c, r)
    return c >= 1 and c <= self.size and r >= 1 and r <= self.size
end

function Game:InMask(c, r)
    if not self:InGrid(c, r) then
        return false
    end
    local half = (self.size + 1) / 2
    local x = (c - half) / (self.size / 2) * 1.22
    local y = (half - r) / (self.size / 2) * 1.22 + 0.16
    local a = x * x + y * y - 1
    return a * a * a - x * x * y * y * y <= 0
end

function Game:Generate()
    math.randomseed(self.seed)
    local occ = {}
    local skip = {}
    local arrows = {}

    local function rayLength(c, r, dir, own)
        local x, y = c + dir.dc, r + dir.dr
        local n = 0
        while self:InGrid(x, y) do
            local k = key(x, y)
            if occ[k] or own[k] then
                return nil
            end
            if self:InMask(x, y) then
                n = n + 1
            end
            x, y = x + dir.dc, y + dir.dr
        end
        return n
    end

    local function walk(c, r, length)
        local cells = {{c, r}}
        local used = {[key(c, r)] = true}
        local dir = DIRS[math.random(4)]
        for _ = 2, length do
            local last = cells[#cells]
            local options = {}
            for _, d in ipairs(DIRS) do
                local nx, ny = last[1] + d.dc, last[2] + d.dr
                if self:InMask(nx, ny) and not occ[key(nx, ny)] and not used[key(nx, ny)] then
                    options[#options + 1] = d
                    if d == dir then
                        options[#options + 1] = d
                        options[#options + 1] = d
                    end
                end
            end
            if #options == 0 then
                break
            end
            dir = options[math.random(#options)]
            local nx, ny = last[1] + dir.dc, last[2] + dir.dr
            cells[#cells + 1] = {nx, ny}
            used[key(nx, ny)] = true
        end
        if #cells < 2 then
            return nil
        end
        return cells, dir, used
    end

    local failures = 0
    while failures < 400 do
        local empty = {}
        for r = 1, self.size do
            for c = 1, self.size do
                if self:InMask(c, r) and not occ[key(c, r)] and not skip[key(c, r)] then
                    empty[#empty + 1] = {c, r}
                end
            end
        end
        if #empty == 0 then
            break
        end
        local half = (self.size + 1) / 2
        table.sort(empty, function(a, b)
            local da = (a[1] - half) ^ 2 + (a[2] - half) ^ 2
            local db = (b[1] - half) ^ 2 + (b[2] - half) ^ 2
            return da < db
        end)
        local best, bestScore = nil, -1
        for _ = 1, 24 do
            local start = empty[math.random(math.min(#empty, 8))]
            local cells, dir, used = walk(start[1], start[2], math.random(3, 12))
            if cells then
                local head = cells[#cells]
                local ray = rayLength(head[1], head[2], dir, used)
                if ray then
                    local score = #cells * 2 + ray * 3 + math.random() * 6
                    if score > bestScore then
                        best, bestScore = {cells = cells, dir = dir}, score
                    end
                end
            end
        end
        if best then
            arrows[#arrows + 1] = best
            for _, cell in ipairs(best.cells) do
                occ[key(cell[1], cell[2])] = true
            end
        else
            failures = failures + 1
            local stuck = empty[1]
            skip[key(stuck[1], stuck[2])] = true
        end
    end
    return arrows
end

function Game:BodyPath(points)
    local path = Path.New():MoveTo(points[1][1], points[1][2])
    local radius = self.cell * 0.4
    for i = 2, #points - 1 do
        local a, b, c = points[i - 1], points[i], points[i + 1]
        local ix, iy = (b[1] - a[1]) / self.cell, (b[2] - a[2]) / self.cell
        local ox, oy = (c[1] - b[1]) / self.cell, (c[2] - b[2]) / self.cell
        if ix == ox and iy == oy then
            path:LineTo(b[1], b[2])
        else
            path:LineTo(b[1] - ix * radius, b[2] - iy * radius)
            path:QuadTo(b[1], b[2], b[1] + ox * radius, b[2] + oy * radius)
        end
    end
    path:LineTo(points[#points][1], points[#points][2])
    return path
end

function Game:SetInk(arrow, color, width, seconds)
    Tween.To(arrow.line, {strokeColor = color, strokeWidth = width}, seconds or 0.15, "easeOutCubic")
    Tween.To(arrow.head, {fillColor = color, strokeColor = color}, seconds or 0.15, "easeOutCubic")
end

function Game:MakeArrow(data)
    local points = {}
    for _, cell in ipairs(data.cells) do
        local x, y = self:CellToWorld(cell[1], cell[2])
        points[#points + 1] = {x, y}
    end
    local bodyLength = self:BodyPath(points):GetLength()
    local hx, hy = points[#points][1], points[#points][2]
    local vx, vy = data.dir.dc, -data.dir.dr
    local exit = (self.size + 6) * self.cell + bodyLength
    local total = bodyLength + exit

    local full = self:BodyPath(points)
    full:LineTo(hx + vx * exit, hy + vy * exit)

    local line = self:Spawn("Arrow", 0, 0)
    line:SetZIndex(1)
    local lv = line:Add("Vector")
    lv:SetPath(full)
    lv:SetFillEnabled(false)
    lv:SetStroke(INK[1], INK[2], INK[3])
    lv:SetStrokeWidth(self.lineWidth)
    lv:SetCap("round")
    lv:SetJoin("round")
    lv:SetTrim(0, 0)

    local head = self:Spawn("ArrowHead", hx, hy)
    head:SetZIndex(2)
    head:SetRotation(data.dir.angle)
    head:SetScale(0, 0)
    local hv = head:Add("Vector")
    hv:SetPath(Path.Points({-4, -10, 12, 0, -4, 10}, true))
    hv:SetFill(INK[1], INK[2], INK[3])
    hv:SetStroke(INK[1], INK[2], INK[3])
    hv:SetStrokeWidth(4)
    hv:SetJoin("round")

    return {
        line = line, head = head, cells = data.cells, dir = data.dir,
        hx = hx, hy = hy, body = bodyLength / total, exit = exit, total = total,
    }
end

function Game:Build()
    self.spawned = {}
    self.occ = {}
    self.arrows = {}
    self.hearts = self.maxHearts
    self.state = "intro"
    self.hovered = nil
    self.canRestart = false

    local title = self:Spawn("Title", 0, 420)
    local tt = title:Add("Text")
    tt:SetText("Arrow Out")
    tt:SetFontSize(40)
    tt:SetAlign(1)
    tt:SetColor(INK[1], INK[2], INK[3])
    Tween.From(title, {y = "+=50", alpha = 0}, 0.6, "easeOutBack")

    self.counter = self:Spawn("Counter", 300, 420)
    self.counterText = self.counter:Add("Text")
    self.counterText:SetFontSize(28)
    self.counterText:SetAlign(1)
    self.counterText:SetColor(INK[1], INK[2], INK[3])
    Tween.From(self.counter, {alpha = 0}, 0.5, "easeOut", {delay = 0.2})

    self.heartIcons = {}
    for i = 1, self.maxHearts do
        local h = self:Spawn("Heart", (i - (self.maxHearts + 1) / 2) * 40, 372)
        h:SetZIndex(5)
        local hv = h:Add("Vector")
        hv:SetHeart(30)
        hv:SetFill(RED[1], RED[2], RED[3])
        h:SetScale(0, 0)
        Tween.To(h, {scale = {1, 1}}, 0.6, "elasticOut", {delay = 0.3 + i * 0.1})
        self.heartIcons[i] = h
    end

    local hint = self:Spawn("Hint", 0, -410)
    local ht = hint:Add("Text")
    ht:SetText("Tap an arrow. It slides along its line, if the way out is clear.")
    ht:SetFontSize(20)
    ht:SetAlign(1)
    ht:SetColor(120, 128, 150)
    Tween.From(hint, {alpha = 0}, 0.6, "easeOut", {delay = 0.6})

    local data = self:Generate()
    for i, d in ipairs(data) do
        local arrow = self:MakeArrow(d)
        arrow.id = i
        self.arrows[i] = arrow
        for _, cell in ipairs(arrow.cells) do
            self.occ[key(cell[1], cell[2])] = arrow
        end
    end
    self.total = #self.arrows
    self.cleared = 0
    self:SetCounter(false)

    local order = {}
    for _, arrow in ipairs(self.arrows) do
        order[#order + 1] = arrow
    end
    table.sort(order, function(a, b)
        return a.hx * a.hx + a.hy * a.hy < b.hx * b.hx + b.hy * b.hy
    end)
    local last = 0
    for i, arrow in ipairs(order) do
        local delay = 0.3 + i * 0.022
        Tween.To(arrow.line, {trimEnd = arrow.body}, 0.45, "easeInOutCubic", {delay = delay})
        Tween.To(arrow.head, {scale = {1, 1}}, 0.4, "easeOutBack", {delay = delay + 0.35})
        last = delay + 0.75
    end
    Tween.Sequence({last, function() self.state = "play" end})
end

function Game:SetCounter(pop)
    self.counterText:SetText(self.cleared .. " / " .. self.total)
    if pop then
        Tween.FromTo(self.counter, {scale = {1.3, 1.3}}, {scale = {1, 1}}, 0.35, "easeOutBack")
    end
end

function Game:SetHovered(arrow)
    if arrow == self.hovered then
        return
    end
    if self.hovered and not self.hovered.gone then
        self:SetInk(self.hovered, INK, self.lineWidth)
    end
    self.hovered = arrow
    if arrow then
        self:SetInk(arrow, ACCENT, self.lineWidth + 3)
    end
end

function Game:FindBlocker(arrow)
    local head = arrow.cells[#arrow.cells]
    local x, y = head[1] + arrow.dir.dc, head[2] + arrow.dir.dr
    local steps = 1
    while self:InGrid(x, y) do
        local other = self.occ[key(x, y)]
        if other and other ~= arrow then
            return other, steps
        end
        x, y = x + arrow.dir.dc, y + arrow.dir.dr
        steps = steps + 1
    end
    return nil
end

function Game:OnUpdate(dt)
    if self.state == "won" or self.state == "lost" then
        if self.canRestart and Input.IsMouseJustPressed(Key.MOUSE_LEFT) and Input.IsMouseInViewport() then
            self:Restart()
        end
        return
    end
    if self.state ~= "play" or not Input.IsMouseInViewport() then
        self:SetHovered(nil)
        return
    end

    local mx, my = self.entity:GetMouseWorld()
    local c, r = self:WorldToCell(mx, my)
    local arrow = c and self.occ[key(c, r)] or nil
    if Time.Now() >= (self.flashUntil or 0) then
        self:SetHovered(arrow)
    end

    if arrow and Input.IsMouseJustPressed(Key.MOUSE_LEFT) then
        local blocker, steps = self:FindBlocker(arrow)
        if blocker then
            self:Blocked(arrow, blocker, steps)
        else
            self:Launch(arrow)
        end
    end
end

function Game:Launch(arrow)
    arrow.gone = true
    self.hovered = nil
    for _, cell in ipairs(arrow.cells) do
        self.occ[key(cell[1], cell[2])] = nil
    end
    self.cleared = self.cleared + 1
    self:SetCounter(true)

    local line, head = arrow.line, arrow.head
    Tween.Kill(line)
    Tween.Kill(head)
    line:SetZIndex(3)
    head:SetZIndex(4)
    local vx, vy = arrow.dir.dc, -arrow.dir.dr
    local duration = 0.75 + arrow.body * 0.3

    Tween.To(line, {strokeColor = ACCENT, strokeWidth = self.lineWidth}, 0.12, "easeOut")
    Tween.To(head, {fillColor = ACCENT, strokeColor = ACCENT}, 0.12, "easeOut")
    Tween.To(line, {trimOffset = arrow.exit / arrow.total}, duration, "easeInQuad")
    Tween.Along(head, Path.Line(arrow.hx, arrow.hy, arrow.hx + vx * arrow.exit, arrow.hy + vy * arrow.exit), duration, "easeInQuad", {
        onComplete = function()
            line:Destroy()
            head:Destroy()
        end,
    })
    Tween.FromTo(head, {scale = {1.5, 1.5}}, {scale = {1, 1}}, 0.3, "easeOutBack")

    local ring = self:Spawn("Ring", arrow.hx, arrow.hy)
    ring:SetZIndex(2)
    local rv = ring:Add("Vector")
    rv:SetCircle(12)
    rv:SetFillEnabled(false)
    rv:SetStroke(ACCENT[1], ACCENT[2], ACCENT[3])
    rv:SetStrokeWidth(4)
    Tween.To(ring, {scale = {3, 3}, alpha = 0, strokeWidth = 1}, 0.45, "easeOutCubic", {
        onComplete = function() ring:Destroy() end,
    })

    if self.cleared == self.total then
        self.state = "won"
        Tween.Sequence({0.6, function() self:Win() end})
    end
end

function Game:Blocked(arrow, blocker, steps)
    local vx, vy = arrow.dir.dc, -arrow.dir.dr
    local bump = math.max((steps - 0.5) * self.cell - 6, self.cell * 0.35)
    Tween.Kill(arrow.line)
    Tween.Kill(arrow.head)
    arrow.head:SetPosition(arrow.hx, arrow.hy)
    arrow.head:SetScale(1, 1)
    local shake = Curves.Sine(0.5)
    Tween.FromTo(arrow.line, {trimOffset = 0}, {trimOffset = bump / arrow.total}, 0.4, shake)
    Tween.Along(arrow.head, Path.Line(arrow.hx, arrow.hy, arrow.hx + vx * bump, arrow.hy + vy * bump), 0.4, shake)
    Tween.FromTo(arrow.line, {strokeColor = RED}, {strokeColor = INK}, 0.8, "easeInCubic")
    Tween.FromTo(arrow.head, {fillColor = RED, strokeColor = RED}, {fillColor = INK, strokeColor = INK}, 0.8, "easeInCubic")
    Tween.FromTo(blocker.line, {strokeColor = RED, strokeWidth = self.lineWidth + 4}, {strokeColor = INK, strokeWidth = self.lineWidth}, 0.8, "easeInCubic")
    Tween.FromTo(blocker.head, {fillColor = RED, strokeColor = RED, scale = {1.4, 1.4}}, {fillColor = INK, strokeColor = INK, scale = {1, 1}}, 0.6, "easeOutBack")
    self.hovered = nil
    self.flashUntil = Time.Now() + 0.8
    if self.camera then
        self.camera:GetCamera():Shake(0.25)
    end

    local heart = self.heartIcons[self.hearts]
    if heart then
        self.hearts = self.hearts - 1
        heart:GetVector():MorphTo(Path.Circle(6), 0.35, "easeInCubic")
        Tween.To(heart, {y = "+=30", alpha = 0, rotation = -25}, 0.45, "easeInBack", {delay = 0.1})
    end
    if self.hearts <= 0 then
        self.state = "lost"
        Tween.Sequence({0.6, function() self:Lose() end})
    end
end

function Game:Banner(text, color)
    local banner = self:Spawn("Banner", 0, 30)
    banner:SetZIndex(20)
    local bt = banner:Add("Text")
    bt:SetText(text)
    bt:SetFontSize(88)
    bt:SetAlign(1)
    bt:SetColor(color[1], color[2], color[3])
    banner:SetScale(0, 0)
    Tween.To(banner, {scale = {1, 1}}, 0.9, "elasticOut")

    local again = self:Spawn("Again", 0, -50)
    again:SetZIndex(20)
    local at = again:Add("Text")
    at:SetText("Click anywhere to play again")
    at:SetFontSize(24)
    at:SetAlign(1)
    at:SetColor(INK[1], INK[2], INK[3], 0)
    Tween.Sequence({
        0.7,
        Tween.To(again, {alpha = 255}, 0.4, "easeOut"),
        function()
            self.canRestart = true
            Tween.To(again, {alpha = 80}, 0.8, "easeInOutSine", {loop = true, yoyo = true})
        end,
    })
end

function Game:Win()
    local ring = self:Spawn("WinRing", 0, 0)
    ring:SetZIndex(15)
    local rv = ring:Add("Vector")
    rv:SetPath(Path.Heart(self.size * self.cell * 1.08))
    rv:SetFillEnabled(false)
    rv:SetStroke(ACCENT[1], ACCENT[2], ACCENT[3])
    rv:SetStrokeWidth(7)
    rv:SetTrim(0, 0)
    ring:SetPosition(0, -14)
    Tween.To(ring, {trimEnd = 1}, 1.2, "easeInOutCubic")
    Tween.To(ring, {scale = {1.03, 1.03}}, 0.7, "easeInOutSine", {delay = 1.2, loop = true, yoyo = true})

    for i = 1, 80 do
        local piece = self:Spawn("Confetti", 0, 0)
        piece:SetZIndex(25)
        local pv = piece:Add("Vector")
        local color = CONFETTI[(i - 1) % #CONFETTI + 1]
        if i % 3 == 0 then
            pv:SetCircle(6)
        else
            pv:SetRect(9, 16, 3)
        end
        pv:SetFill(color[1], color[2], color[3])
        local side = math.random() * 2 - 1
        local path = Path.New():MoveTo(0, 0):CubicTo(side * 260, 450 + math.random() * 250, side * 700, 280, side * 900 + (math.random() - 0.5) * 300, -650)
        local duration = 1.6 + math.random() * 1.0
        Tween.Along(piece, path, duration, "easeOutQuad", {onComplete = function() piece:Destroy() end})
        Tween.To(piece, {rotation = (math.random() < 0.5 and -1 or 1) * (360 + math.random() * 720)}, duration, "linear")
        Tween.To(piece, {alpha = 0}, 0.5, "easeIn", {delay = duration - 0.5})
    end

    self:Banner("Cleared!", ACCENT)
end

function Game:Lose()
    local i = 0
    for _, arrow in ipairs(self.arrows) do
        if not arrow.gone then
            i = i + 1
            arrow.gone = true
            Tween.Kill(arrow.line)
            Tween.Kill(arrow.head)
            Tween.To(arrow.line, {trimStart = arrow.body + 0.01}, 0.5, "easeInCubic", {delay = i * 0.015})
            Tween.To(arrow.head, {scale = {0, 0}}, 0.35, "easeInBack", {delay = i * 0.015 + 0.2})
        end
    end
    self.occ = {}
    self:Banner("Out of hearts", RED)
end

function Game:Restart()
    self.canRestart = false
    Tween.KillAll()
    for _, e in ipairs(self.spawned) do
        if e:IsValid() then
            e:Destroy()
        end
    end
    self:Build()
end

function Game:OnRender()
    Draw.SetZ(-1)
    for r = 1, self.size do
        for c = 1, self.size do
            if self:InMask(c, r) then
                local x, y = self:CellToWorld(c, r)
                Draw.Circle(x, y, 2.6, DOT[1], DOT[2], DOT[3], 255)
            end
        end
    end
end

return Game
