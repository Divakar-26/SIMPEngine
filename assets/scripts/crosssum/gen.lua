local Gen = {}

local function Rng(seed)
    local s = (seed * 2654435761 + 12345) % 4294967296
    return function(n)
        s = (s * 1664525 + 1013904223) % 4294967296
        return math.floor(s / 4294967296 * n) + 1
    end
end

local function RunLengths(white, n, r, c)
    local h, v = 1, 1
    local x = c - 1
    while x >= 1 and white[r][x] do h = h + 1; x = x - 1 end
    x = c + 1
    while x <= n and white[r][x] do h = h + 1; x = x + 1 end
    x = r - 1
    while x >= 1 and white[x][c] do v = v + 1; x = x - 1 end
    x = r + 1
    while x <= n and white[x][c] do v = v + 1; x = x + 1 end
    return h, v
end

local function Repair(white, n)
    local changed = true
    while changed do
        changed = false
        for r = 1, n do
            for c = 1, n do
                if white[r][c] then
                    local h, v = RunLengths(white, n, r, c)
                    if h < 2 or v < 2 or h > 9 or v > 9 then
                        white[r][c] = false
                        changed = true
                    end
                end
            end
        end
    end
end

local function KeepLargest(white, n)
    local seen, best = {}, {}
    for r = 1, n do seen[r] = {} end
    for r = 1, n do
        for c = 1, n do
            if white[r][c] and not seen[r][c] then
                local comp, stack = {}, {{r, c}}
                seen[r][c] = true
                while #stack > 0 do
                    local p = table.remove(stack)
                    comp[#comp + 1] = p
                    for _, d in ipairs({{1, 0}, {-1, 0}, {0, 1}, {0, -1}}) do
                        local rr, cc = p[1] + d[1], p[2] + d[2]
                        if rr >= 1 and rr <= n and cc >= 1 and cc <= n and white[rr][cc] and not seen[rr][cc] then
                            seen[rr][cc] = true
                            stack[#stack + 1] = {rr, cc}
                        end
                    end
                end
                if #comp > #best then best = comp end
            end
        end
    end
    for r = 1, n do for c = 1, n do white[r][c] = false end end
    for _, p in ipairs(best) do white[p[1]][p[2]] = true end
    return #best
end

local function Layout(n, density, rnd)
    local white = {}
    for r = 1, n do
        white[r] = {}
        for c = 1, n do white[r][c] = true end
    end
    local blocks = math.floor(n * n * density / 2 + 0.5)
    for _ = 1, blocks do
        local r, c = rnd(n), rnd(n)
        white[r][c] = false
        white[n + 1 - r][n + 1 - c] = false
    end
    for _ = 1, 6 do
        Repair(white, n)
        KeepLargest(white, n)
    end
    Repair(white, n)
    local count = 0
    for r = 1, n do for c = 1, n do if white[r][c] then count = count + 1 end end end
    return white, count
end

local function Fill(white, n, rnd)
    local hrun, vrun = {}, {}
    for r = 1, n do hrun[r], vrun[r] = {}, {} end
    local cells = {}
    local hid, vid = 0, 0
    for r = 1, n do
        for c = 1, n do
            if white[r][c] then
                if c > 1 and white[r][c - 1] then hrun[r][c] = hrun[r][c - 1] else hid = hid + 1; hrun[r][c] = hid end
                if r > 1 and white[r - 1][c] then vrun[r][c] = vrun[r - 1][c] else vid = vid + 1; vrun[r][c] = vid end
                cells[#cells + 1] = {r, c}
            end
        end
    end
    local hused, vused = {}, {}
    for i = 1, hid do hused[i] = {} end
    for i = 1, vid do vused[i] = {} end
    local val = {}
    for r = 1, n do val[r] = {} end
    local nodes = 0
    local function Solve(i)
        if i > #cells then return true end
        nodes = nodes + 1
        if nodes > 30000 then return false end
        local r, c = cells[i][1], cells[i][2]
        local hu, vu = hused[hrun[r][c]], vused[vrun[r][c]]
        local order = {1, 2, 3, 4, 5, 6, 7, 8, 9}
        for k = 9, 2, -1 do
            local j = rnd(k)
            order[k], order[j] = order[j], order[k]
        end
        for _, d in ipairs(order) do
            if not hu[d] and not vu[d] then
                hu[d], vu[d] = true, true
                val[r][c] = d
                if Solve(i + 1) then return true end
                hu[d], vu[d] = nil, nil
                val[r][c] = nil
                if nodes > 30000 then return false end
            end
        end
        return false
    end
    if Solve(1) then return val end
    return nil
end

function Gen.Build(n, density, seed, minWhite)
    local attempt = 0
    while true do
        local rnd = Rng(seed + attempt * 7919)
        attempt = attempt + 1
        local white, count = Layout(n, density, rnd)
        if count >= minWhite then
            local val = Fill(white, n, rnd)
            if val then
                local runs = {}
                local clues = {}
                for r = 0, n do clues[r] = {} end
                local function Collect(dir)
                    for a = 1, n do
                        local b = 1
                        while b <= n do
                            local r, c = (dir == "h") and a or b, (dir == "h") and b or a
                            if white[r][c] then
                                local cells, sum = {}, 0
                                while b <= n do
                                    local rr, cc = (dir == "h") and a or b, (dir == "h") and b or a
                                    if not white[rr][cc] then break end
                                    cells[#cells + 1] = {rr, cc}
                                    sum = sum + val[rr][cc]
                                    b = b + 1
                                end
                                local cr, cc = (dir == "h") and r or r - 1, (dir == "h") and c - 1 or c
                                local run = {dir = dir, cells = cells, sum = sum, cr = cr, cc = cc}
                                runs[#runs + 1] = run
                                clues[cr][cc] = clues[cr][cc] or {}
                                clues[cr][cc][dir == "h" and "right" or "down"] = sum
                            else
                                b = b + 1
                            end
                        end
                    end
                end
                Collect("h")
                Collect("v")
                return {n = n, white = white, solution = val, runs = runs, clues = clues, count = count}
            end
        end
    end
end

function Gen.Combos(len, sum)
    local out = {}
    local function Rec(start, left, total, acc)
        if left == 0 then
            if total == 0 then out[#out + 1] = {table.unpack(acc)} end
            return
        end
        for d = start, 9 do
            if d > total then break end
            acc[#acc + 1] = d
            Rec(d + 1, left - 1, total - d, acc)
            acc[#acc] = nil
        end
    end
    Rec(1, len, sum, {})
    return out
end

return Gen
