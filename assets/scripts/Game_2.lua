-- Tic Tac Toe, drawn and animated like a Flutter / Material 3 app.
--
-- Everything on screen is built here with Scene.Spawn + Vector shapes + Path,
-- and animated with Tween.* (curves, stagger, spring, morph, trim, Along).
--
--   Click a cell to play      R = new round      M = 2 players / vs computer
--
-- Attach to the "Game" entity (see Game/TicTacToe/main.cpp).

local Game = {}

---------------------------------------------------------------------------
-- Theme (Material 3 baseline colors)
---------------------------------------------------------------------------
local COL = {
    card      = {255, 255, 255},
    cell      = {243, 237, 247},
    cellHover = {232, 222, 248},
    chip      = {230, 224, 233},
    onSurface = {29, 27, 32},
    onVariant = {73, 69, 79},
    hint      = {125, 118, 135},
    x         = {103, 80, 164},
    xSoft     = {234, 221, 255},
    o         = {0, 137, 123},
    oSoft     = {200, 243, 236},
    shadow    = {60, 40, 110},
    amber     = {255, 193, 7},
    pink      = {233, 30, 99},
    blue      = {33, 150, 243},
    white     = {255, 255, 255},
}

local function Strong(p) return p == 1 and COL.x or COL.o end
local function Soft(p) return p == 1 and COL.xSoft or COL.oSoft end

---------------------------------------------------------------------------
-- Layout (world units, y points up, view is 1080 tall)
---------------------------------------------------------------------------
local CELL, GAP, PAD = 176, 14, 22
local STEP = CELL + GAP
local CARD = 3 * CELL + 2 * GAP + 2 * PAD
local BOARD_Y = -50
local TITLE_Y, SEG_Y, CHIP_Y = 468, 388, 305
local STATUS_Y, BTN_Y, HINT_Y = -392, -462, -518
local SEG_W, SEG_H = 400, 60

local MISTAKE = 0.18 -- chance that the computer plays a random move

local LINES = {
    {1, 2, 3}, {4, 5, 6}, {7, 8, 9},
    {1, 4, 7}, {2, 5, 8}, {3, 6, 9},
    {1, 5, 9}, {3, 5, 7},
}

---------------------------------------------------------------------------
-- Rules + computer player
---------------------------------------------------------------------------
local function Winner(b)
    for _, l in ipairs(LINES) do
        local a = b[l[1]]
        if a ~= 0 and a == b[l[2]] and a == b[l[3]] then
            return a, l
        end
    end
    return 0, nil
end

local function Minimax(b, player, me, depth)
    local w = Winner(b)
    if w == me then return 10 - depth end
    if w ~= 0 then return depth - 10 end
    local best = nil
    for i = 1, 9 do
        if b[i] == 0 then
            b[i] = player
            local s = Minimax(b, 3 - player, me, depth + 1)
            b[i] = 0
            if best == nil
                or (player == me and s > best)
                or (player ~= me and s < best) then
                best = s
            end
        end
    end
    return best or 0
end

---------------------------------------------------------------------------
-- Small builders
---------------------------------------------------------------------------
local function Spawn(name, x, y, z, parent)
    local e
    if parent then e = Scene.Spawn(name, x, y, parent) else e = Scene.Spawn(name, x, y) end
    e:SetZIndex(z)
    return e
end

local function RoundRect(name, x, y, w, h, r, color, z, alpha, parent)
    local e = Spawn(name, x, y, z, parent)
    local v = e:Add("Vector")
    v:SetRect(w, h, r)
    v:SetFill(color[1], color[2], color[3], alpha or 255)
    v:SetStrokeEnabled(false)
    return e, v
end

local function Label(name, text, x, y, size, color, z, parent, align)
    local e = Spawn(name, x, y, z, parent)
    local t = e:Add("Text")
    t:SetText(text)
    t:SetFontSize(size)
    t:SetColor(color[1], color[2], color[3], 255)
    t:SetAlign(align or 1)
    return e, t
end

local function StrokeShape(name, x, y, z, parent, path, color, width)
    local e = Spawn(name, x, y, z, parent)
    local v = e:Add("Vector")
    v:SetPath(path)
    v:SetFillEnabled(false)
    v:SetStroke(color[1], color[2], color[3])
    v:SetStrokeWidth(width)
    v:SetCap("round")
    v:SetJoin("round")
    return e, v
end

-- Both strokes of an X in one path (used for the hover ghost and the turn icon).
local function XPath(arm)
    return Path.Line(-arm, arm, arm, -arm):Append(Path.Line(-arm, -arm, arm, arm))
end

local function Dispose(e)
    if e and e:IsValid() then e:Destroy() end
end

---------------------------------------------------------------------------
-- Build the UI
---------------------------------------------------------------------------
function Game:OnCreate()
    math.randomseed(os.time())

    self.board = {0, 0, 0, 0, 0, 0, 0, 0, 0}
    self.marks = {}
    self.cells = {}
    self.buttons = {}
    self.scores = {0, 0, 0} -- X, O, draws
    self.mode = "cpu"
    self.state = "playing"  -- playing | won | draw
    self.turn = 1
    self.starter = 1
    self.winner = nil
    self.winLine = nil
    self.hover = nil
    self.cpuWait = nil
    self.lock = 1.5         -- ignore input while the intro plays

    self:BuildHeader()
    self:BuildSegmented()
    self:BuildChips()
    self:BuildBoard()
    self:BuildFooter()

    self:RefreshScores()
    self:RefreshTurn(true)
    self:PlayIntro()
end

function Game:BuildHeader()
    self.title = Label("Title", "Tic Tac Toe", 0, TITLE_Y, 64, COL.onSurface, 6)
end

-- Material "SegmentedButton": a pill with a thumb that springs between options.
function Game:BuildSegmented()
    self.segBg = RoundRect("SegmentBG", 0, SEG_Y, SEG_W, SEG_H, SEG_H / 2, COL.chip, 3)
    self.thumb = RoundRect("SegmentThumb", 100, SEG_Y, SEG_W / 2 - 8, SEG_H - 8, (SEG_H - 8) / 2, COL.xSoft, 4)

    self.segLabels = {}
    local items = {{key = "pvp", text = "2 Players", x = -100}, {key = "cpu", text = "vs Computer", x = 100}}
    for _, item in ipairs(items) do
        local sel = (item.key == self.mode)
        local e, t = Label("Segment_" .. item.key, item.text, item.x, SEG_Y, 26, sel and COL.onSurface or COL.onVariant, 6)
        self.segLabels[item.key] = {e = e, t = t, x = item.x}
        local key = item.key
        self:AddButton(item.x, SEG_Y, SEG_W / 2, SEG_H, nil, function(g) g:SetMode(key) end)
    end
    self.thumb:SetPosition(self.mode == "cpu" and 100 or -100, SEG_Y)
end

function Game:BuildChips()
    self.chips = {}
    for k = 1, 3 do
        local x = (k - 2) * 208
        local e, v = RoundRect("Chip" .. k, x, CHIP_Y, 196, 66, 33, COL.chip, 3)
        local t, tt = Label("ChipText" .. k, "", 0, 0, 30, COL.onVariant, 4, e)
        self.chips[k] = {e = e, v = v, t = t, tt = tt, x = x}
    end
end

function Game:BuildBoard()
    -- Fake elevation: three soft, growing, translucent layers under the card.
    self.boardParts = {}
    local alphas = {16, 10, 6}
    for k = 1, 3 do
        local e = RoundRect("Shadow" .. k, 0, BOARD_Y - 8 - k * 5, CARD + k * 10, CARD + k * 10, 50 + k * 4, COL.shadow, 0, alphas[k])
        self.boardParts[#self.boardParts + 1] = e
    end
    local card = RoundRect("Card", 0, BOARD_Y, CARD, CARD, 48, COL.card, 1)
    self.boardParts[#self.boardParts + 1] = card

    self.cellEntities = {}
    for r = 1, 3 do
        for c = 1, 3 do
            local i = (r - 1) * 3 + c
            local x = (c - 2) * STEP
            local y = BOARD_Y + (2 - r) * STEP
            local e, v = RoundRect("Cell" .. i, x, y, CELL, CELL, 30, COL.cell, 2)
            self.cells[i] = {e = e, v = v, x = x, y = y}
            self.cellEntities[#self.cellEntities + 1] = e
        end
    end

    -- Ghost mark that follows the cursor, like a hover state.
    self.ghost = StrokeShape("Ghost", 0, 0, 4, nil, Path.Circle(46), COL.x, 22)
    self.ghostVec = self.ghost:Get("Vector")
    self.ghost:SetScale(0, 0)
end

function Game:BuildFooter()
    -- Turn icon (morphs X -> O -> star -> equals) and status text.
    self.ind = StrokeShape("TurnIcon", -170, STATUS_Y, 6, nil, XPath(13), COL.x, 8)
    self.indVec = self.ind:Get("Vector")
    self.status, self.statusText = Label("Status", "", -135, STATUS_Y, 36, COL.x, 6, nil, 0)

    -- Filled button.
    self.btn, self.btnVec = RoundRect("NewGameButton", 0, BTN_Y, 280, 64, 32, COL.x, 3)
    self.btnLabel, self.btnText = Label("NewGameText", "New game", 0, 0, 30, COL.white, 4, self.btn)
    self:AddButton(0, BTN_Y, 280, 64, self.btn, function(g)
        g:NewRound()
        Tween.Sequence({
            Tween.To(g.btn, {scale = {0.92, 0.92}}, 0.07, "easeOut"),
            Tween.To(g.btn, {scale = {1.05, 1.05}}, 0.4, "elasticOut"),
        })
    end, 1.05)

    self.hintLabel = Label("Hint", "R  new round      M  switch mode", 0, HINT_Y, 22, COL.hint, 6)
end

-- Staggered entrance, like a Flutter page transition.
function Game:PlayIntro()
    Tween.From(self.title, {y = "+=40", alpha = 0}, 0.6, "easeOutCubic")

    for _, e in ipairs({self.segBg, self.thumb, self.segLabels.pvp.e, self.segLabels.cpu.e}) do
        Tween.From(e, {y = "-=30"}, 0.55, "easeOutCubic")
    end

    local chipEntities = {}
    for k = 1, 3 do
        self.chips[k].e:SetScale(0, 0)
        chipEntities[k] = self.chips[k].e
    end
    Tween.Stagger(chipEntities, {scale = {1, 1}}, 0.45, "easeOutBack", 0.09, {delay = 0.2})

    for _, e in ipairs(self.boardParts) do
        Tween.From(e, {scale = {0.8, 0.8}}, 0.55, "easeOutCubic")
    end

    for _, e in ipairs(self.cellEntities) do e:SetScale(0, 0) end
    Tween.Stagger(self.cellEntities, {scale = {1, 1}}, 0.45, "easeOutBack", 0.05, {delay = 0.35})

    self.btn:SetScale(0, 0)
    Tween.To(self.btn, {scale = {1, 1}}, 0.5, "easeOutBack", {delay = 0.95})
    Tween.From(self.status, {y = "-=20", alpha = 0}, 0.5, "easeOutCubic")
end

---------------------------------------------------------------------------
-- Buttons (hit test + implicit hover animation)
---------------------------------------------------------------------------
function Game:AddButton(x, y, w, h, e, fn, hoverScale)
    self.buttons[#self.buttons + 1] = {x = x, y = y, w = w, h = h, e = e, fn = fn, hs = hoverScale, hot = false}
end

function Game:UpdateButtons(mx, my, inView, click)
    local consumed = false
    for _, b in ipairs(self.buttons) do
        local over = inView and math.abs(mx - b.x) <= b.w / 2 and math.abs(my - b.y) <= b.h / 2
        if over ~= b.hot then
            b.hot = over
            if b.hs and b.e:IsValid() then
                Tween.To(b.e, {scale = over and {b.hs, b.hs} or {1, 1}}, 0.2, "easeOutCubic")
            end
        end
        if over and click then
            consumed = true
            if self.lock <= 0 then b.fn(self) end
        end
    end
    return consumed
end

---------------------------------------------------------------------------
-- Frame update
---------------------------------------------------------------------------
function Game:HumanTurn()
    return self.mode == "pvp" or self.turn == 1
end

function Game:CellAt(mx, my)
    for i = 1, 9 do
        local c = self.cells[i]
        if math.abs(mx - c.x) <= CELL / 2 and math.abs(my - c.y) <= CELL / 2 then
            return i
        end
    end
    return nil
end

function Game:InCard(mx, my)
    return math.abs(mx) <= CARD / 2 and math.abs(my - BOARD_Y) <= CARD / 2
end

function Game:OnUpdate(dt)
    if self.lock > 0 then self.lock = self.lock - dt end

    local mx, my = self.entity:GetMouseWorld()
    local inView = Input.IsMouseInViewport()
    local click = inView and Input.IsMouseJustPressed(Key.MOUSE_LEFT)

    if self.lock <= 0 then
        if Input.IsKeyJustPressed(Key.R) then self:NewRound() end
        if Input.IsKeyJustPressed(Key.M) then
            self:SetMode(self.mode == "cpu" and "pvp" or "cpu")
        end
    end

    -- Hover state of the cell under the cursor.
    local want = nil
    if self.state == "playing" and self.lock <= 0 and self:HumanTurn() and inView then
        local i = self:CellAt(mx, my)
        if i and self.board[i] == 0 then want = i end
    end
    self:SetHover(want)

    local consumed = self:UpdateButtons(mx, my, inView, click)

    -- Computer "thinks" for a moment before it moves.
    if self.cpuWait then
        self.cpuWait = self.cpuWait - dt
        if self.cpuWait <= 0 and self.lock <= 0 then
            self.cpuWait = nil
            self:CpuMove()
        end
    end

    if click and not consumed and self.lock <= 0 then
        if self.state == "playing" then
            if want and self:CellAt(mx, my) == want then self:Place(want, self.turn) end
        elseif self:InCard(mx, my) then
            self:NewRound()
        end
    end
end

-- Soft floating circles behind everything (CustomPainter).
function Game:OnRender()
    local t = Time.Now()
    Draw.SetZ(-5)
    Draw.Circle(-720 + math.sin(t * 0.35) * 24, 300 + math.cos(t * 0.30) * 20, 300, 103, 80, 164, 22)
    Draw.Circle(760 + math.cos(t * 0.30) * 26, -280 + math.sin(t * 0.40) * 22, 340, 0, 137, 123, 20)
    Draw.Ring(-640 + math.cos(t * 0.25) * 16, -380 + math.sin(t * 0.30) * 16, 150, 14, 103, 80, 164, 34)
    Draw.Ring(650 + math.sin(t * 0.25) * 16, 380 + math.cos(t * 0.30) * 16, 120, 12, 0, 137, 123, 34)
end

---------------------------------------------------------------------------
-- Hover + ghost
---------------------------------------------------------------------------
function Game:SetHover(i)
    if self.hover == i then return end
    local old = self.hover
    self.hover = i

    if old then
        local c = self.cells[old]
        Tween.To(c.e, {scale = {1, 1}, fillColor = COL.cell}, 0.22, "easeOutCubic")
    end

    if i then
        local c = self.cells[i]
        Tween.To(c.e, {scale = {1.04, 1.04}, fillColor = COL.cellHover}, 0.22, "easeOutBack")
        self:ShowGhost(i)
    else
        self:HideGhost()
    end
end

function Game:ShowGhost(i)
    local c = self.cells[i]
    local col = Strong(self.turn)
    Tween.Kill(self.ghost)
    self.ghostVec:SetPath(self.turn == 1 and XPath(42) or Path.Circle(46))
    self.ghostVec:SetStroke(col[1], col[2], col[3], 70)
    self.ghost:SetPosition(c.x, c.y)
    self.ghost:SetScale(0.6, 0.6)
    Tween.To(self.ghost, {scale = {1, 1}}, 0.25, "easeOutBack")
end

function Game:HideGhost()
    Tween.Kill(self.ghost)
    self.ghost:SetScale(0, 0)
end

---------------------------------------------------------------------------
-- Moves
---------------------------------------------------------------------------
function Game:Full()
    for i = 1, 9 do
        if self.board[i] == 0 then return false end
    end
    return true
end

function Game:Place(i, p)
    self:SetHover(nil)
    self.board[i] = p
    self:Ripple(i, p)
    self:SpawnMark(i, p)

    local w, line = Winner(self.board)
    if w ~= 0 then
        self:Finish(w, line)
    elseif self:Full() then
        self:Finish(0, nil)
    else
        self.turn = 3 - p
        self:RefreshTurn()
    end
end

function Game:CpuMove()
    if self.state ~= "playing" then return end
    local empties = {}
    for i = 1, 9 do
        if self.board[i] == 0 then empties[#empties + 1] = i end
    end
    if #empties == 0 then return end

    local pick
    if #empties == 9 then
        pick = ({1, 3, 5, 7, 9})[math.random(5)]
    elseif math.random() < MISTAKE then
        pick = empties[math.random(#empties)]
    else
        local best, moves = -100, {}
        for _, i in ipairs(empties) do
            self.board[i] = 2
            local s = Minimax(self.board, 1, 2, 1)
            self.board[i] = 0
            if s > best then
                best, moves = s, {i}
            elseif s == best then
                moves[#moves + 1] = i
            end
        end
        pick = moves[math.random(#moves)]
    end
    self:Place(pick, 2)
end

-- Material ink splash: the cell color grows and fades.
function Game:Ripple(i, p)
    local c = self.cells[i]
    local col = Strong(p)
    local e = RoundRect("Ink", c.x, c.y, CELL, CELL, 30, col, 3, 90)
    e:SetScale(0.3, 0.3)
    Tween.To(e, {scale = {1, 1}}, 0.4, "easeOutCubic")
    Tween.To(e, {alpha = 0}, 0.4, "easeOut", {onComplete = function() Dispose(e) end})
end

-- X draws its two strokes one after the other. O draws itself around while turning.
function Game:SpawnMark(i, p)
    local c = self.cells[i]
    local col = Strong(p)
    local m = Spawn("Mark" .. i, c.x, c.y, 5)
    m:SetScale(0.55, 0.55)
    self.marks[i] = m

    if p == 1 then
        local a, av = StrokeShape("XStroke1", 0, 0, 5, m, Path.Line(-42, 42, 42, -42), col, 22)
        local b, bv = StrokeShape("XStroke2", 0, 0, 5, m, Path.Line(-42, -42, 42, 42), col, 22)
        av:SetTrim(0, 0)
        bv:SetTrim(0.02, 0) -- start past the end: fully hidden until its turn
        Tween.To(a, {trimEnd = 1}, 0.18, "easeOutCubic")
        Tween.To(b, {trimEnd = 1}, 0.18, "easeOutCubic", {
            delay = 0.16,
            onStart = function()
                if b:IsValid() then bv:SetTrim(0, 0) end
            end,
        })
    else
        local a, av = StrokeShape("OStroke", 0, 0, 5, m, Path.Circle(46), col, 22)
        av:SetTrim(0, 0)
        Tween.To(a, {trimEnd = 1}, 0.38, "easeInOutCubic")
        Tween.From(a, {rotation = -120}, 0.38, "easeOutCubic")
    end

    Tween.To(m, {scale = {1, 1}}, 0.4, "easeOutBack")
end

---------------------------------------------------------------------------
-- Round end
---------------------------------------------------------------------------
function Game:Finish(winner, line)
    self.cpuWait = nil
    self.lock = 0.6

    if winner ~= 0 then
        self.state = "won"
        self.winner = winner
        self.scores[winner] = self.scores[winner] + 1
        self:ShowWinLine(winner, line)
        self:Confetti()
    else
        self.state = "draw"
        self.winner = nil
        self.scores[3] = self.scores[3] + 1
        for i = 1, 9 do
            local m = self.marks[i]
            if m and m:IsValid() then
                Tween.To(m, {rotation = "+=12"}, 0.6, Curves.Sine(2), {delay = 0.35 + (i - 1) * 0.03})
            end
        end
    end

    self:RefreshScores(winner ~= 0 and winner or 3)
    self:RefreshTurn()
end

function Game:ShowWinLine(winner, line)
    local a, b = self.cells[line[1]], self.cells[line[3]]
    local len = Math.Length(b.x - a.x, b.y - a.y)
    local ux, uy = (b.x - a.x) / len, (b.y - a.y) / len
    local ext = 62
    local path = Path.Line(a.x - ux * ext, a.y - uy * ext, b.x + ux * ext, b.y + uy * ext)

    -- White halo under a colored line so it stays visible on top of same-colored marks.
    local halo, hv = StrokeShape("WinHalo", 0, 0, 11, nil, path, COL.white, 42)
    local bar, bv = StrokeShape("WinLine", 0, 0, 12, nil, path:Copy(), Strong(winner), 24)
    hv:SetTrim(0, 0)
    bv:SetTrim(0, 0)
    Tween.To(halo, {trimEnd = 1}, 0.55, "easeInOutCubic", {delay = 0.25})
    Tween.To(bar, {trimEnd = 1}, 0.55, "easeInOutCubic", {delay = 0.25})
    self.winLine = {halo, bar}

    for k = 1, 3 do
        local idx = line[k]
        Tween.To(self.cells[idx].e, {fillColor = Soft(winner), scale = {1, 1}}, 0.3, "easeOut", {delay = 0.2 + (k - 1) * 0.1})
        local m = self.marks[idx]
        if m and m:IsValid() then
            Tween.To(m, {scale = {1.22, 1.22}}, 0.5, Curves.Sine(0.5), {delay = 0.45 + (k - 1) * 0.12})
        end
    end
end

-- Confetti flying along cubic curves (Tween.Along), spinning as they go.
function Game:Confetti()
    local palette = {COL.x, COL.o, COL.amber, COL.pink, COL.blue}
    for _ = 1, 40 do
        local piece = Spawn("Confetti", 0, BOARD_Y, 25)
        local v = piece:Add("Vector")
        v:SetRect(10, 18, 3)
        local c = palette[math.random(#palette)]
        v:SetFill(c[1], c[2], c[3])
        v:SetStrokeEnabled(false)

        local side = math.random() * 2 - 1
        local rise = 380 + math.random() * 320
        local reach = 0.6 + math.random() * 0.6
        local path = Path.New():MoveTo(0, BOARD_Y):CubicTo(
            side * 240, BOARD_Y + rise,
            side * 700 * reach, BOARD_Y + rise * 0.45,
            side * 900, -700)
        local dur = 1.5 + math.random() * 0.9
        Tween.Along(piece, path, dur, "easeOutQuad", {onComplete = function() Dispose(piece) end})
        Tween.To(piece, {rotation = math.random(360, 900) * (math.random() < 0.5 and -1 or 1)}, dur, "linear")
    end
end

function Game:NewRound()
    if self.state ~= "playing" then self.starter = 3 - self.starter end
    self.cpuWait = nil
    self.winner = nil
    self.hover = nil
    self:HideGhost()

    for i = 1, 9 do
        local m = self.marks[i]
        self.marks[i] = nil
        if m and m:IsValid() then
            Tween.To(m, {scale = {0, 0}, rotation = "-=45"}, 0.28, "easeInBack", {
                delay = (i - 1) * 0.02,
                onComplete = function() Dispose(m) end,
            })
        end
        self.board[i] = 0
        Tween.To(self.cells[i].e, {scale = {1, 1}, fillColor = COL.cell}, 0.3, "easeOut")
    end

    if self.winLine then
        for _, e in ipairs(self.winLine) do
            if e:IsValid() then
                Tween.To(e, {trimStart = 1}, 0.3, "easeInCubic", {onComplete = function() Dispose(e) end})
            end
        end
        self.winLine = nil
    end

    self.state = "playing"
    self.turn = self.starter
    self.lock = 0.45
    self:RefreshTurn()
end

function Game:SetMode(mode)
    if mode == self.mode then return end
    self.mode = mode

    Tween.Spring(self.thumb, {x = (mode == "cpu") and 100 or -100}, {stiffness = 320, damping = 26})
    for key, lab in pairs(self.segLabels) do
        Tween.To(lab.e, {textColor = (key == mode) and COL.onSurface or COL.onVariant}, 0.2, "easeOut")
    end

    self.scores = {0, 0, 0}
    self.state = "playing" -- a fresh match, so the starter does not flip
    self.starter = 1
    self:NewRound()
    self:RefreshScores()
end

---------------------------------------------------------------------------
-- Labels, chips and status (implicit animations)
---------------------------------------------------------------------------
function Game:RefreshScores(bump)
    local cpu = self.mode == "cpu"
    local names = {cpu and "You" or "X", cpu and "CPU" or "O", "Draws"}
    for k = 1, 3 do
        self.chips[k].tt:SetText(string.format("%s  %d", names[k], self.scores[k]))
    end

    if bump then
        local chip = self.chips[bump]
        Tween.To(chip.e, {scale = {1.14, 1.14}}, 0.45, Curves.Sine(0.5))

        local col = (bump == 3) and COL.onVariant or Strong(bump)
        local plus = Label("Plus", "+1", chip.x, CHIP_Y + 44, 34, col, 20)
        Tween.To(plus, {y = "+=60", alpha = 0}, 0.8, "easeOutCubic", {onComplete = function() Dispose(plus) end})
    end
end

function Game:RefreshChips()
    local active = 0
    if self.state == "playing" then active = self.turn
    elseif self.state == "won" then active = self.winner end

    for k = 1, 3 do
        local chip = self.chips[k]
        local on = (k == active)
        Tween.To(chip.e, {fillColor = on and Soft(k) or COL.chip}, 0.3, "easeOut")
        Tween.To(chip.t, {textColor = on and Strong(k) or COL.onVariant}, 0.3, "easeOut")
    end
end

function Game:RefreshStatus(instant)
    local cpu = self.mode == "cpu"
    local text, col, target

    if self.state == "won" then
        local w = self.winner
        if w == 1 then text = cpu and "You win!" or "X wins!"
        else text = cpu and "CPU wins" or "O wins!" end
        col = Strong(w)
        target = Path.Star(5, 24, 11)
    elseif self.state == "draw" then
        text = "It's a draw"
        col = COL.onVariant
        target = Path.Line(-15, 9, 15, 9):Append(Path.Line(-15, -9, 15, -9))
    else
        if cpu then
            text = (self.turn == 1) and "Your turn" or "CPU is thinking..."
        else
            text = (self.turn == 1) and "X's turn" or "O's turn"
        end
        col = Strong(self.turn)
        target = (self.turn == 1) and XPath(13) or Path.Circle(16)
    end

    self.statusText:SetText(text)
    self.statusText:SetColor(col[1], col[2], col[3], 255)
    self.btnText:SetText(self.state == "playing" and "New game" or "Play again")

    if instant then
        self.indVec:SetPath(target)
        self.indVec:SetStroke(col[1], col[2], col[3])
        return
    end

    -- The icon morphs from whatever it is now into the new shape.
    self.indVec:MorphTo(target, 0.4, "easeInOutCubic")
    Tween.To(self.ind, {strokeColor = col}, 0.3, "easeOut")
    Tween.FromTo(self.status, {y = STATUS_Y - 14, alpha = 0}, {y = STATUS_Y, alpha = 255}, 0.28, "easeOutCubic")
    if self.state == "won" then
        Tween.FromTo(self.ind, {rotation = -180}, {rotation = 0}, 0.7, "easeOutBack")
    end
end

function Game:RefreshTurn(instant)
    self:RefreshChips()
    self:RefreshStatus(instant)
    if self.state == "playing" and self.mode == "cpu" and self.turn == 2 then
        self.cpuWait = 0.55 + math.random() * 0.4
    end
end

return Game
