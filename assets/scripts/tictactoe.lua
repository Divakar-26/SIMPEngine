local T = {}

T.cell = 140
local GRID = {70, 78, 104}
local XCOL = {255, 110, 140}
local OCOL = {90, 200, 255}
local WIN = {255, 214, 90}
local TEXT = {225, 230, 245}

local LINES = {
    {{1, 1}, {2, 1}, {3, 1}}, {{1, 2}, {2, 2}, {3, 2}}, {{1, 3}, {2, 3}, {3, 3}},
    {{1, 1}, {1, 2}, {1, 3}}, {{2, 1}, {2, 2}, {2, 3}}, {{3, 1}, {3, 2}, {3, 3}},
    {{1, 1}, {2, 2}, {3, 3}}, {{3, 1}, {2, 2}, {1, 3}},
}

function T:CellCenter(c, r)
    return (c - 2) * self.cell, (2 - r) * self.cell
end

function T:CellAt(x, y)
    local c = math.floor(x / self.cell + 2.5)
    local r = math.floor(2.5 - y / self.cell)
    if c < 1 or c > 3 or r < 1 or r > 3 then
        return nil
    end
    return c, r
end

function T:Spawn(name, x, y, parent)
    local e = Scene.Spawn(name, x, y, parent)
    self.spawned[#self.spawned + 1] = e
    return e
end

function T:Stroke(e, path, color, width)
    local v = e:Add("Vector")
    v:SetPath(path)
    v:SetFillEnabled(false)
    v:SetStroke(color[1], color[2], color[3])
    v:SetStrokeWidth(width)
    v:SetCap("round")
    v:SetTrim(0, 0)
    return v
end

function T:OnCreate()
    self:Build()
end

function T:Build()
    self.spawned = {}
    self.board = {{}, {}, {}}
    self.marks = {}
    self.turn = "X"
    self.moves = 0
    self.state = "intro"

    local half = self.cell * 1.5
    local lines = {
        {-self.cell / 2, half, -self.cell / 2, -half},
        {self.cell / 2, half, self.cell / 2, -half},
        {-half, self.cell / 2, half, self.cell / 2},
        {-half, -self.cell / 2, half, -self.cell / 2},
    }
    for i, l in ipairs(lines) do
        local e = self:Spawn("GridLine", 0, 0)
        self:Stroke(e, Path.Line(l[1], l[2], l[3], l[4]), GRID, 8)
        Tween.To(e, {trimEnd = 1}, 0.5, "easeInOutCubic", {delay = 0.1 + i * 0.12})
    end

    self.status = self:Spawn("Status", 0, 290)
    local text = self.status:Add("Text")
    text:SetFontSize(40)
    text:SetAlign(1)
    self:SetStatus("X goes first", TEXT)

    Tween.Sequence({0.9, function() self.state = "play" end})
end

function T:SetStatus(message, color)
    local text = self.status:GetText()
    text:SetText(message)
    text:SetColor(color[1], color[2], color[3])
    Tween.FromTo(self.status, {scale = {0.7, 0.7}, alpha = 0}, {scale = {1, 1}, alpha = 255}, 0.4, "easeOutBack")
end

function T:PlaceX(c, r)
    local x, y = self:CellCenter(c, r)
    local mark = self:Spawn("X", x, y)
    mark:SetZIndex(2)
    local s = 40
    local a = self:Spawn("XLine", 0, 0, mark)
    local b = self:Spawn("XLine", 0, 0, mark)
    self:Stroke(a, Path.Line(-s, s, s, -s), XCOL, 14)
    self:Stroke(b, Path.Line(s, s, -s, -s), XCOL, 14)
    Tween.To(a, {trimEnd = 1}, 0.18, "easeInCubic")
    Tween.To(b, {trimEnd = 1}, 0.18, "easeOutCubic", {delay = 0.16})
    return {root = mark, parts = {a, b}}
end

function T:PlaceO(c, r)
    local x, y = self:CellCenter(c, r)
    local mark = self:Spawn("O", x, y)
    mark:SetZIndex(2)
    mark:SetRotation(90)
    local ring = self:Spawn("ORing", 0, 0, mark)
    self:Stroke(ring, Path.Circle(42), OCOL, 14)
    Tween.To(ring, {trimEnd = 1}, 0.35, "easeInOutCubic")
    return {root = mark, parts = {ring}}
end

function T:Play(c, r)
    local mark = self.turn == "X" and self:PlaceX(c, r) or self:PlaceO(c, r)
    Tween.From(mark.root, {scale = {0.6, 0.6}}, 0.4, "easeOutBack")
    mark.c, mark.r, mark.player = c, r, self.turn
    self.board[r][c] = self.turn
    self.marks[#self.marks + 1] = mark
    self.moves = self.moves + 1

    local line = self:Winner()
    if line then
        self.state = "over"
        Tween.Sequence({0.35, function() self:ShowWin(line) end})
    elseif self.moves == 9 then
        self.state = "over"
        Tween.Sequence({0.35, function() self:ShowDraw() end})
    else
        self.turn = self.turn == "X" and "O" or "X"
        self:SetStatus(self.turn .. "'s turn", self.turn == "X" and XCOL or OCOL)
    end
end

function T:Winner()
    for _, line in ipairs(LINES) do
        local a = self.board[line[1][2]][line[1][1]]
        if a and a == self.board[line[2][2]][line[2][1]] and a == self.board[line[3][2]][line[3][1]] then
            return line
        end
    end
    return nil
end

function T:ShowWin(line)
    local x1, y1 = self:CellCenter(line[1][1], line[1][2])
    local x2, y2 = self:CellCenter(line[3][1], line[3][2])
    local dx, dy = Math.Normalize(x2 - x1, y2 - y1)
    local strike = self:Spawn("Strike", 0, 0)
    strike:SetZIndex(3)
    self:Stroke(strike, Path.Line(x1 - dx * 60, y1 - dy * 60, x2 + dx * 60, y2 + dy * 60), WIN, 12)
    Tween.To(strike, {trimEnd = 1}, 0.4, "easeOutCubic")

    for _, mark in ipairs(self.marks) do
        local inLine = false
        for _, cell in ipairs(line) do
            if cell[1] == mark.c and cell[2] == mark.r then
                inLine = true
            end
        end
        if inLine then
            Tween.To(mark.root, {scale = {1.15, 1.15}}, 0.4, "easeInOutSine", {loop = true, yoyo = true})
        else
            for _, part in ipairs(mark.parts) do
                Tween.To(part, {alpha = 60}, 0.4, "easeOut")
            end
        end
    end
    local winner = self.board[line[1][2]][line[1][1]]
    self:SetStatus(winner .. " wins!  Click to play again", WIN)
end

function T:ShowDraw()
    for i, mark in ipairs(self.marks) do
        Tween.To(mark.root, {rotation = "+=14"}, 0.5, Curves.Sine(2), {delay = i * 0.03})
    end
    self:SetStatus("Draw!  Click to play again", TEXT)
end

function T:Restart()
    self.state = "leaving"
    for i, e in ipairs(self.spawned) do
        Tween.To(e, {alpha = 0, y = "-=30"}, 0.3, "easeIn", {delay = i * 0.01})
    end
    Tween.Sequence({0.5, function()
        Tween.KillAll()
        for _, e in ipairs(self.spawned) do
            if e:IsValid() then
                e:Destroy()
            end
        end
        self:Build()
    end})
end

function T:OnUpdate(dt)
    if not Input.IsMouseJustPressed(Key.MOUSE_LEFT) or not Input.IsMouseInViewport() then
        return
    end
    if self.state == "over" then
        self:Restart()
        return
    end
    if self.state ~= "play" then
        return
    end
    local c, r = self:CellAt(self.entity:GetMouseWorld())
    if c and not self.board[r][c] then
        self:Play(c, r)
    end
end

function T:OnRender()
    if self.state ~= "play" then
        return
    end
    local c, r = self:CellAt(self.entity:GetMouseWorld())
    if c and not self.board[r][c] then
        local x, y = self:CellCenter(c, r)
        local color = self.turn == "X" and XCOL or OCOL
        Draw.SetZ(0)
        Draw.RoundRect(x, y, self.cell - 24, self.cell - 24, 18, color[1], color[2], color[3], 28)
    end
end

return T
