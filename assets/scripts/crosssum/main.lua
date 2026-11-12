local Gen = require("crosssum.gen")

local Vars = Game
local CrossSum = {}

local DIFFS = {
    {
        id = "e", name = "EASY", color = "#5eead4",
        desc = "Small boards, short runs, plenty of starting digits",
        sizes = {4, 4, 4, 5, 5, 5, 5, 5, 6, 6, 6, 6},
        density = 0.24, givens = 0.35, hints = 3,
    },
    {
        id = "m", name = "MEDIUM", color = "#ffcf5c",
        desc = "Bigger boards, longer runs, a few starting digits",
        sizes = {6, 6, 6, 6, 7, 7, 7, 7, 7, 8, 8, 8},
        density = 0.18, givens = 0.14, hints = 3,
    },
    {
        id = "h", name = "HARD", color = "#ff7a90",
        desc = "Huge boards, long runs, almost no help",
        sizes = {7, 7, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9},
        density = 0.13, givens = 0.04, hints = 2,
    },
}

local C = {
    bgTop = "#202760",
    bgBottom = "#0b0e26",
    cream = "#f6f2e9",
    creamShade = "#cfc8b6",
    ink = "#20244a",
    clueCell = "#2d3470",
    clueEdge = "#4a54a8",
    void = "#141836",
    right = "#ffd479",
    down = "#7ff0d8",
    gold = "#ffcf5c",
    mint = "#5eead4",
    coral = "#ff7a90",
    violet = "#8b7cff",
    muted = "#8f98d6",
    text = "#eef0ff",
    bad = "#e5484d",
    badTint = "#ffd9dd",
    given = "#dfe5ff",
    runTint = "#ebeeff",
    selTint = "#fff0b0",
    hint = "#2f7bd9",
}

local starCache = {}

local function Star(x, y, r, color, rot)
    local k = math.floor(r * 10)
    if not starCache[k] then
        starCache[k] = Path.Star(5, r, r * 0.46)
    end
    Draw.Path(starCache[k], x, y, {fill = color, stroke = false, rotation = rot or 0})
end

local function Text(s, x, y, size, color, align)
    Draw.Text(s, x, y, {size = size, color = color, align = align or "center", middle = true})
end

local function Format(t)
    local s = math.floor(t)
    return string.format("%d:%02d", math.floor(s / 60), s % 60)
end

local function CopyNotes(n)
    local out = {}
    for k, v in pairs(n) do out[k] = v end
    return out
end

local comboCache = {}

local function Combos(len, sum)
    local key = len * 100 + sum
    if not comboCache[key] then
        comboCache[key] = Gen.Combos(len, sum)
    end
    return comboCache[key]
end

local function Pop(t)
    return Ease.outBack(Math.Clamp01(t))
end

function CrossSum:OnCreate()
    self.t = 0
    self.state = "menu"
    self.diff = 1
    self.level = 1
    self.buttons = {}
    self.hv = {}
    self.fade = 1
    self.pending = nil
    self.confetti = {}
    self.menuT = 0
    self.progress = 0
    self.hintsUsed = 0
    self.noReg = false
    self.keys = {}
    for name, key in pairs({left = "LEFT", right = "RIGHT", up = "UP", down = "DOWN", back = "BACKSPACE",
        del = "DELETE", esc = "ESCAPE", space = "SPACE", enter = "ENTER", n = "N", h = "H", z = "Z"}) do
        self.keys[name] = Key[key]
    end
    self.digitKeys = {}
    for d = 1, 9 do
        self.digitKeys[d] = {Key["NUM_" .. d], Key["KP_" .. d]}
    end
    local camEntity = self.entity:FindByName("Camera")
    self.cam = camEntity and camEntity:GetCamera() or nil
    Vars.Load(1)
    log.info("Cross Sum ready")
end

function CrossSum:StarsOf(diff, level)
    return math.floor(Vars.Get("cs_" .. DIFFS[diff].id .. level, 0))
end

function CrossSum:Unlocked(diff, level)
    return level == 1 or self:StarsOf(diff, level - 1) > 0
end

function CrossSum:Key(name)
    local k = self.keys[name]
    return k and Input.IsKeyJustPressed(k)
end

function CrossSum:Go(fn)
    if self.pending then return end
    self.pending = fn
    self.fadeOut = true
end

function CrossSum:StartLevel(diff, level)
    local d = DIFFS[diff]
    local n = d.sizes[level]
    local seed = diff * 10007 + level * 131 + 17
    local p = Gen.Build(n, d.density, seed, math.floor(n * n * 0.55))
    self.diff, self.level, self.puzzle = diff, level, p
    self.n = n
    self.cells = {}
    local rnd = seed
    local function Rand()
        rnd = (rnd * 1103515245 + 12345) % 2147483648
        return rnd / 2147483648
    end
    for r = 1, n do
        self.cells[r] = {}
        for c = 1, n do
            if p.white[r][c] then
                self.cells[r][c] = {r = r, c = c, notes = {}, t0 = self.t + 0.25 + (r + c) * 0.045,
                    sol = p.solution[r][c], bad = false}
            end
        end
    end
    for _, run in ipairs(p.runs) do
        run.done = false
        for _, q in ipairs(run.cells) do
            local cell = self.cells[q[1]][q[2]]
            if run.dir == "h" then cell.hrun = run else cell.vrun = run end
        end
    end
    for r = 1, n do
        for c = 1, n do
            local cell = self.cells[r][c]
            if cell and Rand() < d.givens then
                cell.v = cell.sol
                cell.given = true
            end
        end
    end
    self.sel = nil
    for r = 1, n do
        for c = 1, n do
            local cell = self.cells[r][c]
            if cell and not cell.given and not self.sel then self.sel = cell end
        end
    end
    self.undo = {}
    self.noteMode = false
    self.hintsLeft = d.hints
    self.hintsUsed = 0
    self.timer = 0
    self.state = "play"
    self.progress = 0
    self.confetti = {}
    self.levelT = self.t
    self:Validate(true)
end

function CrossSum:Validate(silent)
    local p = self.puzzle
    for r = 1, self.n do
        for c = 1, self.n do
            local cell = self.cells[r][c]
            if cell then cell.wasBad = cell.bad; cell.bad = false end
        end
    end
    local allDone = true
    for _, run in ipairs(p.runs) do
        local seen, dup, filled, sum = {}, {}, 0, 0
        for _, q in ipairs(run.cells) do
            local v = self.cells[q[1]][q[2]].v
            if v then
                filled = filled + 1
                sum = sum + v
                if seen[v] then dup[v] = true end
                seen[v] = true
            end
        end
        local full = filled == #run.cells
        local clean = true
        for _, q in ipairs(run.cells) do
            local cell = self.cells[q[1]][q[2]]
            if cell.v and (dup[cell.v] or sum > run.sum or (full and sum ~= run.sum)) then
                cell.bad = true
                clean = false
            end
        end
        local done = full and clean and sum == run.sum
        if done and not run.done and not silent then run.flash = self.t end
        run.done = done
        if not done then allDone = false end
    end
    for r = 1, self.n do
        for c = 1, self.n do
            local cell = self.cells[r][c]
            if cell and cell.bad and not cell.wasBad and not silent then cell.shake = self.t end
        end
    end
    if allDone and not silent and self.state == "play" then self:Win() end
end

function CrossSum:Win()
    self.state = "win"
    self.winT = self.t
    self.stars = math.max(1, 3 - self.hintsUsed)
    local key = "cs_" .. DIFFS[self.diff].id .. self.level
    if self.stars > self:StarsOf(self.diff, self.level) then
        Vars.Set(key, self.stars)
    end
    Vars.Save(1)
    local pal = {C.gold, C.mint, C.coral, C.violet, "#ffffff", "#6bb6ff"}
    for i = 1, 140 do
        local a = math.random() * math.pi * 2
        local sp = 250 + math.random() * 650
        self.confetti[i] = {
            x = 0, y = 60, vx = math.cos(a) * sp, vy = math.sin(a) * sp + 350,
            rot = math.random() * 360, vr = (math.random() - 0.5) * 720,
            col = pal[math.random(#pal)], size = 8 + math.random() * 10, life = 3 + math.random() * 2,
        }
    end
end

function CrossSum:Select(cell)
    if cell then self.sel = cell; self.selT = self.t end
end

function CrossSum:Place(d)
    local cell = self.sel
    if not cell or cell.given or self.state ~= "play" then return end
    if self.noteMode then
        cell.notes[d] = not cell.notes[d] or nil
        cell.noteT = self.t
        return
    end
    self.undo[#self.undo + 1] = {cell = cell, v = cell.v, notes = CopyNotes(cell.notes), hint = cell.hinted}
    if cell.v == d then
        cell.v = nil
    else
        cell.v = d
        cell.hinted = false
        cell.pop = self.t
    end
    cell.notes = {}
    self:Validate()
end

function CrossSum:Erase()
    local cell = self.sel
    if not cell or cell.given or self.state ~= "play" then return end
    if cell.v == nil and next(cell.notes) == nil then return end
    self.undo[#self.undo + 1] = {cell = cell, v = cell.v, notes = CopyNotes(cell.notes), hint = cell.hinted}
    cell.v = nil
    cell.notes = {}
    self:Validate()
end

function CrossSum:Undo()
    local u = table.remove(self.undo)
    if not u or self.state ~= "play" then return end
    u.cell.v = u.v
    u.cell.notes = u.notes
    u.cell.hinted = u.hint
    u.cell.pop = self.t
    self.sel = u.cell
    self:Validate(true)
end

function CrossSum:Hint()
    if self.hintsLeft <= 0 or self.state ~= "play" then return end
    local target = self.sel
    if not target or target.given or target.v == target.sol then
        target = nil
        local options = {}
        for r = 1, self.n do
            for c = 1, self.n do
                local cell = self.cells[r][c]
                if cell and not cell.given and cell.v ~= cell.sol then options[#options + 1] = cell end
            end
        end
        if #options == 0 then return end
        target = options[math.random(#options)]
    end
    self.undo[#self.undo + 1] = {cell = target, v = target.v, notes = CopyNotes(target.notes), hint = target.hinted}
    target.v = target.sol
    target.hinted = true
    target.notes = {}
    target.pop = self.t
    target.glow = self.t
    self.sel = target
    self.hintsLeft = self.hintsLeft - 1
    self.hintsUsed = self.hintsUsed + 1
    self:Validate()
end

function CrossSum:Move(dr, dc)
    if not self.sel then return end
    local r, c = self.sel.r, self.sel.c
    for _ = 1, self.n do
        r, c = r + dr, c + dc
        if r < 1 or r > self.n or c < 1 or c > self.n then return end
        if self.cells[r][c] then self:Select(self.cells[r][c]); return end
    end
end

function CrossSum:BoardGeometry()
    local n = self.n or 5
    local cs = math.min(112, 690 / (n + 1))
    local w = (n + 1) * cs
    local cx, cy = -270, -35
    return cs, cx - w / 2, cy + w / 2
end

function CrossSum:CellCenter(r, c)
    local cs, bx, by = self:BoardGeometry()
    return bx + c * cs + cs / 2, by - r * cs - cs / 2, cs
end

function CrossSum:OnButton(id)
    if id == "menu" then
        self:Go(function() self.state = "menu"; self.menuT = self.t end)
    elseif id == "undo" then
        self:Undo()
    elseif id == "erase" then
        self:Erase()
    elseif id == "notes" then
        self.noteMode = not self.noteMode
    elseif id == "hint" then
        self:Hint()
    elseif id == "next" then
        local d, l = self.diff, self.level
        if l < 12 then
            self:Go(function() self:StartLevel(d, l + 1) end)
        else
            self:Go(function() self.state = "menu"; self.menuT = self.t end)
        end
    elseif id == "replay" then
        local d, l = self.diff, self.level
        self:Go(function() self:StartLevel(d, l) end)
    elseif id:sub(1, 5) == "diff:" then
        self.diff = tonumber(id:sub(6))
        self.menuT = self.t
    elseif id:sub(1, 4) == "lvl:" then
        local l = tonumber(id:sub(5))
        local d = self.diff
        if self:Unlocked(d, l) then
            self:Go(function() self:StartLevel(d, l) end)
        end
    elseif id:sub(1, 4) == "pad:" then
        self:Place(tonumber(id:sub(5)))
    end
end

function CrossSum:OnUpdate(dt)
    self.t = self.t + dt
    if self.cam then
        local w, h = Window.GetSize()
        if w > 0 and h > 0 then
            self.cam:SetZoom(math.min(w / 1600, h / 900))
        end
    end

    if self.pending then
        self.fade = math.min(1, self.fade + dt / 0.22)
        if self.fade >= 1 then
            local fn = self.pending
            self.pending = nil
            fn()
        end
    else
        self.fade = math.max(0, self.fade - dt / 0.3)
    end

    local mx, my = self.entity:GetMouseWorld()
    local inView = Input.IsMouseInViewport()
    self.mx, self.my = mx, my
    local press = inView and Input.IsMouseJustPressed(Key.MOUSE_LEFT) and not self.pending
    local down = inView and Input.IsMouseDown(Key.MOUSE_LEFT)
    self.mouseDown = down

    local hover = nil
    if inView then
        for i = #self.buttons, 1, -1 do
            local b = self.buttons[i]
            if b.enabled and math.abs(mx - b.x) <= b.w / 2 and math.abs(my - b.y) <= b.h / 2 then
                hover = b.id
                break
            end
        end
    end
    self.hover = hover
    for _, b in ipairs(self.buttons) do
        local target = (b.id == hover) and 1 or 0
        self.hv[b.id] = Math.MoveTowards(self.hv[b.id] or 0, target, dt * 9)
    end

    if press and hover then
        self:OnButton(hover)
    elseif press and self.state == "play" then
        for r = 1, self.n do
            for c = 1, self.n do
                local cell = self.cells[r][c]
                if cell then
                    local x, y, cs = self:CellCenter(r, c)
                    if math.abs(mx - x) <= cs / 2 and math.abs(my - y) <= cs / 2 then
                        self:Select(cell)
                    end
                end
            end
        end
    end

    if self.state == "play" then
        self.timer = self.timer + dt
        if self:Key("esc") then self:OnButton("menu") end
        if self:Key("left") then self:Move(0, -1) end
        if self:Key("right") then self:Move(0, 1) end
        if self:Key("up") then self:Move(-1, 0) end
        if self:Key("down") then self:Move(1, 0) end
        for d = 1, 9 do
            local pair = self.digitKeys[d]
            if (pair[1] and Input.IsKeyJustPressed(pair[1])) or (pair[2] and Input.IsKeyJustPressed(pair[2])) then
                self:Place(d)
            end
        end
        if self:Key("back") or self:Key("del") then self:Erase() end
        if self:Key("n") then self.noteMode = not self.noteMode end
        if self:Key("h") then self:Hint() end
        if self:Key("z") then self:Undo() end
        local target = 0
        for _, run in ipairs(self.puzzle.runs) do
            if run.done then target = target + 1 end
        end
        self.progress = Math.MoveTowards(self.progress, target / #self.puzzle.runs, dt * 1.5)
    elseif self.state == "win" then
        if self:Key("enter") or self:Key("space") then self:OnButton("next") end
        if self:Key("esc") then self:OnButton("menu") end
        for _, p in ipairs(self.confetti) do
            p.vy = p.vy - 900 * dt
            p.vx = p.vx * (1 - 0.6 * dt)
            p.x = p.x + p.vx * dt
            p.y = p.y + p.vy * dt
            p.rot = p.rot + p.vr * dt
            p.life = p.life - dt
        end
    end
end

function CrossSum:Btn(id, x, y, w, h, label, opts)
    opts = opts or {}
    local enabled = opts.enabled ~= false
    if not self.noReg then
        self.buttons[#self.buttons + 1] = {id = id, x = x, y = y, w = w, h = h, enabled = enabled}
    end
    local hv = enabled and (self.hv[id] or 0) or 0
    local pressed = enabled and self.hover == id and self.mouseDown and not self.noReg
    local s = (1 + 0.06 * hv) * (pressed and 0.95 or 1) * (opts.scale or 1)
    local bw, bh = w * s, h * s
    local fill = opts.fill or "#2f3880"
    local col = enabled and Color.Mix(fill, "#ffffff", 0.18 * hv) or Color.Mix(fill, C.void, 0.6)
    Draw.RoundRect(x + 2, y - 5, bw, bh, opts.radius or 14, Color.WithAlpha("#000000", 0.35))
    if opts.active then
        Draw.RoundRect(x, y, bw + 8, bh + 8, (opts.radius or 14) + 4, Color.WithAlpha(opts.activeColor or C.gold, 0.9))
    end
    Draw.RoundRect(x, y, bw, bh, opts.radius or 14, col)
    Draw.RoundRect(x, y + bh * 0.2, bw - 8, bh * 0.45, (opts.radius or 14) * 0.8, Color.WithAlpha("#ffffff", 0.08))
    if label then
        Text(label, x, y, opts.size or 26, enabled and (opts.textColor or C.text) or C.muted)
    end
end

function CrossSum:DrawBackground()
    local strips = 28
    for i = 0, strips - 1 do
        local u = i / (strips - 1)
        Draw.Rect(0, 1100 - (i + 0.5) * (2200 / strips), 3600, 2200 / strips + 2, Color.Mix(C.bgTop, C.bgBottom, u))
    end
    for i = 1, 16 do
        local seed = i * 7.31
        local x = ((math.sin(seed) * 1100) + math.sin(self.t * 0.08 + seed) * 120)
        local y = ((math.cos(seed * 1.7) * 600) + math.cos(self.t * 0.11 + seed) * 90)
        local r = 40 + (i % 5) * 28
        local col = (i % 3 == 0) and C.mint or ((i % 3 == 1) and C.violet or C.coral)
        if i % 2 == 0 then
            Draw.Ring(x, y, r, 3, Color.WithAlpha(col, 0.08))
        else
            Draw.Rect(x, y, r * 1.2, r * 1.2, Color.WithAlpha(col, 0.05), self.t * 6 + i * 20)
        end
    end
end

function CrossSum:DrawCell(cell, x, y, cs)
    local t = self.t
    local grow = Pop((t - cell.t0) / 0.45)
    if t < cell.t0 then return end
    local ox = 0
    if cell.shake and t - cell.shake < 0.35 then
        local k = 1 - (t - cell.shake) / 0.35
        ox = math.sin((t - cell.shake) * 60) * 6 * k
    end
    x = x + ox
    local size = (cs - 6) * grow
    local sel = self.sel == cell and self.state == "play"
    local inRun = false
    if self.sel and self.state == "play" and not sel then
        inRun = (cell.hrun == self.sel.hrun) or (cell.vrun == self.sel.vrun)
    end

    Draw.RoundRect(x + 1, y - 4, size, size, 10, Color.WithAlpha("#000000", 0.35))
    if sel then
        local pulse = 0.55 + 0.35 * math.sin(t * 5)
        Draw.RoundRect(x, y, size + 18, size + 18, 16, Color.WithAlpha(C.gold, 0.18 * pulse))
        Draw.RoundRect(x, y, size + 8, size + 8, 13, Color.WithAlpha(C.gold, pulse))
    end

    local fill = C.cream
    if cell.given then fill = C.given end
    if inRun then fill = Color.Mix(fill, "#c9d4ff", 0.7) end
    if sel then fill = C.selTint end
    if cell.bad then fill = Color.Mix(fill, C.badTint, 0.85) end

    local flash = 0
    for _, run in ipairs({cell.hrun, cell.vrun}) do
        if run.flash then
            local idx = 0
            for i, q in ipairs(run.cells) do
                if q[1] == cell.r and q[2] == cell.c then idx = i end
            end
            local u = (t - run.flash - idx * 0.06) / 0.5
            if u > 0 and u < 1 then flash = math.max(flash, math.sin(u * math.pi)) end
        end
    end
    if cell.glow and t - cell.glow < 0.7 then
        flash = math.max(flash, 1 - (t - cell.glow) / 0.7)
    end
    if flash > 0 then fill = Color.Mix(fill, flash > 0 and (cell.hinted and "#b9d6ff" or "#9bf5df") or fill, flash) end

    Draw.RoundRect(x, y, size, size, 10, Color.Mix(fill, C.creamShade, 0.0))
    Draw.RoundRect(x, y - size * 0.34, size - 4, size * 0.26, 8, Color.WithAlpha("#000000", 0.04))

    if cell.v then
        local pop = 1
        if cell.pop then pop = 0.5 + 0.5 * Pop((t - cell.pop) / 0.3) + 0.25 * math.max(0, 1 - (t - cell.pop) / 0.25) end
        local col = C.ink
        if cell.given then col = "#5c6693" end
        if cell.hinted then col = C.hint end
        if cell.bad then col = C.bad end
        Text(tostring(cell.v), x, y + 1, cs * 0.56 * pop * grow, col)
    else
        local any = next(cell.notes) ~= nil
        if any then
            for d = 1, 9 do
                if cell.notes[d] then
                    local nx = x + ((d - 1) % 3 - 1) * cs * 0.24
                    local ny = y + (1 - math.floor((d - 1) / 3)) * cs * 0.22
                    Text(tostring(d), nx, ny, cs * 0.2, "#6a73a8")
                end
            end
        end
    end
end

function CrossSum:DrawClue(r, c, cs, x, y, clue)
    local t = self.t
    local grow = Pop((t - (self.levelT + 0.1 + (r + c) * 0.03)) / 0.45)
    if grow <= 0 then return end
    local size = (cs - 6) * grow
    if not clue then
        Draw.RoundRect(x, y, size, size, 8, Color.WithAlpha(C.void, 0.55))
        return
    end
    Draw.RoundRect(x + 1, y - 4, size, size, 8, Color.WithAlpha("#000000", 0.35))
    Draw.RoundRect(x, y, size, size, 8, C.clueCell)
    Draw.RoundRect(x, y + size * 0.3, size - 4, size * 0.3, 6, Color.WithAlpha("#ffffff", 0.05))
    local h = size / 2 - 4
    Draw.Line(x - h, y + h, x + h, y - h, 2, Color.WithAlpha(C.clueEdge, 0.9))
    local fs = cs * 0.3 * grow
    if clue.right then
        local run
        for _, rr in ipairs(self.puzzle.runs) do
            if rr.dir == "h" and rr.cr == r and rr.cc == c then run = rr end
        end
        local col = C.right
        if run and run.done then col = "#ffffff" end
        Text(tostring(clue.right), x + cs * 0.2, y + cs * 0.2, fs, col)
    end
    if clue.down then
        local run
        for _, rr in ipairs(self.puzzle.runs) do
            if rr.dir == "v" and rr.cr == r and rr.cc == c then run = rr end
        end
        local col = C.down
        if run and run.done then col = "#ffffff" end
        Text(tostring(clue.down), x - cs * 0.2, y - cs * 0.2, fs, col)
    end
end

function CrossSum:DrawBoard()
    local cs, bx, by = self:BoardGeometry()
    local n = self.n
    local w = (n + 1) * cs
    local cx, cy = bx + w / 2, by - w / 2
    local grow = Pop((self.t - self.levelT) / 0.5)
    Draw.RoundRect(cx + 2, cy - 8, (w + 34) * grow, (w + 34) * grow, 30, Color.WithAlpha("#000000", 0.4))
    Draw.RoundRect(cx, cy, (w + 30) * grow, (w + 30) * grow, 28, Color.WithAlpha("#10143a", 0.92))
    Draw.RoundRect(cx, cy, (w + 30) * grow, (w + 30) * grow, 28, Color.WithAlpha("#ffffff", 0.03))
    for r = 0, n do
        for c = 0, n do
            local x, y = bx + c * cs + cs / 2, by - r * cs - cs / 2
            local cell = (r >= 1 and c >= 1) and self.cells[r][c] or nil
            if cell then
                self:DrawCell(cell, x, y, cs)
            else
                self:DrawClue(r, c, cs, x, y, self.puzzle.clues[r] and self.puzzle.clues[r][c])
            end
        end
    end
end

function CrossSum:DrawHelper()
    local x, y, w, h = 560, 225, 420, 270
    Draw.RoundRect(x + 2, y - 6, w, h, 22, Color.WithAlpha("#000000", 0.35))
    Draw.RoundRect(x, y, w, h, 22, "#1a1f4e")
    Draw.RoundRect(x, y + h / 2 - 28, w - 4, 52, 20, Color.WithAlpha("#ffffff", 0.04))
    Text("SUM HELPER", x - w / 2 + 28, y + h / 2 - 28, 22, C.muted, "left")
    local sel = self.sel
    if not sel then
        Text("Select a cell", x, y, 24, C.muted)
        return
    end
    local ly = y + h / 2 - 78
    for i, info in ipairs({{sel.hrun, "ACROSS", C.right}, {sel.vrun, "DOWN", C.down}}) do
        local run, label, col = info[1], info[2], info[3]
        local placed = {}
        for _, q in ipairs(run.cells) do
            local v = self.cells[q[1]][q[2]].v
            if v then placed[v] = true end
        end
        local all = Combos(#run.cells, run.sum)
        local list = {}
        for _, combo in ipairs(all) do
            local ok = true
            for d in pairs(placed) do
                local has = false
                for _, v in ipairs(combo) do if v == d then has = true end end
                if not has then ok = false break end
            end
            if ok then list[#list + 1] = combo end
        end
        Text(label .. "  " .. run.sum .. "  in " .. #run.cells, x - w / 2 + 28, ly, 24, col, "left")
        local shown = math.min(#list, 6)
        for k = 1, shown do
            local cx = x - w / 2 + 28 + ((k - 1) % 3) * 128
            local cy = ly - 34 - math.floor((k - 1) / 3) * 30
            Text(table.concat(list[k], " "), cx, cy, 22, C.text, "left")
        end
        if #list > shown then
            Text("+" .. (#list - shown) .. " more", x + w / 2 - 28, ly, 20, C.muted, "right")
        elseif #list == 0 then
            Text("no combination fits", x + w / 2 - 28, ly, 20, C.coral, "right")
        end
        ly = ly - 122
    end
end

function CrossSum:DrawPlay()
    local d = DIFFS[self.diff]
    local t = self.t
    self:Btn("menu", -690, 395, 140, 52, "MENU", {size = 24})
    Text(d.name .. "   LEVEL " .. self.level, -270, 398, 30, d.color)
    Text(Format(self.timer), 690, 398, 34, C.text, "right")
    Text("TIME", 590, 398, 20, C.muted, "right")

    local bw = 640
    Draw.RoundRect(-270, 352, bw, 12, 6, Color.WithAlpha("#000000", 0.35))
    local fw = math.max(12, bw * self.progress)
    Draw.RoundRect(-270 - bw / 2 + fw / 2, 352, fw, 12, 6, Color.Mix(d.color, "#ffffff", 0.15))

    self:DrawBoard()
    self:DrawHelper()

    for k = 1, 9 do
        local col, row = (k - 1) % 3, math.floor((k - 1) / 3)
        local px, py = 476 + col * 84, 40 - row * 84
        local appear = Pop((t - self.levelT - 0.4 - k * 0.04) / 0.4)
        self:Btn("pad:" .. k, px, py, 76 * appear, 76 * appear, tostring(k),
            {size = 38, fill = self.noteMode and "#3b3f94" or "#2f3880", radius = 18})
    end
    local acts = {
        {"undo", "Undo", #self.undo > 0},
        {"erase", "Erase", true},
        {"notes", self.noteMode and "Notes ON" or "Notes", true},
        {"hint", "Hint " .. self.hintsLeft, self.hintsLeft > 0},
    }
    for i, a in ipairs(acts) do
        self:Btn(a[1], 404 + (i - 1) * 104, -230, 98, 58, a[2], {
            size = 21, enabled = a[3], fill = a[1] == "hint" and "#2a6bb8" or "#2f3880",
            active = a[1] == "notes" and self.noteMode, activeColor = C.mint,
        })
    end
    Text("1-9 enter   Backspace erase   N notes   H hint   Z undo   Arrows move", 560, -310, 18, C.muted)
    Text("Every run must add up to its clue. A digit can only appear once per run.", 560, -340, 18, C.muted)
end

function CrossSum:DrawMenu()
    local t = self.t
    local mt = self.menuT
    local title = "CROSS SUM"
    local size = 120
    local gap = 6
    local widths, total = {}, 0
    for i = 1, #title do
        local ch = title:sub(i, i)
        local cw = ch == " " and 40 or select(1, Draw.MeasureText(ch, size))
        widths[i] = cw
        total = total + cw + gap
    end
    local x = -total / 2
    local cols = {C.coral, C.gold, C.mint, C.violet, C.coral, C.gold, C.mint, C.violet, C.coral}
    for i = 1, #title do
        local ch = title:sub(i, i)
        local appear = Pop((t - 0.1 - i * 0.06) / 0.55)
        local bob = math.sin(t * 2 + i * 0.7) * 8
        if ch ~= " " then
            Text(ch, x + widths[i] / 2 + 4, 290 + bob - 6, size * appear, Color.WithAlpha("#000000", 0.35))
            Text(ch, x + widths[i] / 2, 296 + bob, size * appear, cols[i])
        end
        x = x + widths[i] + gap
    end
    Text("Make every run add up", 0, 205, 30, C.muted)

    local tabs = {-270, 0, 270}
    for i, d in ipairs(DIFFS) do
        local appear = Pop((t - 0.5 - i * 0.08) / 0.45)
        self:Btn("diff:" .. i, tabs[i], 120, 240 * appear, 70 * appear, d.name, {
            size = 30, fill = self.diff == i and Color.Mix(d.color, "#0b0e26", 0.55) or "#262c6b",
            textColor = self.diff == i and d.color or C.muted, active = self.diff == i, activeColor = d.color,
        })
    end
    local d = DIFFS[self.diff]
    Text(d.desc, 0, 60, 24, C.text)

    local total, got = 0, 0
    for l = 1, 12 do
        local col, row = (l - 1) % 6, math.floor((l - 1) / 6)
        local bx, by = -350 + col * 140, -30 - row * 150
        local stars = self:StarsOf(self.diff, l)
        local unlocked = self:Unlocked(self.diff, l)
        local appear = Pop((t - mt - 0.1 - l * 0.035) / 0.4)
        local fill = stars > 0 and Color.Mix(d.color, "#0b0e26", 0.55) or "#262c6b"
        self:Btn("lvl:" .. l, bx, by, 116 * appear, 124 * appear, nil, {
            fill = fill, enabled = unlocked, radius = 20, scale = 1,
        })
        if appear > 0.05 then
            if unlocked then
                Text(tostring(l), bx, by + 18 * appear, 44 * appear, stars > 0 and d.color or C.text)
                for s = 1, 3 do
                    Star(bx + (s - 2) * 28, by - 32 * appear, 11 * appear,
                        s <= stars and C.gold or Color.WithAlpha("#ffffff", 0.14))
                end
            else
                Draw.RoundRect(bx, by - 8, 36 * appear, 28 * appear, 6, C.muted)
                Draw.Ring(bx, by + 14 * appear, 12 * appear, 4 * appear, C.muted)
                Draw.RoundRect(bx, by - 8, 6 * appear, 12 * appear, 3, "#262c6b")
            end
        end
        total = total + 3
        got = got + stars
    end
    Star(-20, -370, 16, C.gold, math.sin(t * 2) * 8)
    Text(got .. " / " .. total, 22, -370, 28, C.text, "left")
    Text("Click a level to play   |   levels unlock in order", 0, -415, 18, C.muted)
end

function CrossSum:DrawWin()
    local t = self.t - self.winT
    local a = Math.Clamp01(t / 0.4)
    Draw.Rect(0, 0, 3600, 2200, Color.WithAlpha("#05071a", 0.72 * a))
    for _, p in ipairs(self.confetti) do
        if p.life > 0 then
            Draw.Rect(p.x, p.y, p.size, p.size * 0.5, Color.WithAlpha(p.col, Math.Clamp01(p.life)), p.rot)
        end
    end
    local s = Pop((t - 0.1) / 0.5)
    local cx, cy = 0, 30
    local d = DIFFS[self.diff]
    Draw.RoundRect(cx + 3, cy - 10, 640 * s, 470 * s, 36, Color.WithAlpha("#000000", 0.5))
    Draw.RoundRect(cx, cy, 640 * s, 470 * s, 36, "#1b2056")
    Draw.RoundRect(cx, cy + 235 * s - 8, 640 * s, 16 * s, 8, d.color)
    Text("SOLVED!", cx, cy + 150 * s, 84 * s, C.text)
    Text(d.name .. "  LEVEL " .. self.level, cx, cy + 80 * s, 26 * s, d.color)
    for i = 1, 3 do
        local sp = Pop((t - 0.4 - i * 0.18) / 0.5)
        local on = i <= self.stars
        local sx = cx + (i - 2) * 100
        if on then
            Draw.Circle(sx, cy + 10, 52 * sp, Color.WithAlpha(C.gold, 0.12))
            Star(sx, cy + 10, 46 * sp, C.gold, (1 - sp) * 90)
        else
            Star(sx, cy + 10, 40, Color.WithAlpha("#ffffff", 0.12))
        end
    end
    Text("Time " .. Format(self.timer) .. "      Hints used " .. self.hintsUsed, cx, cy - 70, 26 * s, C.muted)
    self.noReg = false
    local last = self.level >= 12
    self:Btn("menu", cx - 190, cy - 160, 170 * s, 66 * s, "MENU", {size = 26})
    self:Btn("replay", cx, cy - 160, 150 * s, 66 * s, "AGAIN", {size = 26})
    self:Btn("next", cx + 190, cy - 160, 170 * s, 66 * s, last and "DONE" or "NEXT", {
        size = 28, fill = Color.Mix(d.color, "#0b0e26", 0.4), textColor = "#ffffff", active = true, activeColor = d.color,
    })
end

function CrossSum:OnRender()
    self.buttons = {}
    self:DrawBackground()
    if self.state == "menu" then
        self:DrawMenu()
    else
        self.noReg = self.state == "win"
        self:DrawPlay()
        if self.state == "win" then
            self.noReg = false
            self:DrawWin()
        end
    end
    if self.fade > 0.001 then
        Draw.Rect(0, 0, 3600, 2200, Color.WithAlpha("#05071a", self.fade))
    end
end

return CrossSum
