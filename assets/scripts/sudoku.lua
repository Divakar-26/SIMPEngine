local Sudoku = {}

local CELL = 60
local BOARD_SIZE = 540
local BOARD_LEFT = -270
local BOARD_TOP = 245
local BOARD_RIGHT = BOARD_LEFT + BOARD_SIZE
local BOARD_BOTTOM = BOARD_TOP - BOARD_SIZE
local BOARD_CENTER_Y = (BOARD_TOP + BOARD_BOTTOM) * 0.5
local PANEL_X = 470
local PANEL_Y = -25
local PANEL_W = 300
local PANEL_H = 540

local Palette = {
    background = {9, 14, 22},
    surface = {18, 28, 42},
    surfaceRaised = {25, 38, 55},
    surfaceHover = {35, 53, 73},
    board = {17, 28, 43},
    cell = {20, 32, 48},
    cellSoft = {25, 40, 58},
    selected = {40, 91, 112},
    selectedEdge = {101, 220, 224},
    related = {27, 47, 64},
    sameNumber = {42, 76, 91},
    fineGrid = {56, 76, 96},
    boldGrid = {112, 144, 165},
    text = {231, 239, 246},
    muted = {133, 153, 171},
    given = {238, 244, 249},
    entered = {100, 213, 222},
    accent = {91, 211, 215},
    accentDark = {30, 108, 119},
    green = {115, 217, 166},
    red = {255, 112, 124},
    gold = {245, 194, 106},
    white = {255, 255, 255}
}

local function Color(name, alpha)
    local c = Palette[name]
    return c[1], c[2], c[3], alpha or 255
end

local function CopyGrid(source)
    local result = {}
    for r = 1, 9 do
        result[r] = {}
        for c = 1, 9 do
            result[r][c] = source[r][c]
        end
    end
    return result
end

local function Shuffle(values)
    for i = #values, 2, -1 do
        local j = math.random(i)
        values[i], values[j] = values[j], values[i]
    end
    return values
end

local function MakeSolution()
    local bands = Shuffle({0, 1, 2})
    local stacks = Shuffle({0, 1, 2})
    local rows = {}
    local columns = {}

    for _, band in ipairs(bands) do
        local inner = Shuffle({0, 1, 2})
        for _, offset in ipairs(inner) do
            rows[#rows + 1] = band * 3 + offset
        end
    end

    for _, stack in ipairs(stacks) do
        local inner = Shuffle({0, 1, 2})
        for _, offset in ipairs(inner) do
            columns[#columns + 1] = stack * 3 + offset
        end
    end

    local digits = Shuffle({1, 2, 3, 4, 5, 6, 7, 8, 9})
    local solution = {}

    for r = 1, 9 do
        solution[r] = {}
        for c = 1, 9 do
            local baseRow = rows[r]
            local baseColumn = columns[c]
            local value = (baseRow * 3 + math.floor(baseRow / 3) + baseColumn) % 9 + 1
            solution[r][c] = digits[value]
        end
    end

    return solution
end

local function Candidates(grid, row, column)
    local used = {}

    for i = 1, 9 do
        local rowValue = grid[row][i]
        local columnValue = grid[i][column]
        if rowValue ~= 0 then
            used[rowValue] = true
        end
        if columnValue ~= 0 then
            used[columnValue] = true
        end
    end

    local boxRow = math.floor((row - 1) / 3) * 3 + 1
    local boxColumn = math.floor((column - 1) / 3) * 3 + 1

    for r = boxRow, boxRow + 2 do
        for c = boxColumn, boxColumn + 2 do
            local value = grid[r][c]
            if value ~= 0 then
                used[value] = true
            end
        end
    end

    local result = {}
    for value = 1, 9 do
        if not used[value] then
            result[#result + 1] = value
        end
    end

    return result
end

local function CountSolutions(grid, limit)
    local bestRow = nil
    local bestColumn = nil
    local bestCandidates = nil
    local bestCount = 10

    for r = 1, 9 do
        for c = 1, 9 do
            if grid[r][c] == 0 then
                local candidates = Candidates(grid, r, c)
                local count = #candidates
                if count == 0 then
                    return 0
                end
                if count < bestCount then
                    bestRow = r
                    bestColumn = c
                    bestCandidates = candidates
                    bestCount = count
                    if count == 1 then
                        break
                    end
                end
            end
        end
        if bestCount == 1 then
            break
        end
    end

    if not bestRow then
        return 1
    end

    local total = 0
    for _, value in ipairs(bestCandidates) do
        grid[bestRow][bestColumn] = value
        total = total + CountSolutions(grid, limit - total)
        grid[bestRow][bestColumn] = 0
        if total >= limit then
            return total
        end
    end

    return total
end

local function MakePuzzle(solution, holes)
    local puzzle = CopyGrid(solution)
    local positions = {}
    for i = 1, 81 do
        positions[i] = i
    end
    Shuffle(positions)

    local removed = 0
    for _, index in ipairs(positions) do
        if removed >= holes then
            break
        end

        local row = math.floor((index - 1) / 9) + 1
        local column = (index - 1) % 9 + 1
        local previous = puzzle[row][column]
        puzzle[row][column] = 0

        if CountSolutions(CopyGrid(puzzle), 2) == 1 then
            removed = removed + 1
        else
            puzzle[row][column] = previous
        end
    end

    return puzzle
end

local function Index(row, column)
    return (row - 1) * 9 + column
end

local function CellCenter(row, column)
    return BOARD_LEFT + (column - 0.5) * CELL,
        BOARD_TOP - (row - 0.5) * CELL
end

local function Inside(x, y, cx, cy, width, height)
    return x >= cx - width * 0.5 and x <= cx + width * 0.5
        and y >= cy - height * 0.5 and y <= cy + height * 0.5
end

local function CopyNotes(notes)
    local copy = {}
    for value, enabled in pairs(notes or {}) do
        if enabled then
            copy[value] = true
        end
    end
    return copy
end

local function FormatTime(seconds)
    local total = math.floor(seconds)
    local minutes = math.floor(total / 60)
    local remainder = total % 60
    return string.format("%02d:%02d", minutes, remainder)
end

local function DrawText(text, x, centerY, size, color, alpha, align)
    local r, g, b, a = Color(color, alpha)
    Draw.Text(tostring(text), x, centerY + size * 0.6, size, r, g, b, a, align or "center")
end

local function DrawButton(x, y, width, height, label, active, accent, size)
    local fill = active and "selected" or "surfaceRaised"
    local r, g, b = Color(fill)
    Draw.RoundRect(x, y, width, height, math.min(12, height * 0.28), r, g, b, active and 255 or 235)

    if active or accent then
        local cr, cg, cb = Color(active and "selectedEdge" or "accentDark")
        Draw.RoundRect(x, y - height * 0.5 + 2, width - 8, 3, 1.5, cr, cg, cb, active and 230 or 180)
    end

    DrawText(label, x, y - (size or 15) * 0.06, size or 15, active and "white" or "text", 255)
end

local function HitButton(mx, my, x, y, width, height)
    return Inside(mx, my, x, y, width, height)
end

function Sudoku:CreateGridVectors()
    local finePath = Path.New()
    local boldPath = Path.New()

    for i = 1, 8 do
        local x = BOARD_LEFT + i * CELL
        local y = BOARD_TOP - i * CELL
        if i % 3 == 0 then
            boldPath:MoveTo(x, BOARD_BOTTOM):LineTo(x, BOARD_TOP)
            boldPath:MoveTo(BOARD_LEFT, y):LineTo(BOARD_RIGHT, y)
        else
            finePath:MoveTo(x, BOARD_BOTTOM):LineTo(x, BOARD_TOP)
            finePath:MoveTo(BOARD_LEFT, y):LineTo(BOARD_RIGHT, y)
        end
    end

    boldPath:MoveTo(BOARD_LEFT, BOARD_BOTTOM)
        :LineTo(BOARD_RIGHT, BOARD_BOTTOM)
        :LineTo(BOARD_RIGHT, BOARD_TOP)
        :LineTo(BOARD_LEFT, BOARD_TOP)
        :Close()

    local fineEntity = Scene.Spawn("Sudoku Fine Grid", 0, 0, self.entity)
    fineEntity:SetZIndex(8)
    local fine = fineEntity:Add("Vector")
    fine:SetPath(finePath)
    fine:SetFillEnabled(false)
    fine:SetStroke(Color("fineGrid"))
    fine:SetStrokeWidth(1.25)
    fine:SetCap("round")
    fine:SetJoin("round")
    fine:SetTrim(0, 0, 0)

    local boldEntity = Scene.Spawn("Sudoku Strong Grid", 0, 0, self.entity)
    boldEntity:SetZIndex(9)
    local bold = boldEntity:Add("Vector")
    bold:SetPath(boldPath)
    bold:SetFillEnabled(false)
    bold:SetStroke(Color("boldGrid"))
    bold:SetStrokeWidth(3.2)
    bold:SetCap("round")
    bold:SetJoin("round")
    bold:SetTrim(0, 0, 0)

    self.gridFineEntity = fineEntity
    self.gridBoldEntity = boldEntity
    self.gridFine = fine
    self.gridBold = bold
end

function Sudoku:RevealGrid()
    Tween.Kill(self.gridFineEntity)
    Tween.Kill(self.gridBoldEntity)
    self.gridFine:SetTrim(0, 0, 0)
    self.gridBold:SetTrim(0, 0, 0)

    Tween.To(self.gridFineEntity, {trimEnd = 1}, 1.35, "easeInOutCubic")
    Tween.To(self.gridBoldEntity, {trimEnd = 1}, 0.85, "easeOutCubic", {delay = 0.28})
end

function Sudoku:CreateRipple(row, column, colorName)
    local x, y = CellCenter(row, column)
    local entity = Scene.Spawn("Sudoku Ripple", x, y, self.entity)
    entity:SetZIndex(11)

    local vector = entity:Add("Vector")
    vector:SetCircle(16)
    vector:SetFillEnabled(false)
    vector:SetStroke(Color(colorName or "accent", 220))
    vector:SetStrokeWidth(2.8)
    vector:SetCap("round")

    Tween.To(entity, {
        scale = {3.15, 3.15},
        alpha = 0
    }, 0.48, "easeOutCubic", {
        onComplete = function()
            if entity:IsValid() then
                entity:Destroy()
            end
        end
    })

    Tween.To(entity, {strokeWidth = 0.9}, 0.48, "easeOutCubic")
end

function Sudoku:KillPopTweens()
    for _, handle in pairs(self.popTweens or {}) do
        handle:Kill()
    end
    self.popTweens = {}
end

function Sudoku:PopCell(index, delay)
    if self.popTweens and self.popTweens[index] then
        self.popTweens[index]:Kill()
    end

    self.cellPop[index] = 0
    self.popTweens = self.popTweens or {}
    self.popTweens[index] = Tween.Value(0, 1, 0.34, "easeOutBack", function(value)
        self.cellPop[index] = value
    end, {
        delay = delay or 0,
        onComplete = function()
            self.popTweens[index] = nil
        end
    })
end

function Sudoku:AnimateGivens()
    for index = 1, 81 do
        local row = math.floor((index - 1) / 9) + 1
        local column = (index - 1) % 9 + 1
        if self.board[row][column] ~= 0 then
            self:PopCell(index, (index - 1) * 0.006)
        else
            self.cellPop[index] = 1
        end
    end
end

function Sudoku:NewGame()
    self:KillPopTweens()
    local targetHoles = ({34, 43, 50})[self.difficulty]
    self.solution = MakeSolution()
    self.original = MakePuzzle(self.solution, targetHoles)
    self.board = CopyGrid(self.original)
    self.givens = {}
    self.notes = {}
    self.wrong = {}
    self.cellPop = {}
    self.undoStack = {}
    self.elapsed = 0
    self.mistakes = 0
    self.hints = 0
    self.won = false
    self.notesMode = false
    self.lastEntry = nil
    self.selected = 1

    for index = 1, 81 do
        local row = math.floor((index - 1) / 9) + 1
        local column = (index - 1) % 9 + 1
        local value = self.board[row][column]
        self.givens[index] = value ~= 0
        self.notes[index] = {}
        self.cellPop[index] = 1
    end

    self.selected = self:FirstEditable()
    self:RevealGrid()
    self:AnimateGivens()
end

function Sudoku:ResetPuzzle()
    self:KillPopTweens()
    self.board = CopyGrid(self.original)
    self.notes = {}
    self.wrong = {}
    self.undoStack = {}
    self.elapsed = 0
    self.mistakes = 0
    self.hints = 0
    self.won = false
    self.notesMode = false

    for index = 1, 81 do
        self.notes[index] = {}
        self.cellPop[index] = 1
    end

    self:AnimateGivens()
end

function Sudoku:Snapshot(index)
    local row = math.floor((index - 1) / 9) + 1
    local column = (index - 1) % 9 + 1
    self.undoStack[#self.undoStack + 1] = {
        index = index,
        value = self.board[row][column],
        notes = CopyNotes(self.notes[index]),
        wrong = self.wrong[index],
        mistakes = self.mistakes
    }
end

function Sudoku:Undo()
    local action = table.remove(self.undoStack)
    if not action then
        return
    end

    local row = math.floor((action.index - 1) / 9) + 1
    local column = (action.index - 1) % 9 + 1
    self.board[row][column] = action.value
    self.notes[action.index] = action.notes
    self.wrong[action.index] = action.wrong
    self.mistakes = action.mistakes
    self.won = false
    self:PopCell(action.index, 0)
end

function Sudoku:FirstEditable()
    for index = 1, 81 do
        if not self.givens[index] then
            return index
        end
    end
    return 1
end

function Sudoku:IsConflict(row, column)
    local value = self.board[row][column]
    if value == 0 then
        return false
    end

    for i = 1, 9 do
        if i ~= column and self.board[row][i] == value then
            return true
        end
        if i ~= row and self.board[i][column] == value then
            return true
        end
    end

    local boxRow = math.floor((row - 1) / 3) * 3 + 1
    local boxColumn = math.floor((column - 1) / 3) * 3 + 1
    for r = boxRow, boxRow + 2 do
        for c = boxColumn, boxColumn + 2 do
            if (r ~= row or c ~= column) and self.board[r][c] == value then
                return true
            end
        end
    end

    return false
end

function Sudoku:IsWrong(index)
    local row = math.floor((index - 1) / 9) + 1
    local column = (index - 1) % 9 + 1
    local value = self.board[row][column]
    return value ~= 0 and not self.givens[index] and value ~= self.solution[row][column]
end

function Sudoku:IsSolved()
    for row = 1, 9 do
        for column = 1, 9 do
            if self.board[row][column] ~= self.solution[row][column] then
                return false
            end
        end
    end
    return true
end

function Sudoku:CheckWin()
    if self:IsSolved() and not self.won then
        self.won = true
        self:Celebrate()
    end
end

function Sudoku:Celebrate()
    for i = 1, 28 do
        local x = -260 + math.random() * 520
        local y = -250 + math.random() * 500
        local entity = Scene.Spawn("Sudoku Confetti", x, y, self.entity)
        entity:SetZIndex(14)
        entity:SetRotation(math.random(0, 360))

        local vector = entity:Add("Vector")
        vector:SetRect(math.random(6, 11), math.random(9, 17), 2)
        vector:SetFill(80 + math.random(100), 150 + math.random(80), 170 + math.random(70), 255)
        vector:SetStrokeEnabled(false)

        local side = math.random() * 2 - 1
        local path = Path.New():MoveTo(x, y)
            :CubicTo(x + side * 160, y + 210, x + side * 260, y - 60, x + side * 320, y - 460)

        Tween.Along(entity, path, 2.2, "easeInCubic")
        Tween.To(entity, {rotation = math.random(-540, 540), alpha = 0}, 2.2, "linear", {
            delay = 0.15,
            onComplete = function()
                if entity:IsValid() then
                    entity:Destroy()
                end
            end
        })
    end

    log.info("Sudoku solved in " .. FormatTime(self.elapsed))
end

function Sudoku:SetNumber(value)
    if self.won then
        return
    end

    if not self.selected then
        self.selected = self:FirstEditable()
    end

    local index = self.selected
    if self.givens[index] then
        return
    end

    local row = math.floor((index - 1) / 9) + 1
    local column = (index - 1) % 9 + 1

    self:Snapshot(index)

    if self.notesMode then
        if self.board[row][column] ~= 0 then
            self.board[row][column] = 0
        end
        self.notes[index][value] = not self.notes[index][value] or nil
        self.cellPop[index] = 1
        return
    end

    self.board[row][column] = value
    self.notes[index] = {}
    self.wrong[index] = value ~= self.solution[row][column]
    self.cellPop[index] = 0
    self.lastEntry = value
    self:PopCell(index, 0)
    self:CreateRipple(row, column, self.wrong[index] and "red" or "accent")

    if self.wrong[index] then
        self.mistakes = self.mistakes + 1
    end

    self:CheckWin()
end

function Sudoku:Erase()
    if not self.selected or self.givens[self.selected] then
        return
    end

    local row = math.floor((self.selected - 1) / 9) + 1
    local column = (self.selected - 1) % 9 + 1
    self:Snapshot(self.selected)
    self.board[row][column] = 0
    self.notes[self.selected] = {}
    self.wrong[self.selected] = nil
    self.cellPop[self.selected] = 1
end

function Sudoku:Hint()
    if self.won then
        return
    end

    if not self.selected then
        self.selected = self:FirstEditable()
    end

    local index = self.selected
    if self.givens[index] then
        return
    end

    local row = math.floor((index - 1) / 9) + 1
    local column = (index - 1) % 9 + 1
    if self.board[row][column] == self.solution[row][column] then
        return
    end
    self:Snapshot(index)
    self.board[row][column] = self.solution[row][column]
    self.notes[index] = {}
    self.wrong[index] = nil
    self.hints = self.hints + 1
    self:PopCell(index, 0)
    self:CreateRipple(row, column, "gold")
    self:CheckWin()
end

function Sudoku:MoveSelection(dx, dy)
    local row = math.floor((self.selected - 1) / 9) + 1
    local column = (self.selected - 1) % 9 + 1
    row = math.max(1, math.min(9, row + dy))
    column = math.max(1, math.min(9, column + dx))
    self.selected = Index(row, column)
    self:CreateRipple(row, column, "accent")
end

function Sudoku:HandleMouse(mx, my)
    for difficulty = 1, 3 do
        local x = PANEL_X - 104 + (difficulty - 1) * 104
        if HitButton(mx, my, x, 184, 94, 38) then
            if self.difficulty ~= difficulty then
                self.difficulty = difficulty
                self:NewGame()
            end
            return
        end
    end

    if HitButton(mx, my, 395, -75, 126, 46) then
        self.notesMode = not self.notesMode
        return
    end
    if HitButton(mx, my, 545, -75, 126, 46) then
        self:Erase()
        return
    end
    if HitButton(mx, my, 395, -137, 126, 46) then
        self:Hint()
        return
    end
    if HitButton(mx, my, 545, -137, 126, 46) then
        self:Undo()
        return
    end
    if HitButton(mx, my, PANEL_X, -205, 276, 44) then
        self:ResetPuzzle()
        return
    end
    if HitButton(mx, my, PANEL_X, -258, 276, 46) then
        self:NewGame()
        return
    end

    if my >= -384 and my <= -334 and mx >= BOARD_LEFT and mx <= BOARD_RIGHT then
        local number = math.floor((mx - BOARD_LEFT) / CELL) + 1
        if number >= 1 and number <= 9 then
            self:SetNumber(number)
            return
        end
    end

    if mx >= BOARD_LEFT and mx <= BOARD_RIGHT and my >= BOARD_BOTTOM and my <= BOARD_TOP then
        local column = math.floor((mx - BOARD_LEFT) / CELL) + 1
        local row = math.floor((BOARD_TOP - my) / CELL) + 1
        row = math.max(1, math.min(9, row))
        column = math.max(1, math.min(9, column))
        self.selected = Index(row, column)
        self:CreateRipple(row, column, "accent")
    end
end

function Sudoku:HandleKeyboard()
    if Input.IsKeyJustPressed(Key.NUM_1) then self:SetNumber(1) end
    if Input.IsKeyJustPressed(Key.NUM_2) then self:SetNumber(2) end
    if Input.IsKeyJustPressed(Key.NUM_3) then self:SetNumber(3) end
    if Input.IsKeyJustPressed(Key.NUM_4) then self:SetNumber(4) end
    if Input.IsKeyJustPressed(Key.NUM_5) then self:SetNumber(5) end
    if Input.IsKeyJustPressed(Key.NUM_6) then self:SetNumber(6) end
    if Input.IsKeyJustPressed(Key.NUM_7) then self:SetNumber(7) end
    if Input.IsKeyJustPressed(Key.NUM_8) then self:SetNumber(8) end
    if Input.IsKeyJustPressed(Key.NUM_9) then self:SetNumber(9) end

    if Input.IsKeyJustPressed(Key.BACKSPACE) or Input.IsKeyJustPressed(Key.DELETE) then
        self:Erase()
    end
    if Input.IsKeyJustPressed(Key.N) then
        self.notesMode = not self.notesMode
    end
    if Input.IsKeyJustPressed(Key.H) then
        self:Hint()
    end
    if Input.IsKeyJustPressed(Key.R) then
        self:ResetPuzzle()
    end
    if Input.IsKeyJustPressed(Key.F5) then
        self:NewGame()
    end
    if Input.IsKeyDown(Key.LCTRL) and Input.IsKeyJustPressed(Key.Z) then
        self:Undo()
    end
    if Input.IsKeyDown(Key.LCTRL) and Input.IsKeyJustPressed(Key.N) then
        self:NewGame()
    end

    if Input.IsKeyJustPressed(Key.LEFT) then self:MoveSelection(-1, 0) end
    if Input.IsKeyJustPressed(Key.RIGHT) then self:MoveSelection(1, 0) end
    if Input.IsKeyJustPressed(Key.UP) then self:MoveSelection(0, -1) end
    if Input.IsKeyJustPressed(Key.DOWN) then self:MoveSelection(0, 1) end
end

function Sudoku:OnCreate()
    math.randomseed(os.time())
    self.difficulty = 2
    self.notesMode = false
    self.elapsed = 0
    self.mistakes = 0
    self.hints = 0
    self.won = false
    self.selected = 1
    self.popTweens = {}

    Scene.SetLighting(false)
    self:CreateGridVectors()
    self:NewGame()
end

function Sudoku:OnUpdate(dt)
    if not self.won then
        self.elapsed = self.elapsed + dt
    end

    self:HandleKeyboard()

    if Input.IsMouseJustPressed(Key.MOUSE_LEFT) and Input.IsMouseInViewport() then
        local mx, my = self.entity:GetMouseWorld()
        self:HandleMouse(mx, my)
    end
end

function Sudoku:DrawBackground()
    Draw.SetZ(0)
    Draw.RoundRect(0, BOARD_CENTER_Y, 560, 560, 22, 17, 28, 43, 255)
    Draw.RoundRect(PANEL_X, PANEL_Y, PANEL_W + 8, PANEL_H + 8, 24, 7, 12, 20, 110)
    Draw.RoundRect(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, 22, 18, 29, 44, 255)
    Draw.Circle(-635, 420, 90, 46, 112, 126, 16)
    Draw.Circle(650, -420, 140, 42, 106, 122, 12)
    Draw.Circle(550, 420, 52, 50, 140, 143, 13)
end

function Sudoku:DrawBoard()
    Draw.SetZ(4)
    local selectedRow = math.floor((self.selected - 1) / 9) + 1
    local selectedColumn = (self.selected - 1) % 9 + 1
    local selectedValue = self.board[selectedRow][selectedColumn]

    for row = 1, 9 do
        for column = 1, 9 do
            local index = Index(row, column)
            local value = self.board[row][column]
            local x, y = CellCenter(row, column)
            local fill = "cell"
            local inRelated = row == selectedRow or column == selectedColumn
            local sameBox = math.floor((row - 1) / 3) == math.floor((selectedRow - 1) / 3)
                and math.floor((column - 1) / 3) == math.floor((selectedColumn - 1) / 3)

            if inRelated or sameBox then
                fill = "related"
            end
            if selectedValue ~= 0 and value == selectedValue then
                fill = "sameNumber"
            end
            if self:IsWrong(index) or self:IsConflict(row, column) then
                fill = "red"
            end
            if index == self.selected then
                fill = "selected"
            end

            local r, g, b = Color(fill, index == self.selected and 255 or 190)
            Draw.RoundRect(x, y, CELL - 3, CELL - 3, 4, r, g, b, index == self.selected and 255 or 190)
        end
    end

    Draw.SetZ(12)
    for row = 1, 9 do
        for column = 1, 9 do
            local index = Index(row, column)
            local value = self.board[row][column]
            local x, y = CellCenter(row, column)

            if value ~= 0 then
                local color = self.givens[index] and "given" or "entered"
                if self:IsWrong(index) or self:IsConflict(row, column) then
                    color = "red"
                end
                local pop = self.cellPop[index] or 1
                local size = 31 * (0.68 + 0.32 * pop)
                DrawText(value, x, y, size, color, math.floor(255 * math.min(1, pop)))
            else
                local notes = self.notes[index]
                for note = 1, 9 do
                    if notes[note] then
                        local offsetColumn = (note - 1) % 3
                        local offsetRow = math.floor((note - 1) / 3)
                        local nx = x - 17 + offsetColumn * 17
                        local ny = y + 17 - offsetRow * 17
                        DrawText(note, nx, ny, 11, "muted", 230)
                    end
                end
            end
        end
    end

    local sx, sy = CellCenter(selectedRow, selectedColumn)
    Draw.SetZ(13)
    local sr, sg, sb = Color("selectedEdge", 245)
    Draw.Line(sx - 27, sy + 27, sx + 27, sy + 27, 2.5, sr, sg, sb, 245)
    Draw.Line(sx + 27, sy + 27, sx + 27, sy - 27, 2.5, sr, sg, sb, 245)
    Draw.Line(sx + 27, sy - 27, sx - 27, sy - 27, 2.5, sr, sg, sb, 245)
    Draw.Line(sx - 27, sy - 27, sx - 27, sy + 27, 2.5, sr, sg, sb, 245)
end

function Sudoku:DrawHeader()
    Draw.SetZ(20)
    DrawText("SUDOKU", 0, 438, 43, "text")
    DrawText("A little focus, one square at a time.", 0, 398, 16, "muted")
    DrawText("LOGIC  /  PATIENCE  /  FLOW", 0, 348, 10, "accent", 180)
end

function Sudoku:DrawNumberPad()
    Draw.SetZ(20)
    for value = 1, 9 do
        local x = BOARD_LEFT + (value - 0.5) * CELL
        local active = self.lastEntry == value
        local r, g, b = Color(active and "selected" or "surfaceRaised")
        Draw.RoundRect(x, -359, 50, 46, 10, r, g, b, 255)
        DrawText(value, x, -359, 22, active and "white" or "entered")
    end
    DrawText("SELECT A CELL  /  CLICK A NUMBER OR PRESS 1–9", 0, -416, 11, "muted", 220)
    DrawText("N NOTES     H HINT     R RESET     CTRL+Z UNDO     F5 NEW GAME", 0, -478, 10, "muted", 190)
end

function Sudoku:DrawPanel()
    Draw.SetZ(20)
    DrawText("YOUR PUZZLE", PANEL_X, 216, 11, "muted")

    local names = {"EASY", "MEDIUM", "HARD"}
    for difficulty = 1, 3 do
        local x = PANEL_X - 104 + (difficulty - 1) * 104
        DrawButton(x, 184, 94, 38, names[difficulty], self.difficulty == difficulty, true, 11)
    end

    DrawText("TIME", PANEL_X - 84, 126, 10, "muted")
    DrawText(FormatTime(self.elapsed), PANEL_X - 84, 83, 34, "text")
    DrawText("MISTAKES", PANEL_X + 83, 126, 10, "muted")
    DrawText(string.format("%02d", self.mistakes), PANEL_X + 83, 83, 34, self.mistakes > 0 and "red" or "green")

    local filled = 0
    for row = 1, 9 do
        for column = 1, 9 do
            if self.board[row][column] ~= 0 then
                filled = filled + 1
            end
        end
    end
    local progress = filled / 81
    DrawText("PROGRESS", PANEL_X - 104, 24, 10, "muted")
    DrawText(string.format("%d / 81", filled), PANEL_X + 104, 24, 11, "text")
    Draw.RoundRect(PANEL_X, -7, 250, 8, 4, 38, 54, 69, 255)
    if progress > 0 then
        Draw.RoundRect(PANEL_X - 125 + 125 * progress, -7, 250 * progress, 8, 4, 91, 211, 215, 255)
    end

    DrawText("TOOLS", PANEL_X - 108, -43, 10, "muted")
    DrawButton(395, -75, 126, 46, self.notesMode and "NOTES  ON" or "NOTES", self.notesMode, true, 13)
    DrawButton(545, -75, 126, 46, "ERASE", false, false, 13)
    DrawButton(395, -137, 126, 46, "HINT  +1", false, false, 13)
    DrawButton(545, -137, 126, 46, "UNDO", false, false, 13)
    DrawButton(PANEL_X, -205, 276, 44, "RESET PUZZLE", false, false, 13)
    DrawButton(PANEL_X, -258, 276, 46, "NEW GAME", true, true, 14)

    DrawText(string.format("HINTS USED  %d", self.hints), PANEL_X, -288, 9, "muted", 210)

    if self.won then
        Draw.SetZ(24)
        Draw.RoundRect(PANEL_X, -19, 268, 78, 16, 30, 92, 79, 245)
        DrawText("PUZZLE COMPLETE", PANEL_X, -8, 15, "white")
        DrawText("Beautifully done.", PANEL_X, -35, 11, "white", 220)
    elseif self.notesMode then
        DrawText("NOTES MODE ENABLED", PANEL_X, -288, 9, "accent")
    end
end

function Sudoku:OnRender()
    self:DrawBackground()
    self:DrawBoard()
    self:DrawHeader()
    self:DrawNumberPad()
    self:DrawPanel()
end

return Sudoku
