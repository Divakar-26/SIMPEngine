local Sudoku = {}

local INK = {52, 56, 72}
local MUTED = {150, 146, 158}
local CARD = {255, 255, 255}
local LINE = {234, 229, 218}
local LINE_BOLD = {190, 184, 168}
local SAND = {246, 242, 231}
local SAGE = {142, 166, 107}
local SAGE_INK = {96, 126, 70}
local SAGE_SOFT = {226, 236, 212}
local ROSE = {216, 162, 162}
local ROSE_INK = {196, 98, 98}
local BLUSH = {255, 226, 224}
local SHADOW = {120, 100, 70}

local CELL = 84
local BOARD = CELL * 9
local BX, BY = -190, -30
local LEFT, TOP = BX - BOARD / 2, BY + BOARD / 2
local PANEL_X, PANEL_W = 470, 380

local LEVELS = {
    {name = "Easy", givens = 40},
    {name = "Medium", givens = 32},
    {name = "Hard", givens = 26},
}

local function rgba(c, a)
    return c[1], c[2], c[3], math.floor(math.max(0, math.min(1, a or 1)) * 255)
end

local function mix(a, b, t)
    return {a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t, a[3] + (b[3] - a[3]) * t}
end

local function text(str, x, cy, size, c, a, align)
    Draw.Text(str, x, cy + size * 0.52, size, c[1], c[2], c[3], math.floor((a or 1) * 255), align or "center")
end

local function shuffle(t)
    for i = #t, 2, -1 do
        local j = math.random(i)
        t[i], t[j] = t[j], t[i]
    end
    return t
end

local function rowOf(i) return (i - 1) // 9 end
local function colOf(i) return (i - 1) % 9 end
local function boxOf(i) return (rowOf(i) // 3) * 3 + colOf(i) // 3 end

local function canPlace(g, i, v)
    local r, c = rowOf(i), colOf(i)
    for k = 0, 8 do
        if g[r * 9 + k + 1] == v or g[k * 9 + c + 1] == v then
            return false
        end
    end
    local br, bc = r - r % 3, c - c % 3
    for dr = 0, 2 do
        for dc = 0, 2 do
            if g[(br + dr) * 9 + bc + dc + 1] == v then
                return false
            end
        end
    end
    return true
end

local function fill(g)
    for i = 1, 81 do
        if g[i] == 0 then
            for _, v in ipairs(shuffle({1, 2, 3, 4, 5, 6, 7, 8, 9})) do
                if canPlace(g, i, v) then
                    g[i] = v
                    if fill(g) then
                        return true
                    end
                    g[i] = 0
                end
            end
            return false
        end
    end
    return true
end

local function countSolutions(g, limit)
    local best, bestCands
    for i = 1, 81 do
        if g[i] == 0 then
            local cands = {}
            for v = 1, 9 do
                if canPlace(g, i, v) then
                    cands[#cands + 1] = v
                end
            end
            if #cands == 0 then
                return 0
            end
            if not best or #cands < #bestCands then
                best, bestCands = i, cands
                if #cands == 1 then
                    break
                end
            end
        end
    end
    if not best then
        return 1
    end
    local total = 0
    for _, v in ipairs(bestCands) do
        g[best] = v
        total = total + countSolutions(g, limit - total)
        if total >= limit then
            break
        end
    end
    g[best] = 0
    return total
end

local function generate(givens)
    local solution = {}
    for i = 1, 81 do
        solution[i] = 0
    end
    fill(solution)
    local puzzle = {}
    for i = 1, 81 do
        puzzle[i] = solution[i]
    end
    local left = 81
    local order = {}
    for i = 1, 81 do
        order[i] = i
    end
    for _, i in ipairs(shuffle(order)) do
        if left <= givens then
            break
        end
        local keep = puzzle[i]
        puzzle[i] = 0
        if countSolutions(puzzle, 2) ~= 1 then
            puzzle[i] = keep
        else
            left = left - 1
        end
    end
    return puzzle, solution
end

local function cellCenter(i)
    return LEFT + (colOf(i) + 0.5) * CELL, TOP - (rowOf(i) + 0.5) * CELL
end

local function inside(mx, my, x, y, w, h)
    return mx >= x - w / 2 and mx <= x + w / 2 and my >= y - h / 2 and my <= y + h / 2
end

function Sudoku:OnCreate()
    self.time = 0
    self.level = 2
    self.hover = nil
    self.petals = {}
    self:NewGame()
end

function Sudoku:NewGame()
    Tween.KillAll()
    local puzzle, solution = generate(LEVELS[self.level].givens)
    self.solution = solution
    self.cells = {}
    for i = 1, 81 do
        self.cells[i] = {value = puzzle[i], given = puzzle[i] ~= 0, scale = 0.5, alpha = 0, flash = 0, shake = 0, hinted = false}
    end
    self.selected = 41
    self.selX, self.selY = cellCenter(41)
    self.mistakes = 0
    self.elapsed = 0
    self.won = false
    self.winAlpha = 0
    self.boardIn = 0
    self.petals = {}
    Tween.Value(0, 1, 0.6, "easeOutCubic", function(v) self.boardIn = v end)
    for i, cell in ipairs(self.cells) do
        local delay = 0.15 + (rowOf(i) + colOf(i)) * 0.028
        Tween.Value(0, 1, 0.5, "easeOutBack", function(v)
            cell.scale = 0.5 + 0.5 * v
            cell.alpha = math.min(1, v * 1.4)
        end, {delay = delay})
    end
end

function Sudoku:Select(i)
    if not i then
        return
    end
    self.selected = i
    local x, y = cellCenter(i)
    Tween.Value(self.selX, x, 0.16, "easeOutCubic", function(v) self.selX = v end)
    Tween.Value(self.selY, y, 0.16, "easeOutCubic", function(v) self.selY = v end)
end

function Sudoku:Pop(cell, from)
    Tween.Value(from or 1.35, 1, 0.42, "easeOutBack", function(v) cell.scale = v end)
end

function Sudoku:Shake(cell)
    Tween.Value(0, 1, 0.45, Curves.Sine(3), function(v) cell.shake = v * 7 end)
end

function Sudoku:Flash(i, delay)
    local cell = self.cells[i]
    Tween.Value(1, 0, 0.7, "easeOutCubic", function(v) cell.flash = v end, {delay = delay or 0})
end

function Sudoku:GroupDone(indices)
    for _, i in ipairs(indices) do
        if self.cells[i].value ~= self.solution[i] then
            return false
        end
    end
    return true
end

function Sudoku:Groups(i)
    local r, c, b = rowOf(i), colOf(i), boxOf(i)
    local row, col, box = {}, {}, {}
    for j = 1, 81 do
        if rowOf(j) == r then row[#row + 1] = j end
        if colOf(j) == c then col[#col + 1] = j end
        if boxOf(j) == b then box[#box + 1] = j end
    end
    return {row, col, box}
end

function Sudoku:CheckGroups(i)
    for _, group in ipairs(self:Groups(i)) do
        if self:GroupDone(group) then
            table.sort(group, function(a, b)
                local ar, ac = cellCenter(a)
                local br, bc = cellCenter(b)
                local ci, cj = cellCenter(i)
                return (ar - ci) ^ 2 + (ac - cj) ^ 2 < (br - ci) ^ 2 + (bc - cj) ^ 2
            end)
            for k, j in ipairs(group) do
                self:Flash(j, (k - 1) * 0.035)
            end
        end
    end
end

function Sudoku:Place(v)
    if self.won then
        return
    end
    local i = self.selected
    local cell = self.cells[i]
    if cell.given then
        self:Shake(cell)
        return
    end
    if cell.value == v then
        return
    end
    cell.value = v
    cell.hinted = false
    if v == 0 then
        return
    end
    if v == self.solution[i] then
        self:Pop(cell)
        self:CheckGroups(i)
        self:CheckWin()
    else
        self.mistakes = self.mistakes + 1
        self:Pop(cell, 1.2)
        self:Shake(cell)
    end
end

function Sudoku:Hint()
    if self.won then
        return
    end
    local target = nil
    local sel = self.cells[self.selected]
    if not sel.given and sel.value ~= self.solution[self.selected] then
        target = self.selected
    else
        local open = {}
        for i, cell in ipairs(self.cells) do
            if cell.value ~= self.solution[i] then
                open[#open + 1] = i
            end
        end
        if #open == 0 then
            return
        end
        target = open[math.random(#open)]
        self:Select(target)
    end
    local cell = self.cells[target]
    cell.value = self.solution[target]
    cell.hinted = true
    self:Pop(cell, 0.4)
    self:Flash(target)
    self:CheckGroups(target)
    self:CheckWin()
end

function Sudoku:CheckWin()
    for i, cell in ipairs(self.cells) do
        if cell.value ~= self.solution[i] then
            return
        end
    end
    self.won = true
    local cx, cy = cellCenter(41)
    for i = 1, 81 do
        local x, y = cellCenter(i)
        local d = math.sqrt((x - cx) ^ 2 + (y - cy) ^ 2)
        self:Flash(i, 0.1 + d / 900)
        Tween.Value(1, 1.18, 0.18, "easeOut", function(v) self.cells[i].scale = v end, {delay = 0.1 + d / 900, yoyo = true, loop = 1})
    end
    Tween.Value(0, 1, 0.6, "easeOutCubic", function(v) self.winAlpha = v end, {delay = 0.8})
    for _ = 1, 70 do
        self:AddPetal(true)
    end
end

function Sudoku:AddPetal(burst)
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

function Sudoku:CellAt(mx, my)
    local c = math.floor((mx - LEFT) / CELL)
    local r = math.floor((TOP - my) / CELL)
    if c < 0 or c > 8 or r < 0 or r > 8 then
        return nil
    end
    return r * 9 + c + 1
end

function Sudoku:Layout()
    local px = PANEL_X
    local items = {}
    for k, level in ipairs(LEVELS) do
        items[#items + 1] = {kind = "level", value = k, x = px - 120 + (k - 1) * 120, y = 300, w = 112, h = 48, label = level.name}
    end
    for v = 1, 9 do
        local r, c = (v - 1) // 3, (v - 1) % 3
        items[#items + 1] = {kind = "digit", value = v, x = px - 112 + c * 112, y = 40 - r * 104, w = 100, h = 92}
    end
    items[#items + 1] = {kind = "hint", x = px - 92, y = -306, w = 168, h = 64, label = "Hint"}
    items[#items + 1] = {kind = "new", x = px + 92, y = -306, w = 168, h = 64, label = "New game"}
    return items
end

function Sudoku:OnUpdate(dt)
    self.time = self.time + dt
    if not self.won then
        self.elapsed = self.elapsed + dt
    end

    local mx, my = self.entity:GetMouseWorld()
    local inView = Input.IsMouseInViewport()
    self.hover = nil
    if inView then
        for _, item in ipairs(self:Layout()) do
            if inside(mx, my, item.x, item.y, item.w, item.h) then
                self.hover = item
            end
        end
    end

    if inView and Input.IsMouseJustPressed(Key.MOUSE_LEFT) then
        local cell = self:CellAt(mx, my)
        if cell then
            self:Select(cell)
        elseif self.hover then
            local item = self.hover
            if item.kind == "digit" then
                self:Place(item.value)
            elseif item.kind == "level" and item.value ~= self.level then
                self.level = item.value
                self:NewGame()
            elseif item.kind == "hint" then
                self:Hint()
            elseif item.kind == "new" then
                self:NewGame()
            end
        end
    end

    for v = 1, 9 do
        if Input.IsKeyJustPressed(Key["NUM_" .. v]) then
            self:Place(v)
        end
    end
    if Input.IsKeyJustPressed(Key.BACKSPACE) or Input.IsKeyJustPressed(Key.DELETE) then
        self:Place(0)
    end
    if Input.IsKeyJustPressed(Key.H) then
        self:Hint()
    end
    if Input.IsKeyJustPressed(Key.N) then
        self:NewGame()
    end
    local r, c = rowOf(self.selected), colOf(self.selected)
    if Input.IsKeyJustPressed(Key.LEFT) then self:Select(r * 9 + (c + 8) % 9 + 1) end
    if Input.IsKeyJustPressed(Key.RIGHT) then self:Select(r * 9 + (c + 1) % 9 + 1) end
    if Input.IsKeyJustPressed(Key.UP) then self:Select(((r + 8) % 9) * 9 + c + 1) end
    if Input.IsKeyJustPressed(Key.DOWN) then self:Select(((r + 1) % 9) * 9 + c + 1) end

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

function Sudoku:Card(x, y, w, h, radius, z)
    Draw.SetZ(z - 0.02)
    Draw.RoundRect(x, y - 10, w + 8, h + 6, radius + 4, rgba(SHADOW, 0.06))
    Draw.RoundRect(x, y - 4, w + 2, h + 2, radius + 1, rgba(SHADOW, 0.07))
    Draw.SetZ(z)
    Draw.RoundRect(x, y, w, h, radius, rgba(CARD, 1))
end

function Sudoku:DrawBoard()
    local k = self.boardIn
    self:Card(BX, BY, BOARD + 36, BOARD + 36, 22, 0)

    local sel = self.selected
    local selValue = self.cells[sel].value
    Draw.SetZ(0.1)
    for i, cell in ipairs(self.cells) do
        local x, y = cellCenter(i)
        local related = rowOf(i) == rowOf(sel) or colOf(i) == colOf(sel) or boxOf(i) == boxOf(sel)
        local same = selValue ~= 0 and cell.value == selValue and i ~= sel
        local wrong = cell.value ~= 0 and cell.value ~= self.solution[i]
        local fill = nil
        if wrong then
            fill = BLUSH
        elseif same then
            fill = SAGE_SOFT
        elseif related then
            fill = SAND
        end
        if fill then
            Draw.RoundRect(x, y, CELL - 4, CELL - 4, 8, rgba(fill, k))
        end
        if cell.flash > 0.01 then
            Draw.RoundRect(x, y, CELL - 4, CELL - 4, 8, rgba(SAGE, cell.flash * 0.55))
        end
    end

    Draw.SetZ(0.2)
    Draw.RoundRect(self.selX, self.selY, CELL - 2, CELL - 2, 10, rgba(SAGE, 0.28 * k))
    Draw.SetZ(0.21)
    Draw.RoundRect(self.selX, self.selY, CELL - 12, CELL - 12, 7, rgba(CARD, 0.55 * k))

    Draw.SetZ(0.3)
    for n = 1, 8 do
        local bold = n % 3 == 0
        local c = bold and LINE_BOLD or LINE
        local w = bold and 3 or 1.5
        local x = LEFT + n * CELL
        local y = TOP - n * CELL
        Draw.Line(x, TOP - 6, x, TOP - BOARD + 6, w, rgba(c, k))
        Draw.Line(LEFT + 6, y, LEFT + BOARD - 6, y, w, rgba(c, k))
    end

    Draw.SetZ(0.5)
    for i, cell in ipairs(self.cells) do
        if cell.value ~= 0 then
            local x, y = cellCenter(i)
            local color = INK
            if not cell.given then
                color = cell.value == self.solution[i] and SAGE_INK or ROSE_INK
            end
            local size = 46 * cell.scale
            text(tostring(cell.value), x + cell.shake, y, size, color, cell.alpha)
        end
    end
end

function Sudoku:Button(item, label, primary, active)
    local hovered = self.hover == item
    local fill = primary and SAGE or (active and SAGE_SOFT or SAND)
    if hovered then
        fill = mix(fill, primary and INK or SAGE, 0.12)
    end
    Draw.SetZ(1.1)
    Draw.RoundRect(item.x, item.y, item.w, item.h, 16, rgba(fill, 1))
    Draw.SetZ(1.2)
    text(label, item.x, item.y, 24, primary and CARD or (active and SAGE_INK or INK), 1)
end

function Sudoku:DrawPanel()
    local px = PANEL_X
    self:Card(px, BY, PANEL_W, BOARD + 36, 22, 1)

    local remaining = {}
    for v = 1, 9 do
        remaining[v] = 9
    end
    for i, cell in ipairs(self.cells) do
        if cell.value ~= 0 and cell.value == self.solution[i] then
            remaining[cell.value] = remaining[cell.value] - 1
        end
    end

    for _, item in ipairs(self:Layout()) do
        if item.kind == "level" then
            self:Button(item, item.label, false, item.value == self.level)
        elseif item.kind == "digit" then
            local left = remaining[item.value]
            local hovered = self.hover == item
            Draw.SetZ(1.1)
            Draw.RoundRect(item.x, item.y, item.w, item.h, 18, rgba(hovered and SAGE_SOFT or SAND, 1))
            Draw.SetZ(1.2)
            local a = left == 0 and 0.25 or 1
            text(tostring(item.value), item.x, item.y + 6, 44, INK, a)
            if left > 0 then
                text(tostring(left), item.x, item.y - 30, 16, MUTED, 1)
            end
        elseif item.kind == "hint" then
            self:Button(item, item.label, false, false)
        elseif item.kind == "new" then
            self:Button(item, item.label, true, false)
        end
    end

    Draw.SetZ(1.2)
    local minutes = math.floor(self.elapsed / 60)
    local seconds = math.floor(self.elapsed % 60)
    text(string.format("%d:%02d", minutes, seconds), px - 150, 210, 40, INK, 1, "left")
    text("time", px - 150, 172, 18, MUTED, 1, "left")
    local mistakes = self.mistakes == 0 and SAGE_INK or ROSE_INK
    text(tostring(self.mistakes), px + 150, 210, 40, mistakes, 1, "right")
    text("mistakes", px + 150, 172, 18, MUTED, 1, "right")
end

function Sudoku:DrawHeader()
    Draw.SetZ(1)
    text("Sudoku", LEFT, TOP + 92, 64, INK, 1, "left")
    text("Fill every row, column and box with 1 to 9.", LEFT + 4, TOP + 42, 22, MUTED, 1, "left")
    text("arrows  ·  1-9  ·  backspace  ·  H hint  ·  N new", PANEL_X, TOP + 52, 18, MUTED, 1, "center")
end

function Sudoku:DrawWin()
    local a = self.winAlpha
    if a <= 0.01 then
        return
    end
    local rise = (1 - a) * 40
    local y = BY - rise
    Draw.SetZ(5)
    Draw.RoundRect(BX, y - 14, 548, 268, 30, rgba(SHADOW, 0.08 * a))
    Draw.RoundRect(BX, y - 5, 540, 262, 28, rgba(SHADOW, 0.08 * a))
    Draw.SetZ(5.05)
    Draw.RoundRect(BX, y, 536, 258, 28, rgba(CARD, a))
    Draw.SetZ(5.1)
    text("Solved", BX, y + 52, 84, SAGE_INK, a)
    local minutes = math.floor(self.elapsed / 60)
    local seconds = math.floor(self.elapsed % 60)
    local detail = string.format("%s  ·  %d:%02d  ·  %d %s", LEVELS[self.level].name, minutes, seconds, self.mistakes, self.mistakes == 1 and "mistake" or "mistakes")
    text(detail, BX, y - 26, 28, MUTED, a)
    text("New game or N for another", BX, y - 78, 22, MUTED, a * 0.8)
end

function Sudoku:DrawPetals()
    Draw.SetZ(4.5)
    for _, p in ipairs(self.petals) do
        Draw.Rect(p.x, p.y, p.size, p.size * 1.7, p.color[1], p.color[2], p.color[3], 230, p.rot)
    end
end

function Sudoku:OnRender()
    self:DrawHeader()
    self:DrawBoard()
    self:DrawPanel()
    self:DrawWin()
    self:DrawPetals()
end

return Sudoku
