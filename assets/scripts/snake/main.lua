local Game = {}

Game.cols = 19
Game.rows = 15
Game.cell = 34
Game.startLength = 4

local UP, DOWN, LEFT, RIGHT = {0, 1}, {0, -1}, {-1, 0}, {1, 0}

local BOARD = {22, 28, 50}
local BOARD_EDGE = {58, 70, 118}
local DOT = {44, 54, 92}
local HEAD = {255, 200, 96}
local TAIL = {96, 224, 176}
local DEAD = {92, 100, 128}
local BERRY = {255, 86, 146}
local TEXT = {226, 232, 255}
local FAINT = {128, 140, 186}

local function lerp(a, b, t)
    return a + (b - a) * t
end

local function mix(a, b, t)
    return lerp(a[1], b[1], t), lerp(a[2], b[2], t), lerp(a[3], b[3], t)
end

local function key(c, r)
    return r * 100 + c
end

function Game:OnCreate()
    self.camera = Scene.Find("Camera")
    self.best = 0
    self.time = 0
    self:Reset()
end

function Game:Reset()
    Tween.KillAll()
    local midC = math.floor(self.cols / 2) - 1
    local midR = math.floor(self.rows / 2) + 1
    self.body = {}
    for i = 1, self.startLength do
        self.body[i] = {midC - i + 1, midR}
    end
    self.prevTail = {midC - self.startLength, midR}
    self.dir = RIGHT
    self.queue = {}
    self.grow = 0
    self.score = 0
    self.clock = 0
    self.state = "ready"
    self.bulges = {}
    self.sparks = {}
    self.rings = {}
    self.deadT = 0
    self.flash = 0
    self.scorePop = 1
    self.cardAlpha = 0
    self.tongue = 0
    self.nextTongue = 1.5
    self:PlaceFood()
    Tween.Value(0, 1, 0.5, "easeOut", function(v) self.cardAlpha = v end)
end

function Game:CellToWorld(c, r)
    return (c - (self.cols + 1) / 2) * self.cell, (r - (self.rows + 1) / 2) * self.cell - 24
end

function Game:StepTime()
    return math.max(0.07, 0.125 - self.score * 0.0018)
end

function Game:PlaceFood()
    local used = {}
    for _, cell in ipairs(self.body) do
        used[key(cell[1], cell[2])] = true
    end
    local free = {}
    for r = 1, self.rows do
        for c = 1, self.cols do
            if not used[key(c, r)] then
                free[#free + 1] = {c, r}
            end
        end
    end
    if #free == 0 then
        self.food = nil
        return
    end
    self.food = free[math.random(#free)]
    self.foodScale = 0
    Tween.Value(0, 1, 0.45, "elasticOut", function(v) self.foodScale = v end)
end

function Game:ReadInput()
    local pressed = nil
    if Input.IsKeyJustPressed(Key.UP) or Input.IsKeyJustPressed(Key.W) then
        pressed = UP
    elseif Input.IsKeyJustPressed(Key.DOWN) or Input.IsKeyJustPressed(Key.S) then
        pressed = DOWN
    elseif Input.IsKeyJustPressed(Key.LEFT) or Input.IsKeyJustPressed(Key.A) then
        pressed = LEFT
    elseif Input.IsKeyJustPressed(Key.RIGHT) or Input.IsKeyJustPressed(Key.D) then
        pressed = RIGHT
    end
    if not pressed then
        return false
    end
    local last = self.queue[#self.queue] or self.dir
    local reverse = pressed[1] == -last[1] and pressed[2] == -last[2]
    if pressed ~= last and not reverse and #self.queue < 3 then
        self.queue[#self.queue + 1] = pressed
    end
    return true
end

function Game:OnUpdate(dt)
    self.time = self.time + dt

    if self.state == "ready" then
        if self:ReadInput() and (#self.queue > 0 or self.dir) then
            self.state = "play"
            self.clock = 0
            Tween.Value(self.cardAlpha, 0, 0.25, "easeIn", function(v) self.cardAlpha = v end)
        end
        return
    end

    if self.state == "paused" then
        if Input.IsKeyJustPressed(Key.P) or Input.IsKeyJustPressed(Key.ESCAPE) or Input.IsKeyJustPressed(Key.SPACE) then
            self.state = "play"
        end
        return
    end

    if self.state == "dead" or self.state == "won" then
        self:UpdateEffects(dt)
        if self.canRestart and (Input.IsKeyJustPressed(Key.SPACE) or Input.IsKeyJustPressed(Key.RETURN)) then
            self:Reset()
        end
        return
    end

    if Input.IsKeyJustPressed(Key.P) or Input.IsKeyJustPressed(Key.ESCAPE) then
        self.state = "paused"
        return
    end

    self:ReadInput()
    self.clock = self.clock + dt
    local step = self:StepTime()
    while self.clock >= step and self.state == "play" do
        self.clock = self.clock - step
        self:Advance()
    end
    self:UpdateEffects(dt)

    self.nextTongue = self.nextTongue - dt
    if self.nextTongue <= 0 then
        self.nextTongue = 2 + math.random() * 3
        Tween.Value(0, 1, 0.35, Curves.Sine(1), function(v) self.tongue = v end)
    end
end

function Game:Advance()
    if #self.queue > 0 then
        self.dir = table.remove(self.queue, 1)
    end
    local head = self.body[1]
    local nc, nr = head[1] + self.dir[1], head[2] + self.dir[2]

    local tail = self.body[#self.body]
    local hitsBody = false
    for i = 1, #self.body do
        local cell = self.body[i]
        local leaving = i == #self.body and self.grow == 0
        if cell[1] == nc and cell[2] == nr and not leaving then
            hitsBody = true
        end
    end
    if nc < 1 or nc > self.cols or nr < 1 or nr > self.rows or hitsBody then
        self:Die()
        return
    end

    table.insert(self.body, 1, {nc, nr})
    if self.grow > 0 then
        self.grow = self.grow - 1
        self.prevTail = {tail[1], tail[2]}
    else
        self.prevTail = table.remove(self.body)
    end

    for _, b in ipairs(self.bulges) do
        b.index = b.index + 1
    end
    for i = #self.bulges, 1, -1 do
        if self.bulges[i].index > #self.body + 1 then
            table.remove(self.bulges, i)
        end
    end

    if self.food and nc == self.food[1] and nr == self.food[2] then
        self:Eat()
    end
end

function Game:Eat()
    local fx, fy = self:CellToWorld(self.food[1], self.food[2])
    self.grow = self.grow + 1
    self.score = self.score + 1
    if self.score > self.best then
        self.best = self.score
    end
    self.bulges[#self.bulges + 1] = {index = 0}
    Tween.Value(1.5, 1, 0.4, "easeOutBack", function(v) self.scorePop = v end)

    for i = 1, 14 do
        local a = (i / 14) * math.pi * 2 + math.random() * 0.4
        local speed = 90 + math.random() * 160
        self.sparks[#self.sparks + 1] = {
            x = fx, y = fy,
            vx = math.cos(a) * speed, vy = math.sin(a) * speed,
            life = 0.45 + math.random() * 0.3, age = 0,
            size = 2 + math.random() * 2.5,
        }
    end
    local ring = {x = fx, y = fy, radius = 8, alpha = 1}
    self.rings[#self.rings + 1] = ring
    Tween.Value(0, 1, 0.5, "easeOutCubic", function(v)
        ring.radius = 8 + v * 38
        ring.alpha = 1 - v
    end)

    self:PlaceFood()
    if not self.food then
        self:Win()
    end
end

function Game:UpdateEffects(dt)
    for i = #self.sparks, 1, -1 do
        local s = self.sparks[i]
        s.age = s.age + dt
        s.x = s.x + s.vx * dt
        s.y = s.y + s.vy * dt
        s.vx = s.vx * (1 - 4 * dt)
        s.vy = s.vy * (1 - 4 * dt)
        if s.age >= s.life then
            table.remove(self.sparks, i)
        end
    end
    for i = #self.rings, 1, -1 do
        if self.rings[i].alpha <= 0.001 then
            table.remove(self.rings, i)
        end
    end
end

function Game:Die()
    self.state = "dead"
    self.queue = {}
    self.canRestart = false
    if self.camera then
        self.camera:GetCamera():Shake(0.45)
    end
    Tween.Value(1, 0, 0.6, "easeOutCubic", function(v) self.flash = v end)
    Tween.Value(0, 1, 0.7, "easeInOutCubic", function(v) self.deadT = v end)
    Tween.Sequence({
        0.5,
        Tween.Value(0, 1, 0.4, "easeOutBack", function(v) self.cardAlpha = v end),
        function() self.canRestart = true end,
    })
end

function Game:Win()
    self.state = "won"
    self.canRestart = false
    Tween.Sequence({
        0.3,
        Tween.Value(0, 1, 0.4, "easeOutBack", function(v) self.cardAlpha = v end),
        function() self.canRestart = true end,
    })
end

function Game:VisualPoints()
    local t = 0
    if self.state == "play" then
        t = math.min(self.clock / self:StepTime(), 1)
    elseif self.state == "paused" then
        t = math.min(self.clock / self:StepTime(), 1)
    elseif self.state == "dead" or self.state == "won" then
        t = 1
    end
    local pts = {}
    local body = self.body
    if #body >= 2 and t < 1 then
        local hx, hy = self:CellToWorld(body[2][1], body[2][2])
        local nx, ny = self:CellToWorld(body[1][1], body[1][2])
        pts[1] = {lerp(hx, nx, t), lerp(hy, ny, t)}
        for i = 2, #body do
            local x, y = self:CellToWorld(body[i][1], body[i][2])
            pts[#pts + 1] = {x, y}
        end
        local px, py = self:CellToWorld(self.prevTail[1], self.prevTail[2])
        local lx, ly = pts[#pts][1], pts[#pts][2]
        local tx, ty = lerp(px, lx, t), lerp(py, ly, t)
        if math.abs(tx - lx) + math.abs(ty - ly) > 0.01 then
            pts[#pts + 1] = {tx, ty}
        end
    else
        for i = 1, #body do
            local x, y = self:CellToWorld(body[i][1], body[i][2])
            pts[#pts + 1] = {x, y}
        end
    end
    return pts, t
end

function Game:DrawBoard()
    local w, h = self.cols * self.cell, self.rows * self.cell
    local cx, cy = self:CellToWorld((self.cols + 1) / 2, (self.rows + 1) / 2)
    Draw.SetZ(-3)
    local edge = BOARD_EDGE
    if self.flash > 0 then
        edge = {lerp(edge[1], 255, self.flash), lerp(edge[2], 90, self.flash), lerp(edge[3], 110, self.flash)}
    end
    Draw.RoundRect(cx, cy, w + 22, h + 22, 20, edge[1], edge[2], edge[3], 255)
    Draw.SetZ(-2.9)
    Draw.RoundRect(cx, cy, w + 14, h + 14, 16, BOARD[1], BOARD[2], BOARD[3], 255)
    Draw.SetZ(-2.8)
    for r = 1, self.rows do
        for c = 1, self.cols do
            local x, y = self:CellToWorld(c, r)
            Draw.Circle(x, y, 2.2, DOT[1], DOT[2], DOT[3], 255)
        end
    end
end

function Game:DrawFood()
    if not self.food then
        return
    end
    local x, y = self:CellToWorld(self.food[1], self.food[2])
    local pulse = 0.5 + 0.5 * math.sin(self.time * 4)
    local s = self.foodScale or 1
    local r = self.cell * 0.3 * s
    Draw.SetZ(0.5)
    Draw.Circle(x, y, r * (1.9 + pulse * 0.3), BERRY[1], BERRY[2], BERRY[3], math.floor(30 + pulse * 25))
    Draw.Ring(x, y, r * (1.45 + pulse * 0.15), 1.5, BERRY[1], BERRY[2], BERRY[3], 110)
    Draw.SetZ(0.6)
    Draw.Circle(x, y, r, BERRY[1], BERRY[2], BERRY[3], 255)
    Draw.SetZ(0.7)
    Draw.Circle(x - r * 0.35, y + r * 0.35, r * 0.28, 255, 220, 236, 230)
    Draw.Line(x + r * 0.1, y + r * 0.9, x + r * 0.45, y + r * 1.5, 2.5, 120, 210, 140, 255)
end

function Game:DrawSnake()
    local pts, t = self:VisualPoints()
    if #pts < 2 then
        return
    end
    local lengths = {0}
    for i = 2, #pts do
        local dx, dy = pts[i][1] - pts[i - 1][1], pts[i][2] - pts[i - 1][2]
        lengths[i] = lengths[i - 1] + math.sqrt(dx * dx + dy * dy)
    end
    local total = lengths[#lengths]
    if total <= 0 then
        return
    end

    local bulgeAt = {}
    for _, b in ipairs(self.bulges) do
        bulgeAt[#bulgeAt + 1] = (b.index - 1 + t) * self.cell
    end

    local base = self.cell * 0.36
    local spacing = 3.5
    local samples = {}
    local seg = 2
    local s = 0
    while s <= total do
        while seg < #pts and lengths[seg] < s do
            seg = seg + 1
        end
        local a, b = pts[seg - 1], pts[seg]
        local len = lengths[seg] - lengths[seg - 1]
        local u = len > 0 and (s - lengths[seg - 1]) / len or 0
        local x, y = lerp(a[1], b[1], u), lerp(a[2], b[2], u)
        local radius = base
        local fromTail = total - s
        if fromTail < self.cell * 1.6 then
            radius = base * (0.45 + 0.55 * fromTail / (self.cell * 1.6))
        end
        for _, at in ipairs(bulgeAt) do
            local d = math.abs(s - at) / (self.cell * 0.8)
            if d < 1 then
                radius = radius * (1 + 0.32 * (1 - d * d))
            end
        end
        samples[#samples + 1] = {x, y, radius, s / total}
        s = s + spacing
    end
    samples[#samples + 1] = {pts[#pts][1], pts[#pts][2], base * 0.45, 1}

    local function colorAt(k)
        local r, g, b = mix(HEAD, TAIL, k)
        if self.deadT > 0 then
            local d = math.max(0, math.min(1, self.deadT * 1.6 - k * 0.6))
            r, g, b = lerp(r, DEAD[1], d), lerp(g, DEAD[2], d), lerp(b, DEAD[3], d)
        end
        return r, g, b
    end

    Draw.SetZ(1)
    for i = #samples, 1, -1 do
        local p = samples[i]
        local r, g, b = colorAt(p[4])
        Draw.Circle(p[1], p[2], p[3] + 3, r * 0.35, g * 0.35, b * 0.4, 255)
    end
    Draw.SetZ(1.1)
    for i = #samples, 1, -1 do
        local p = samples[i]
        local r, g, b = colorAt(p[4])
        Draw.Circle(p[1], p[2], p[3], r, g, b, 255)
    end
    if #pts >= 2 then
        local shine = Path.New()
        local ox, oy = -base * 0.22, base * 0.3
        shine:MoveTo(pts[1][1] + ox, pts[1][2] + oy)
        for i = 2, #pts do
            shine:LineTo(pts[i][1] + ox, pts[i][2] + oy)
        end
        local alpha = math.floor(70 * (1 - self.deadT))
        if alpha > 0 then
            Draw.SetZ(1.2)
            Draw.Path(shine, 0, 0, {fill = false, stroke = {255, 255, 255, alpha}, width = base * 0.38, cap = "round", join = "round", trim = {0, 0.85, 0}})
        end
    end

    local hx, hy = pts[1][1], pts[1][2]
    local dx, dy = pts[1][1] - pts[2][1], pts[1][2] - pts[2][2]
    local dl = math.sqrt(dx * dx + dy * dy)
    if dl < 0.001 then
        dx, dy = self.dir[1], self.dir[2]
    else
        dx, dy = dx / dl, dy / dl
    end
    local px, py = -dy, dx

    if self.tongue > 0.01 and self.state ~= "dead" then
        local reach = base + self.tongue * self.cell * 0.45
        local tx, ty = hx + dx * reach, hy + dy * reach
        Draw.SetZ(0.9)
        Draw.Line(hx, hy, tx, ty, 2.2, 255, 96, 120, 255)
        Draw.Line(tx, ty, tx + (dx + px) * 5, ty + (dy + py) * 5, 2, 255, 96, 120, 255)
        Draw.Line(tx, ty, tx + (dx - px) * 5, ty + (dy - py) * 5, 2, 255, 96, 120, 255)
    end

    local fx, fy = hx + dx * 20, hy + dy * 20
    if self.food then
        fx, fy = self:CellToWorld(self.food[1], self.food[2])
    end
    Draw.SetZ(1.3)
    for side = -1, 1, 2 do
        local ex, ey = hx + dx * base * 0.3 + px * side * base * 0.48, hy + dy * base * 0.3 + py * side * base * 0.48
        if self.state == "dead" then
            Draw.Line(ex - 3, ey - 3, ex + 3, ey + 3, 2, 30, 34, 52, 255)
            Draw.Line(ex - 3, ey + 3, ex + 3, ey - 3, 2, 30, 34, 52, 255)
        else
            Draw.Circle(ex, ey, base * 0.3, 255, 252, 240, 255)
            local lx, ly = fx - ex, fy - ey
            local ll = math.sqrt(lx * lx + ly * ly)
            if ll > 0.001 then
                lx, ly = lx / ll, ly / ll
            end
            Draw.SetZ(1.4)
            Draw.Circle(ex + lx * base * 0.12, ey + ly * base * 0.12, base * 0.16, 26, 24, 44, 255)
            Draw.SetZ(1.3)
        end
    end
end

function Game:DrawEffects()
    Draw.SetZ(2)
    for _, ring in ipairs(self.rings) do
        Draw.Ring(ring.x, ring.y, ring.radius, 3, BERRY[1], BERRY[2], BERRY[3], math.floor(ring.alpha * 255))
    end
    for _, s in ipairs(self.sparks) do
        local k = 1 - s.age / s.life
        Draw.Circle(s.x, s.y, s.size * k + 0.5, 255, lerp(150, 230, k), lerp(170, 120, k), math.floor(255 * k))
    end
end

function Game:DrawHud()
    local mid = (self.rows / 2) * self.cell - 24 + 38
    local left = -(self.cols / 2) * self.cell
    local right = (self.cols / 2) * self.cell
    local size = 28 * self.scorePop
    Draw.SetZ(3)
    Draw.Circle(left + 12, mid, 9, BERRY[1], BERRY[2], BERRY[3], 255)
    Draw.Text(tostring(self.score), left + 30, mid + size * 0.5, size, TEXT[1], TEXT[2], TEXT[3], 255, "left")
    Draw.Text("best " .. self.best, right, mid + 10, 20, FAINT[1], FAINT[2], FAINT[3], 255, "right")
    Draw.Text("GLOWWORM", 0, mid + 11, 22, HEAD[1], HEAD[2], HEAD[3], 255, "center")
end

function Game:DrawCard()
    local a = self.cardAlpha
    if a <= 0.01 then
        if self.state == "paused" then
            a = 1
        else
            return
        end
    end
    local _, cy = self:CellToWorld(1, (self.rows + 1) / 2)
    local alpha = math.floor(math.min(a, 1) * 255)
    Draw.SetZ(4)
    local scale = 0.85 + 0.15 * math.min(a, 1.2)
    if self.state == "ready" then
        Draw.RoundRect(0, cy - 70, 360 * scale, 64 * scale, 14, 14, 18, 34, math.floor(alpha * 0.85))
        Draw.SetZ(4.1)
        Draw.Text("Arrow keys or WASD to start", 0, cy - 59, 20, TEXT[1], TEXT[2], TEXT[3], alpha, "center")
    elseif self.state == "paused" then
        Draw.RoundRect(0, cy, 300, 110, 16, 14, 18, 34, 220)
        Draw.SetZ(4.1)
        Draw.Text("Paused", 0, cy + 38, 40, TEXT[1], TEXT[2], TEXT[3], 255, "center")
        Draw.Text("P to continue", 0, cy - 14, 18, FAINT[1], FAINT[2], FAINT[3], 255, "center")
    else
        local won = self.state == "won"
        Draw.RoundRect(0, cy, 380 * scale, 190 * scale, 20, 14, 18, 34, math.floor(alpha * 0.92))
        Draw.SetZ(4.1)
        local color = won and HEAD or BERRY
        Draw.Text(won and "Board full!" or "Game over", 0, cy + 72, 44, color[1], color[2], color[3], alpha, "center")
        Draw.Text(tostring(self.score), 0, cy + 20, 40, TEXT[1], TEXT[2], TEXT[3], alpha, "center")
        local isBest = self.score > 0 and self.score >= self.best
        Draw.Text(isBest and "new best" or ("best " .. self.best), 0, cy - 30, 18, FAINT[1], FAINT[2], FAINT[3], alpha, "center")
        if self.canRestart then
            local blink = math.floor((0.55 + 0.45 * math.sin(self.time * 4)) * alpha)
            Draw.Text("Space to play again", 0, cy - 58, 18, TEXT[1], TEXT[2], TEXT[3], blink, "center")
        end
    end
end

function Game:OnRender()
    self:DrawBoard()
    self:DrawFood()
    self:DrawSnake()
    self:DrawEffects()
    self:DrawHud()
    self:DrawCard()
end

return Game
