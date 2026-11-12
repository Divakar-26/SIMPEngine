-- =========================================================================
-- PHYSICS SANDBOX — single file build
-- Consolidated from: sandbox/{main, world, ui, scenes}.lua
-- Button & body events now polled directly (no external scripts needed).
-- Drop this on any entity in a scene with a Camera. Press Play.
-- =========================================================================

-- =========================================================================
-- Utilities
-- =========================================================================
local unpack = table.unpack or unpack
local function clamp(v, a, b) if v < a then return a elseif v > b then return b end return v end
local function rot(x, y, deg)
    local a = math.rad(deg)
    local c, s = math.cos(a), math.sin(a)
    return x * c - y * s, x * s + y * c
end
local function dist(x1, y1, x2, y2) return math.sqrt((x2 - x1) ^ 2 + (y2 - y1) ^ 2) end
local function round(v) return math.floor(v + 0.5) end
local function warn(...) log.warn("[sandbox]", ...) end
local function try(f, ...)
    local ok, err = pcall(f, ...)
    if not ok then warn(tostring(err)) end
    return ok, err
end
local function flat(pts)
    local out = {}
    for _, p in ipairs(pts) do out[#out + 1] = p[1]; out[#out + 1] = p[2] end
    return out
end

-- =========================================================================
-- WORLD
-- =========================================================================
local World = {}
World.__index = World

World.ARENA = { x0 = -720, x1 = 580, y0 = -440, y1 = 430, wall = 80 }
World.CONFIG = { pointAnchors = false, maxBodies = 450 }
World.PALETTE = {
    { "Coral",  { 255, 107, 107 } },
    { "Amber",  { 255, 168,  67 } },
    { "Sun",    { 254, 211,  91 } },
    { "Mint",   {  46, 213, 140 } },
    { "Aqua",   {  52, 220, 220 } },
    { "Sky",    {  78, 162, 255 } },
    { "Violet", { 158, 140, 255 } },
    { "Pink",   { 255, 120, 200 } },
}
World.MASK_MODES = { "Everything", "Same layer only", "Other layers", "Nothing" }
World.KIND_NAMES = { hinge = "Hinge", spring = "Spring", rod = "Rod", weld = "Weld", wheel = "Wheel" }

function World.Shade(c, k)
    local r, g, b
    if k >= 0 then
        r, g, b = c[1] + (255 - c[1]) * k, c[2] + (255 - c[2]) * k, c[3] + (255 - c[3]) * k
    else
        k = -k
        r, g, b = c[1] * (1 - k), c[2] * (1 - k), c[3] * (1 - k)
    end
    return { round(r), round(g), round(b) }
end
local shade = World.Shade

function World.PolyPoints(sides, r)
    local pts = {}
    for i = 0, sides - 1 do
        local a = math.pi / 2 + i * 2 * math.pi / sides
        pts[#pts + 1] = { math.cos(a) * r, math.sin(a) * r }
    end
    return pts
end

function World.Outline(shape, w, h, r, sides, x, y, angle)
    local pts = {}
    angle = angle or 0
    local function add(lx, ly)
        local ox, oy = rot(lx, ly, angle)
        pts[#pts + 1] = x + ox
        pts[#pts + 1] = y + oy
    end
    if shape == "circle" then
        for i = 0, 31 do
            local a = i / 32 * 2 * math.pi
            add(math.cos(a) * r, math.sin(a) * r)
        end
    elseif shape == "polygon" then
        for _, p in ipairs(World.PolyPoints(sides, r)) do add(p[1], p[2]) end
    elseif shape == "capsule" then
        local rad = math.min(w, h) / 2
        local half = math.max(w, h) / 2 - rad
        local vertical = h > w
        local function cap(cx, a0, a1)
            for i = 0, 8 do
                local a = math.rad(a0 + (a1 - a0) * i / 8)
                local lx, ly = cx + math.cos(a) * rad, math.sin(a) * rad
                if vertical then add(ly, lx) else add(lx, ly) end
            end
        end
        cap(half, -90, 90)
        cap(-half, 90, 270)
    else
        local hw, hh = w / 2, h / 2
        add(-hw, -hh); add(hw, -hh); add(hw, hh); add(-hw, hh)
    end
    return pts
end

function World.new(owner)
    local w = setmetatable({}, World)
    w.owner = owner
    w.bodies, w.byName, w.joints = {}, {}, {}
    w.nextBody, w.nextJoint = 1, 1
    w.colorCursor = 0
    return w
end

function World:NextColor()
    self.colorCursor = self.colorCursor % #World.PALETTE + 1
    return self.colorCursor
end

function World:UserBodyCount()
    local n = 0
    for _, r in ipairs(self.bodies) do
        if not r.arena and not r.anchor then n = n + 1 end
    end
    return n
end

function World:Add(shape, x, y, o)
    o = o or {}
    local id = self.nextBody
    self.nextBody = id + 1
    local name = "SB" .. id

    local rec = {
        id = id, name = name, shape = shape,
        w = o.w or 80, h = o.h or 60, r = o.r or 30, sides = o.sides or 6,
        static = o.static or false, mass = o.mass or 1,
        bounce = o.bounce or 0.25, friction = o.friction or 0.5,
        linDamp = o.linDamp, angDamp = o.angDamp,
        sensor = o.sensor or false, lockRot = o.lockRot or false, lockPos = o.lockPos or false,
        noGravity = o.noGravity or false, oneWay = o.oneWay or false,
        canSleep = o.canSleep ~= false,
        layer = o.layer or 0, group = o.group or 0, maskMode = o.maskMode or 1,
        color = o.color or 1, arena = o.arena, anchor = o.anchor, tracer = o.tracer,
        inside = 0, hit = 0, trail = {},
    }
    if rec.static and rec.mass <= 0 then rec.mass = 1 end

    local e = Scene.Spawn(name, x, y)
    e:SetRotation(o.angle or 0)
    e:SetZIndex(rec.arena and -2 or (rec.anchor and 4 or (rec.static and 0 or 1)))
    rec.entity = e
    rec.vec = e:Add("Vector")
    rec.body = e:Add("Body")

    local mass = rec.static and 0 or rec.mass
    local b = rec.body
    if shape == "circle" then
        b:CreateCircle(rec.r, mass)
    elseif shape == "capsule" then
        local rad = math.min(rec.w, rec.h) / 2
        b:CreateCapsule(math.max(1, math.max(rec.w, rec.h) / 2 - rad), rad, mass)
    elseif shape == "polygon" then
        b:CreatePoints(World.PolyPoints(rec.sides, rec.r), 0, mass)
    else
        b:CreateBox(rec.w, rec.h, mass)
    end

    rec.vw, rec.vh = rec.w, rec.h
    if shape == "box" or shape == "capsule" then
        local ok, bw, bh = pcall(b.GetSize, b)
        if ok and bw and bh and bw > 0 and bh > 0 then rec.vw, rec.vh = bw, bh end
    end

    try(b.SetBounce, b, rec.bounce)
    try(b.SetFriction, b, rec.friction, rec.friction)
    if rec.linDamp or rec.angDamp then try(b.SetDamping, b, rec.linDamp or 1, rec.angDamp or 1) end
    if rec.sensor then try(b.SetSensor, b, true) end
    if rec.lockRot or rec.lockPos then try(b.SetLocked, b, rec.lockRot, rec.lockPos) end
    if rec.noGravity then try(b.SetIgnoreGravity, b, true) end
    if rec.oneWay then try(b.SetOneWay, b, true) end
    if not rec.canSleep then try(b.SetCanSleep, b, false) end
    if rec.layer ~= 0 then self:SetLayer(rec, rec.layer) end
    if rec.group ~= 0 then try(b.SetGroup, b, rec.group) end

    self:BuildVisual(rec)
    self.bodies[#self.bodies + 1] = rec
    self.byName[name] = rec
    return rec
end

function World:Anchor(x, y)
    return self:Add("circle", x, y, { r = 6, static = true, anchor = true, sensor = true })
end

function World:BuildVisual(rec)
    local v = rec.vec
    if rec.shape == "circle" then
        v:SetCircle(rec.r)
    elseif rec.shape == "polygon" then
        v:SetPath(Path.Points(flat(World.PolyPoints(rec.sides, rec.r)), true))
    else
        local small = math.min(rec.vw, rec.vh)
        local radius = (rec.shape == "capsule") and small / 2 or math.min(5, small / 4)
        v:SetRect(rec.vw, rec.vh, radius)
    end
    v:SetStrokeWidth(rec.arena and 2 or 3)
    v:SetJoin("round")
    self:Style(rec)
end

function World:BaseColor(rec)
    if rec.anchor then return { 236, 241, 255 } end
    if rec.arena then return { 28, 35, 52 } end
    if rec.static then return { 66, 80, 112 } end
    return World.PALETTE[rec.color or 1][2]
end

function World:Style(rec)
    local v = rec.vec
    local base = self:BaseColor(rec)
    if rec.inside > 0 then base = shade(base, 0.25) end
    local alpha = rec.sensor and (rec.inside > 0 and 170 or 85) or 255
    if rec.anchor then alpha = 255 end
    local half = (rec.shape == "circle") and rec.r or (rec.vh or 60) / 2
    local top, bot = shade(base, 0.22), shade(base, -0.18)
    v:SetGradient(0, half, 0, -half, top[1], top[2], top[3], alpha, bot[1], bot[2], bot[3], alpha)
    local stroke
    if rec.arena then stroke = { 64, 78, 112 }
    elseif rec.static then stroke = { 140, 160, 208 }
    else stroke = shade(base, 0.5) end
    v:SetStroke(stroke[1], stroke[2], stroke[3], rec.sensor and 220 or 255)
    if rec.sensor and not rec.anchor then v:SetDash(12, 8) else v:SetDash(0) end
end

function World:Find(entity)
    if not entity then return nil end
    local ok, name = pcall(entity.GetName, entity)
    if not ok then return nil end
    return self.byName[name]
end

function World:Remove(rec)
    if rec.removed then return end
    rec.removed = true
    for i = #self.joints, 1, -1 do
        local j = self.joints[i]
        if j.a == rec or j.b == rec then self:RemoveJoint(j) end
    end
    for i, r in ipairs(self.bodies) do
        if r == rec then table.remove(self.bodies, i) break end
    end
    self.byName[rec.name] = nil
    try(rec.body.Destroy, rec.body)
    try(rec.entity.Destroy, rec.entity)
    for i = #self.bodies, 1, -1 do
        local r = self.bodies[i]
        if r.anchor and not self:HasJoint(r) then self:Remove(r) end
    end
end

function World:HasJoint(rec)
    for _, j in ipairs(self.joints) do
        if j.a == rec or j.b == rec then return true end
    end
    return false
end

function World:JointsOf(rec)
    local out = {}
    for _, j in ipairs(self.joints) do
        if j.a == rec or j.b == rec then out[#out + 1] = j end
    end
    return out
end

function World:Clear()
    for i = #self.joints, 1, -1 do self:RemoveJoint(self.joints[i]) end
    for i = #self.bodies, 1, -1 do
        local r = self.bodies[i]
        if not r.arena then self:Remove(r) end
    end
end

function World:SetMass(rec, m)
    rec.mass = m
    if not rec.static then try(rec.body.SetMass, rec.body, m) end
end

function World:SetStatic(rec, on)
    if rec.static == on then return end
    rec.static = on
    try(rec.body.SetMass, rec.body, on and 0 or math.max(rec.mass, 0.05))
    rec.entity:SetZIndex(on and 0 or 1)
    if not on then try(rec.body.Wake, rec.body) end
    self:Style(rec)
end

function World:SetBounce(rec, v) rec.bounce = v; try(rec.body.SetBounce, rec.body, v) end
function World:SetFriction(rec, v) rec.friction = v; try(rec.body.SetFriction, rec.body, v, v) end
function World:SetDamping(rec, lin, ang)
    rec.linDamp, rec.angDamp = lin, ang
    try(rec.body.SetDamping, rec.body, lin, ang)
end
function World:SetSensor(rec, on) rec.sensor = on; try(rec.body.SetSensor, rec.body, on); self:Style(rec) end
function World:SetLocks(rec, rotation, position)
    rec.lockRot, rec.lockPos = rotation, position
    try(rec.body.SetLocked, rec.body, rotation, position)
end
function World:SetNoGravity(rec, on) rec.noGravity = on; try(rec.body.SetIgnoreGravity, rec.body, on) end
function World:SetOneWay(rec, on) rec.oneWay = on; try(rec.body.SetOneWay, rec.body, on, 0, 1) end
function World:SetCanSleep(rec, on) rec.canSleep = on; try(rec.body.SetCanSleep, rec.body, on) end
function World:SetColor(rec, idx) rec.color = idx; self:Style(rec) end

function World:ApplyMask(rec)
    local bit = 1 << rec.layer
    local bits = 0xFFFF
    if rec.maskMode == 2 then bits = bit
    elseif rec.maskMode == 3 then bits = 0xFFFF & ~bit
    elseif rec.maskMode == 4 then bits = 0 end
    try(rec.body.SetMask, rec.body, bits)
end

function World:SetLayer(rec, layer)
    rec.layer = clamp(layer, 0, 15)
    try(rec.body.SetLayer, rec.body, rec.layer)
    self:ApplyMask(rec)
end

function World:SetMaskMode(rec, mode)
    rec.maskMode = mode
    self:ApplyMask(rec)
end

function World:SetGroup(rec, g) rec.group = g; try(rec.body.SetGroup, rec.body, g) end

function World:Pose(rec)
    local x, y = rec.entity:GetPosition()
    return x, y, rec.entity:GetRotation()
end

function World:Duplicate(rec, dx, dy)
    local x, y, a = self:Pose(rec)
    return self:Add(rec.shape, x + (dx or 40), y + (dy or 40), {
        w = rec.w, h = rec.h, r = rec.r, sides = rec.sides, angle = a,
        static = rec.static, mass = rec.mass, bounce = rec.bounce, friction = rec.friction,
        linDamp = rec.linDamp, angDamp = rec.angDamp, sensor = rec.sensor,
        lockRot = rec.lockRot, lockPos = rec.lockPos, noGravity = rec.noGravity,
        oneWay = rec.oneWay, canSleep = rec.canSleep, layer = rec.layer, group = rec.group,
        maskMode = rec.maskMode, color = rec.color,
    })
end

local function localPoint(rec, wx, wy)
    local x, y = rec.entity:GetPosition()
    local ang = rec.entity:GetRotation()
    local lx, ly = rot(wx - x, wy - y, -ang)
    return { lx, ly }
end

local function anchorWorld(rec, la)
    local x, y = rec.entity:GetPosition()
    if not la then return x, y end
    local ox, oy = rot(la[1], la[2], rec.entity:GetRotation())
    return x + ox, y + oy
end

local function jointDefaults(kind)
    if kind == "hinge" then
        return { collide = false, limit = false, lmin = -60, lmax = 60, motor = false, mspeed = 3, mtorque = 40000, drive = false }
    elseif kind == "spring" then
        return { collide = false, softness = 0.01, length = 100, limit = false, lmin = 20, lmax = 300 }
    elseif kind == "rod" then
        return { collide = false, length = 100 }
    elseif kind == "weld" then
        return { collide = false }
    else
        return { collide = false, hz = 4, damp = 0.7, motor = false, mspeed = 14, mtorque = 40000,
                 limit = true, lmin = -30, lmax = 30, drive = false }
    end
end

function World:AddJoint(kind, a, b, spec)
    spec = spec or {}
    local j = { id = self.nextJoint, kind = kind, a = a, b = b, p = jointDefaults(kind) }
    for k, v in pairs(spec.p or {}) do j.p[k] = v end

    local h
    if kind == "hinge" then
        h = Physics.CreateRevoluteJoint(a.entity, b.entity, spec.px, spec.py)
        j.la, j.lb = localPoint(a, spec.px, spec.py), localPoint(b, spec.px, spec.py)
    elseif kind == "spring" or kind == "rod" then
        local ax, ay = anchorWorld(a, nil)
        local bx, by = anchorWorld(b, nil)
        if World.CONFIG.pointAnchors then
            ax, ay = spec.ax or ax, spec.ay or ay
            bx, by = spec.bx or bx, spec.by or by
            j.la, j.lb = localPoint(a, ax, ay), localPoint(b, bx, by)
            h = Physics.CreateDistanceJoint(a.entity, b.entity, ax, ay, bx, by)
        else
            h = Physics.CreateDistanceJoint(a.entity, b.entity)
        end
        if not spec.p or not spec.p.length then j.p.length = dist(ax, ay, bx, by) end
    elseif kind == "weld" then
        h = Physics.CreateWeldJoint(a.entity, b.entity)
    else
        local px, py = spec.px, spec.py
        h = Physics.CreateWheelJoint(a.entity, b.entity, px, py, 0, 1)
        j.la, j.lb = localPoint(a, px, py), localPoint(b, px, py)
    end
    if not h then
        warn("could not create", kind, "joint")
        return nil
    end
    j.h = h
    self.nextJoint = self.nextJoint + 1
    self.joints[#self.joints + 1] = j
    self:ApplyJoint(j)
    return j
end

function World:ApplyJoint(j)
    local h, p = j.h, j.p
    try(h.SetCollideConnected, h, p.collide)
    if j.kind == "spring" or j.kind == "rod" then
        try(h.SetLength, h, p.length)
        if j.kind == "spring" then
            try(h.SetSoftness, h, p.softness)
            if p.limit then try(h.SetLimits, h, p.lmin, p.lmax) else try(h.SetLimits, h, 0, 1e9) end
        end
    elseif j.kind == "hinge" then
        if p.limit then try(h.SetLimits, h, p.lmin, p.lmax) else try(h.SetLimits, h, -36000, 36000) end
        if p.motor then try(h.SetMotor, h, p.mspeed, p.mtorque) else try(h.DisableMotor, h) end
    elseif j.kind == "wheel" then
        try(h.SetSpring, h, p.hz, p.damp)
        if p.limit then try(h.SetLimits, h, p.lmin, p.lmax) else try(h.SetLimits, h, -100000, 100000) end
        if p.motor then try(h.SetMotor, h, p.mspeed, p.mtorque) else try(h.SetMotor, h, 0, 0) end
    end
    j.driveDir = nil
end

function World:RemoveJoint(j)
    for i, x in ipairs(self.joints) do
        if x == j then table.remove(self.joints, i) break end
    end
    j.removed = true
    try(j.h.Destroy, j.h)
end

function World:Drive(dir)
    for _, j in ipairs(self.joints) do
        local p = j.p
        if p.drive and j.driveDir ~= dir then
            j.driveDir = dir
            if j.kind == "hinge" then
                if dir == 0 then
                    if p.motor then try(j.h.SetMotor, j.h, p.mspeed, p.mtorque) else try(j.h.DisableMotor, j.h) end
                else
                    try(j.h.SetMotor, j.h, dir * math.abs(p.mspeed), p.mtorque)
                end
            elseif j.kind == "wheel" then
                try(j.h.SetMotor, j.h, dir * math.abs(p.mspeed), dir == 0 and 0 or p.mtorque)
            end
        end
    end
end

function World:JointEnds(j)
    local ax, ay = anchorWorld(j.a, j.la)
    local bx, by = anchorWorld(j.b, j.lb)
    local hx, hy = (ax + bx) / 2, (ay + by) / 2
    if j.kind == "hinge" then hx, hy = ax, ay end
    return ax, ay, bx, by, hx, hy
end

function World:AddRope(a, b, x1, y1, x2, y2, segLen, thick)
    local len = dist(x1, y1, x2, y2)
    if len < 20 then return nil end
    segLen = segLen or 24
    local n = clamp(round(len / segLen), 2, 60)
    local ang = math.deg(math.atan(y2 - y1, x2 - x1))
    local step = len / n
    local first, prev = nil, a
    local links = {}
    local px, py = x1, y1
    for i = 1, n do
        local nx, ny = x1 + (x2 - x1) * i / n, y1 + (y2 - y1) * i / n
        local link = self:Add("box", (px + nx) / 2, (py + ny) / 2, {
            w = step + 2, h = thick or 8, angle = ang, mass = 0.4, bounce = 0.05, friction = 0.6,
            color = 3,
        })
        links[#links + 1] = link
        if not prev then prev = self:Anchor(px, py) end
        self:AddJoint("hinge", prev, link, { px = px, py = py })
        prev = link
        px, py = nx, ny
    end
    local last = b or self:Anchor(x2, y2)
    self:AddJoint("hinge", prev, last, { px = x2, py = y2 })
    return links
end

function World:BuildArena()
    local A = World.ARENA
    local cx, cy = (A.x0 + A.x1) / 2, (A.y0 + A.y1) / 2
    local w, h, t = A.x1 - A.x0, A.y1 - A.y0, A.wall
    local function wall(x, y, ww, hh)
        return self:Add("box", x, y, { w = ww, h = hh, static = true, arena = true, friction = 0.6, bounce = 0.1 })
    end
    self.arena = {
        wall(cx, A.y0 - t / 2, w + t * 2, t),
        wall(cx, A.y1 + t / 2, w + t * 2, t),
        wall(A.x0 - t / 2, cy, t, h + t * 2),
        wall(A.x1 + t / 2, cy, t, h + t * 2),
    }
end

local function dump(v)
    local t = type(v)
    if t == "number" then
        if v ~= v or v == math.huge or v == -math.huge then return "0" end
        return string.format("%.6g", v)
    elseif t == "boolean" then return tostring(v)
    elseif t == "string" then return string.format("%q", v)
    elseif t == "table" then
        local parts = {}
        local n = #v
        for i = 1, n do parts[#parts + 1] = dump(v[i]) end
        local keys = {}
        for k in pairs(v) do
            if not (type(k) == "number" and k >= 1 and k <= n) then keys[#keys + 1] = k end
        end
        table.sort(keys, function(a, b) return tostring(a) < tostring(b) end)
        for _, k in ipairs(keys) do parts[#parts + 1] = tostring(k) .. "=" .. dump(v[k]) end
        return "{" .. table.concat(parts, ",") .. "}"
    end
    return "nil"
end

function World:Snapshot(name)
    local data = { version = 1, name = name or "Saved scene", bodies = {}, joints = {} }
    for _, r in ipairs(self.bodies) do
        if not r.arena then
            local x, y, a = self:Pose(r)
            data.bodies[#data.bodies + 1] = {
                id = r.id, shape = r.shape, x = x, y = y, angle = a,
                w = r.w, h = r.h, r = r.r, sides = r.sides, static = r.static, mass = r.mass,
                bounce = r.bounce, friction = r.friction, linDamp = r.linDamp, angDamp = r.angDamp,
                sensor = r.sensor, lockRot = r.lockRot, lockPos = r.lockPos, noGravity = r.noGravity,
                oneWay = r.oneWay, canSleep = r.canSleep, layer = r.layer, group = r.group,
                maskMode = r.maskMode, color = r.color, anchor = r.anchor, tracer = r.tracer,
            }
        end
    end
    for _, j in ipairs(self.joints) do
        local ax, ay, bx, by = self:JointEnds(j)
        data.joints[#data.joints + 1] = {
            kind = j.kind, a = j.a.id, b = j.b.id, px = ax, py = ay, ax = ax, ay = ay, bx = bx, by = by, p = j.p,
        }
    end
    return data
end

function World:Restore(data)
    self:Clear()
    local map = {}
    for _, b in ipairs(data.bodies or {}) do
        map[b.id] = self:Add(b.shape, b.x, b.y, b)
    end
    for _, jd in ipairs(data.joints or {}) do
        local a, b = map[jd.a], map[jd.b]
        if a and b then
            self:AddJoint(jd.kind, a, b, { px = jd.px, py = jd.py, ax = jd.ax, ay = jd.ay, bx = jd.bx, by = jd.by, p = jd.p })
        end
    end
end

function World:SaveToFile(path, name)
    local f, err = io.open(path, "w")
    if not f then return false, err end
    f:write("return ", dump(self:Snapshot(name)), "\n")
    f:close()
    return true
end

function World:LoadFromFile(path)
    local f = io.open(path, "r")
    if not f then return false, "no file" end
    local text = f:read("a")
    f:close()
    local fn, err = load(text, path, "t", {})
    if not fn then return false, err end
    local ok, data = pcall(fn)
    if not ok or type(data) ~= "table" then return false, "bad file" end
    self:Restore(data)
    return true, data.name
end

-- =========================================================================
-- UI
-- =========================================================================
local UI = {}
UI.__index = UI

local T = {
    panel   = { 15, 19, 31, 240 },
    bar     = { 12, 15, 25, 246 },
    card    = { 24, 30, 46, 255 },
    btn     = { 35, 43, 66, 255 },
    line    = { 255, 255, 255, 24 },
    text    = { 229, 235, 250, 255 },
    dim     = { 134, 148, 180, 255 },
    accent  = { 104, 120, 255, 255 },
    good    = { 40, 200, 160, 255 },
    danger  = { 226, 78, 104, 255 },
    modal   = { 4, 6, 12, 190 },
}
UI.THEME = T

local ROW_H, ROWS = 36, 24
local PANEL_W = 360

local function addc(e, ...)
    local names = { ... }
    local lastErr
    for _, n in ipairs(names) do
        local ok, h = pcall(e.Add, e, n)
        if ok and h then return h end
        lastErr = h
    end
    error("cannot add component " .. tostring(names[1]) .. ": " .. tostring(lastErr))
end

local function applyRect(r, s)
    if s.a then r:SetAnchors(unpack(s.a)) end
    if s.pv then r:SetPivot(unpack(s.pv)) end
    if s.pos then r:SetPosition(unpack(s.pos)) end
    if s.size then r:SetSize(unpack(s.size)) end
    if s.m then r:SetMargin(unpack(s.m)) end
end

function UI.new(ctl)
    local u = setmetatable({}, UI)
    u.ctl = ctl
    u.n = 0
    u.callbacks = {}
    u.groups = {}
    u.toolButtons, u.tabButtons = {}, {}
    u.rows = {}
    u.allButtons = {}
    u.toast = { time = 0 }
    u.root = Scene.Spawn("SandboxUI", 0, 0)
    local canvas = addc(u.root, "Canvas")
    try(canvas.SetReferenceSize, canvas, 1920, 1080)
    try(canvas.SetSortOrder, canvas, 10)
    return u
end

function UI:uid(prefix)
    self.n = self.n + 1
    return "ui_" .. prefix .. self.n
end

function UI:Node(parent, prefix, rect, group)
    local e = Scene.Spawn(self:uid(prefix), 0, 0, parent or self.root)
    local r = addc(e, "UIRect")
    applyRect(r, rect)
    if group then
        local g = self.groups[group]
        if not g then g = {}; self.groups[group] = g end
        g[#g + 1] = r
    end
    return e, r
end

function UI:Panel(parent, rect, o)
    o = o or {}
    local e, r = self:Node(parent, "p", rect, o.group)
    local p = addc(e, "UIPanel")
    p:SetColor(unpack(o.color or T.panel))
    p:SetCornerRadius(o.radius or 0)
    if o.border then p:SetBorder(o.border, unpack(o.borderColor or T.line)) end
    return { e = e, r = r, p = p, color = o.color or T.panel }
end

function UI:Label(parent, rect, text, o)
    o = o or {}
    local e, r = self:Node(parent, "l", rect, o.group)
    local l = addc(e, "UILabel")
    l:SetText(text or "")
    l:SetFontSize(o.size or 20)
    l:SetColor(unpack(o.color or T.text))
    l:SetAlign(o.h or 0, o.v or 1)
    if o.shadow ~= nil then try(l.SetShadow, l, o.shadow) end
    return { e = e, r = r, l = l, text = text or "" }
end

function UI:SetText(w, text)
    if w.text ~= text then
        w.text = text
        w.l:SetText(text)
    end
end

function UI:Button(parent, rect, text, o, onClick)
    o = o or {}
    local b = self:Panel(parent, rect, {
        color = o.color or T.btn, radius = o.radius or 9, border = 1, borderColor = T.line, group = o.group,
    })
    -- Make sure the button's own rect gets the click before passing through
    pcall(function() b.r:SetBlocksInput(true) end)

    b.ub = addc(b.e, "UIButton")
    pcall(function() b.ub:SetEnabled(true) end)
    try(b.ub.SetHoverScale, b.ub, 1.035)
    try(b.ub.SetPressScale, b.ub, 0.965)

    b.label = self:Label(b.e, { a = { 0, 0, 1, 1 }, m = { 0, 0, 0, 0 } }, text,
        { size = o.size or 19, h = 1, v = 1, color = o.textColor or T.text, group = o.group })
    b.onClick = onClick
    b.wasPressed = false
    b.isStepButton = false
    self.allButtons[#self.allButtons + 1] = b
    return b
end

function UI:SetButtonColor(b, color)
    if b.color ~= color then
        b.color = color
        pcall(function() b.p:SetColor(unpack(color)) end)
    end
end

function UI:Show(group, visible)
    for _, r in ipairs(self.groups[group] or {}) do r:SetVisible(visible) end
end

function UI:Toast(text, seconds)
    self:SetText(self.toastLabel, text)
    self.toast.time = seconds or 2.4
    self.toast.total = self.toast.time
end

function UI:Build()
    local ctl = self.ctl

    local top = self:Panel(self.root, { a = { 0, 1, 1, 1 }, pv = { 0.5, 1 }, pos = { 0, 0 }, size = { 0, 60 }, m = { 0, 0, 0, 0 } },
        { color = T.bar })
    self:Panel(top.e, { a = { 0, 0, 1, 0 }, pv = { 0.5, 0 }, size = { 0, 1 }, m = { 0, 0, 0, 0 } }, { color = T.line })
    self:Panel(top.e, { a = { 0, 0.5 }, pv = { 0, 0.5 }, pos = { 16, 0 }, size = { 6, 26 } }, { color = T.accent, radius = 3 })
    self:Label(top.e, { a = { 0, 0.5 }, pv = { 0, 0.5 }, pos = { 34, 0 }, size = { 300, 60 } }, "PHYSICS SANDBOX", { size = 25 })

    self:Button(top.e, { a = { 0, 0.5 }, pv = { 0, 0.5 }, pos = { 330, 0 }, size = { 124, 38 } }, "Scenes",
        { color = T.accent, size = 20 }, function() ctl:OpenScenes() end)
    self:Button(top.e, { a = { 0, 0.5 }, pv = { 0, 0.5 }, pos = { 462, 0 }, size = { 104, 38 } }, "Reset", { size = 20 },
        function() ctl:ResetScene() end)
    self:Button(top.e, { a = { 0, 0.5 }, pv = { 0, 0.5 }, pos = { 574, 0 }, size = { 104, 38 } }, "Clear", { size = 20 },
        function() ctl:ClearAll() end)

    self.sceneLabel = self:Label(top.e, { a = { 0.5, 0.5 }, pv = { 0.5, 0.5 }, pos = { 90, 0 }, size = { 440, 60 } }, "",
        { size = 21, h = 1, color = T.dim })
    self.statsLabel = self:Label(top.e, { a = { 1, 0.5 }, pv = { 1, 0.5 }, pos = { -20, 0 }, size = { 560, 60 } }, "",
        { size = 19, h = 2, color = T.dim })

    local bottom = self:Panel(self.root, { a = { 0, 0, 1, 0 }, pv = { 0.5, 0 }, pos = { 0, 0 }, size = { 0, 44 }, m = { 0, 0, 0, 0 } },
        { color = T.bar })
    self:Panel(bottom.e, { a = { 0, 1, 1, 1 }, pv = { 0.5, 1 }, size = { 0, 1 }, m = { 0, 0, 0, 0 } }, { color = T.line })
    self.hintLabel = self:Label(bottom.e, { a = { 0, 0.5 }, pv = { 0, 0.5 }, pos = { 20, 0 }, size = { 1500, 44 } }, "",
        { size = 18, color = T.dim })

    local left = self:Panel(self.root, { a = { 0, 0, 0, 1 }, pv = { 0, 0.5 }, pos = { 0, 0 }, size = { 212, 0 }, m = { 0, 44, 0, 60 } },
        { color = T.panel })
    self:Panel(left.e, { a = { 1, 0, 1, 1 }, pv = { 1, 0.5 }, size = { 1, 0 }, m = { 0, 0, 0, 0 } }, { color = T.line })
    local y = 14
    for _, group in ipairs(ctl.TOOL_GROUPS) do
        self:Label(left.e, { a = { 0, 1 }, pv = { 0, 1 }, pos = { 14, -y }, size = { 180, 22 } }, group.title,
            { size = 14, color = T.dim })
        y = y + 26
        for i, tool in ipairs(group.tools) do
            local col = (i - 1) % 2
            local b = self:Button(left.e, { a = { 0, 1 }, pv = { 0, 1 }, pos = { 12 + col * 100, -y }, size = { 92, 42 } },
                tool.label, { size = 18 }, function() ctl:SetTool(tool.id) end)
            self.toolButtons[tool.id] = b
            if col == 1 or i == #group.tools then y = y + 48 end
        end
        y = y + 10
    end

    local right = self:Panel(self.root, { a = { 1, 0, 1, 1 }, pv = { 1, 0.5 }, pos = { 0, 0 }, size = { PANEL_W, 0 }, m = { 0, 44, 0, 60 } },
        { color = T.panel })
    self:Panel(right.e, { a = { 0, 0, 0, 1 }, pv = { 0, 0.5 }, size = { 1, 0 }, m = { 0, 0, 0, 0 } }, { color = T.line })
    self.rightPanel = right
    for i, tab in ipairs({ { "create", "Create" }, { "edit", "Edit" }, { "world", "World" } }) do
        local b = self:Button(right.e, { a = { 0, 1 }, pv = { 0, 1 }, pos = { 14 + (i - 1) * 112, -12 }, size = { 106, 36 } },
            tab[2], { size = 19 }, function() ctl:SetTab(tab[1]) end)
        self.tabButtons[tab[1]] = b
    end
    self:BuildRows(right.e)

    local toast = self:Panel(self.root, { a = { 0.5, 0 }, pv = { 0.5, 0 }, pos = { -60, 66 }, size = { 560, 44 } },
        { color = { 22, 27, 44, 235 }, radius = 14, border = 1, borderColor = { 120, 134, 255, 90 }, group = "toast" })
    self.toastPanel = toast
    self.toastLabel = self:Label(toast.e, { a = { 0, 0, 1, 1 }, m = { 12, 0, 12, 0 } }, "",
        { size = 20, h = 1, v = 1, group = "toast" })
    self:Show("toast", false)

    self:BuildModal()
end

function UI:BuildModal()
    local ctl = self.ctl
    local g = "modal"
    local dim = self:Panel(self.root, { a = { 0, 0, 1, 1 }, m = { 0, 0, 0, 0 } }, { color = T.modal, group = g })
    local card = self:Panel(dim.e, { a = { 0.5, 0.5 }, pv = { 0.5, 0.5 }, pos = { 0, 0 }, size = { 840, 700 } },
        { color = T.panel, radius = 22, border = 2, borderColor = { 120, 134, 255, 110 }, group = g })
    self:Label(card.e, { a = { 0, 1 }, pv = { 0, 1 }, pos = { 32, -22 }, size = { 500, 40 } }, "Scenes", { size = 30, group = g })
    self:Label(card.e, { a = { 0, 1 }, pv = { 0, 1 }, pos = { 32, -64 }, size = { 760, 26 } },
        "Load a ready-made setup, or save what you built. Loading replaces everything in the arena.",
        { size = 17, color = T.dim, group = g })
    self:Button(card.e, { a = { 1, 1 }, pv = { 1, 1 }, pos = { -24, -24 }, size = { 44, 40 } }, "X",
        { size = 20, group = g }, function() ctl:CloseScenes() end)

    self.sceneButtons = {}
    for i, sc in ipairs(ctl.scenes) do
        local col, row = (i - 1) % 3, math.floor((i - 1) / 3)
        local b = self:Button(card.e, { a = { 0, 1 }, pv = { 0, 1 }, pos = { 32 + col * 264, -(110 + row * 84) }, size = { 252, 72 } },
            "", { group = g, radius = 14 }, function() ctl:LoadPreset(i) end)
        b.label.l:SetText("")
        self:Label(b.e, { a = { 0, 1 }, pv = { 0, 1 }, pos = { 14, -9 }, size = { 224, 26 } }, sc.name, { size = 21, group = g })
        self:Label(b.e, { a = { 0, 1 }, pv = { 0, 1 }, pos = { 14, -37 }, size = { 224, 26 } }, sc.short or "",
            { size = 15, color = T.dim, group = g })
        self.sceneButtons[i] = b
    end

    local y = 110 + math.ceil(#ctl.scenes / 3) * 84 + 14
    self:Label(card.e, { a = { 0, 1 }, pv = { 0, 1 }, pos = { 32, -y }, size = { 400, 24 } }, "YOUR SAVES",
        { size = 15, color = T.dim, group = g })
    y = y + 30
    for slot = 1, 3 do
        local x = 32 + (slot - 1) * 264
        self:Button(card.e, { a = { 0, 1 }, pv = { 0, 1 }, pos = { x, -y }, size = { 124, 44 } }, "Save " .. slot,
            { group = g, color = T.btn, size = 18 }, function() ctl:SaveSlot(slot) end)
        self:Button(card.e, { a = { 0, 1 }, pv = { 0, 1 }, pos = { x + 130, -y }, size = { 122, 44 } }, "Load " .. slot,
            { group = g, color = T.accent, size = 18 }, function() ctl:LoadSlot(slot) end)
    end
    self:Show(g, false)
end

function UI:BuildRows(parent)
    for i = 1, ROWS do
        local y = -(62 + (i - 1) * ROW_H)
        local row = { index = i, y = y, hold = {}, rep = {}, fired = {}, wasPressed = {} }
        row.lbl = self:Label(parent, { a = { 0, 1 }, pv = { 0, 1 }, pos = { 14, y }, size = { 170, ROW_H } }, "", { size = 18 })
        row.val = self:Label(parent, { a = { 0, 1 }, pv = { 0, 1 }, pos = { 228, y }, size = { 70, ROW_H } }, "",
            { size = 18, h = 1, color = T.accent })
        for k = 1, 3 do
            local key = "b" .. k
            local b = self:Button(parent, { a = { 0, 1 }, pv = { 0, 1 }, pos = { 190, y - 3 }, size = { 36, ROW_H - 6 } }, "",
                { size = 18 }, nil)
            b.isStepButton = true
            row[key] = b
        end
        self.rows[i] = row
        self:ShowRow(row, "none")
    end
end

local function setRect(r, x, y, w, h)
    r:SetPosition(x, y)
    r:SetSize(w, h)
end

function UI:ShowRow(row, kind)
    local show = {
        none = {}, header = { "lbl" }, info = { "lbl" }, step = { "lbl", "val", "b1", "b2" },
        toggle = { "lbl", "b3" }, actions = { "b1", "b2", "b3" },
    }
    local want = {}
    for _, k in ipairs(show[kind] or {}) do want[k] = true end
    for _, k in ipairs({ "lbl", "val", "b1", "b2", "b3" }) do
        row[k].r:SetVisible(want[k] == true)
    end
    row.kind = kind
end

function UI:SetRows(specs)
    for i, row in ipairs(self.rows) do
        local s = specs[i]
        row.spec = s
        if not s then
            self:ShowRow(row, "none")
        else
            local y = row.y
            self:ShowRow(row, s.kind)
            if s.kind == "header" then
                setRect(row.lbl.r, 14, y, 332, ROW_H)
                row.lbl.l:SetColor(unpack(T.accent))
                row.lbl.l:SetFontSize(16)
                self:SetText(row.lbl, s.label)
            elseif s.kind == "info" then
                setRect(row.lbl.r, 14, y, 332, ROW_H)
                row.lbl.l:SetColor(unpack(T.dim))
                row.lbl.l:SetFontSize(16)
                self:SetText(row.lbl, s.text and s.text() or s.label or "")
            elseif s.kind == "step" then
                setRect(row.lbl.r, 14, y, 172, ROW_H)
                row.lbl.l:SetColor(unpack(T.text))
                row.lbl.l:SetFontSize(18)
                self:SetText(row.lbl, s.label)
                setRect(row.b1.r, 190, y - 3, 36, ROW_H - 6)
                setRect(row.b2.r, 300, y - 3, 36 + 10, ROW_H - 6)
                setRect(row.val.r, 228, y, 70, ROW_H)
                self:SetText(row.b1.label, "-")
                self:SetText(row.b2.label, "+")
                self:SetButtonColor(row.b1, T.btn)
                self:SetButtonColor(row.b2, T.btn)
            elseif s.kind == "toggle" then
                setRect(row.lbl.r, 14, y, 230, ROW_H)
                row.lbl.l:SetColor(unpack(T.text))
                row.lbl.l:SetFontSize(18)
                self:SetText(row.lbl, s.label)
                setRect(row.b3.r, 262, y - 3, 84, ROW_H - 6)
            elseif s.kind == "actions" then
                local n = #s.items
                local gap = 6
                local w = (332 - gap * (n - 1)) / n
                for k = 1, 3 do
                    local b = row["b" .. k]
                    local it = s.items[k]
                    b.r:SetVisible(it ~= nil)
                    if it then
                        setRect(b.r, 14 + (k - 1) * (w + gap), y - 3, w, ROW_H - 6)
                        self:SetText(b.label, it.label)
                        local tone = it.tone == "danger" and T.danger or (it.tone == "primary" and T.accent or T.btn)
                        self:SetButtonColor(b, tone)
                    end
                end
            end
        end
    end
    self:RefreshRows()
end

function UI:FireRow(row, key)
    local s = row.spec
    if not s then return end
    if s.kind == "step" then
        if key == "b1" then s.dec() elseif key == "b2" then s.inc() end
    elseif s.kind == "toggle" then
        s.set(not s.get())
    elseif s.kind == "actions" then
        local it = s.items[tonumber(key:sub(2))]
        if it then it.fn() end
    end
    self.ctl:AfterRowEdit(s)
    self:RefreshRows()
end

function UI:RefreshRows()
    for _, row in ipairs(self.rows) do
        local s = row.spec
        if s then
            local ok = pcall(function()
                if s.kind == "step" then
                    self:SetText(row.val, s.value())
                elseif s.kind == "toggle" then
                    local on = s.get()
                    self:SetText(row.b3.label, on and "ON" or "OFF")
                    self:SetButtonColor(row.b3, on and T.good or T.btn)
                elseif s.kind == "info" and s.text then
                    self:SetText(row.lbl, s.text())
                end
            end)
            if not ok then self:ShowRow(row, "none"); row.spec = nil end
        end
    end
end

function UI:SetActiveTool(id)
    for tid, b in pairs(self.toolButtons) do
        self:SetButtonColor(b, tid == id and T.accent or T.btn)
    end
end

function UI:SetActiveTab(tab)
    for id, b in pairs(self.tabButtons) do
        self:SetButtonColor(b, id == tab and T.accent or T.btn)
    end
end

function UI:Update(dt)
    -- Step row hold-to-repeat
    for _, row in ipairs(self.rows) do
        local s = row.spec
        if s and s.kind == "step" then
            for _, key in ipairs({ "b1", "b2" }) do
                local b = row[key]
                if b.e:IsValid() and b.r:IsVisible() then
                    local pressed = false
                    pcall(function() pressed = b.ub:IsPressed() and true or false end)
                    if pressed and not b.wasPressed then
                        self:FireRow(row, key)
                    elseif pressed then
                        row.hold[key] = (row.hold[key] or 0) + dt
                        if row.hold[key] > 0.4 then
                            row.rep[key] = (row.rep[key] or 0) + dt
                            while row.rep[key] > 0.07 do
                                row.rep[key] = row.rep[key] - 0.07
                                self:FireRow(row, key)
                            end
                        end
                    else
                        row.hold[key], row.rep[key] = 0, 0
                    end
                    b.wasPressed = pressed
                end
            end
        end
    end

    -- All other buttons: multi-method click detection
    -- Fire on EITHER the press-state edge OR a fresh mouse-down while hovering.
    local mouseDownNow = Input.IsMouseJustPressed(Key.MOUSE_LEFT)
    for _, b in ipairs(self.allButtons) do
        if b.e:IsValid() and not b.isStepButton and b.onClick then
            local hovered = false
            local pressed = false
            pcall(function()
                hovered = b.ub:IsHovered() and true or false
                pressed = b.ub:IsPressed() and true or false
            end)

            local pressEdge  = pressed and not b.wasPressed
            local hoverClick = hovered and mouseDownNow

            if pressEdge or hoverClick then
                local ok, err = pcall(b.onClick)
                if not ok then
                    log.warn("[sandbox-ui] onClick failed: " .. tostring(err))
                end
            end

            b.wasPressed = pressed
        end
    end

    -- Toast fade
    local t = self.toast
    if t.time > 0 then
        t.time = t.time - dt
        if not t.visible then self:Show("toast", true); t.visible = true end
        local a = math.min(1, t.time * 3, (t.total - t.time) * 8)
        a = math.max(0, a)
        self.toastLabel.l:SetColor(T.text[1], T.text[2], T.text[3], math.floor(255 * a))
        self.toastPanel.p:SetColor(22, 27, 44, math.floor(235 * a))
        if t.time <= 0 then self:Show("toast", false); t.visible = false end
    end
end

-- =========================================================================
-- SCENES (preset data)
-- =========================================================================
local Scenes = {}

local function velocity(rec, x, y) pcall(rec.body.SetVelocity, rec.body, x, y) end
local function rnd(a, b) return a + math.random() * (b - a) end

local PINK, MINT, SKY, AMBER, CORAL, SUN, AQUA, VIOLET = 8, 4, 6, 2, 1, 3, 5, 7

local function toyBox(c)
    local W, A = c.W, c.A
    local cx = (A.x0 + A.x1) / 2
    W:Add("box", -500, -250, { w = 360, h = 20, angle = -18, static = true })
    for row = 0, 3 do
        for i = 0, 3 - row do
            W:Add("box", 230 + (i - (3 - row) / 2) * 50, A.y0 + 24 + row * 48,
                { w = 46, h = 46, mass = 0.8, color = AMBER + (row % 3), friction = 0.7 })
        end
    end
    local plank = W:Add("box", -90, -350, { w = 340, h = 16, mass = 3, color = SKY })
    W:AddJoint("hinge", plank, W:Anchor(-90, -350), { px = -90, py = -350 })
    W:Add("circle", -200, -310, { r = 22, color = PINK, bounce = 0.4 })
    W:Add("box", 20, 40, { w = 60, h = 60, mass = 4, color = CORAL })
    local top = W:Anchor(cx + 330, 330)
    local ball = W:Add("circle", cx + 330, 100, { r = 34, mass = 2, color = MINT, bounce = 0.5 })
    W:AddJoint("spring", top, ball, { p = { softness = 0.004 } })
    for i = 1, 7 do
        W:Add("circle", rnd(-650, -150), rnd(150, 380), { r = rnd(14, 26), color = 1 + (i % 8), bounce = 0.45 })
    end
    c.toast("Toy Box: grab things, drop things, add joints from the left toolbar")
end

local function cradle(c)
    local W, A = c.W, c.A
    local cx = (A.x0 + A.x1) / 2
    local top, L, r = 250, 300, 34
    W:Add("box", cx, top + 14, { w = 460, h = 14, static = true, sensor = true })
    for i = 1, 5 do
        local x = cx + (i - 3) * (r * 2 + 1)
        local bx, by = x, top - L
        if i == 1 then
            local a = math.rad(48)
            bx, by = x - L * math.sin(a), top - L * math.cos(a)
        end
        local ball = W:Add("circle", bx, by, { r = r, mass = 2, bounce = 1.0, friction = 0,
            color = (i == 1) and CORAL or SKY })
        local pin = W:Anchor(x, top)
        W:AddJoint("rod", pin, ball)
    end
    c.toast("Newton's cradle: rods are distance joints; try the Spring tool instead")
end

local function chaos(c)
    local W, A = c.W, c.A
    local cx = (A.x0 + A.x1) / 2
    local y = 220
    local pin = W:Anchor(cx, y)
    local arm1 = W:Add("box", cx + 100, y, { w = 200, h = 14, mass = 1.5, color = SKY })
    local arm2 = W:Add("box", cx + 290, y, { w = 180, h = 14, mass = 1.2, color = VIOLET })
    local bob = W:Add("circle", cx + 392, y, { r = 24, mass = 1.5, color = PINK, tracer = true })
    W:AddJoint("hinge", pin, arm1, { px = cx, py = y })
    W:AddJoint("hinge", arm1, arm2, { px = cx + 200, py = y })
    W:AddJoint("hinge", arm2, bob, { px = cx + 380, py = y })
    c.ctl:SetTrails(true)
    c.toast("Double pendulum with a tracer. Nudge it and watch it go chaotic")
end

local function bridge(c)
    local W, A = c.W, c.A
    local x1, x2, y, n = -620, 480, -110, 12
    local seg = (x2 - x1) / n
    W:Add("box", x1 - 26, -300, { w = 52, h = 280, static = true })
    W:Add("box", x2 + 26, -300, { w = 52, h = 280, static = true })
    local prev = W:Anchor(x1, y)
    for i = 1, n do
        local plank = W:Add("box", x1 + (i - 0.5) * seg, y,
            { w = seg - 3, h = 14, mass = 1, color = AMBER, friction = 0.8 })
        W:AddJoint("hinge", prev, plank, { px = x1 + (i - 1) * seg, py = y })
        prev = plank
    end
    W:AddJoint("hinge", prev, W:Anchor(x2, y), { px = x2, py = y })
    for i = 1, 5 do
        W:Add("box", -400 + i * 120, 120 + (i % 2) * 70, { w = 50, h = 50, mass = 1.5, color = 1 + i })
    end
    for i = 1, 3 do
        W:Add("circle", -250 + i * 150, 330, { r = 30, mass = 3, color = MINT, bounce = 0.2 })
    end
    c.toast("Rope bridge: every plank is a hinge joint")
end

local function car(c)
    local W, A = c.W, c.A
    W:Add("box", -320, -430, { w = 120, h = 24, static = true, angle = 4 })
    W:Add("box", -120, -430, { w = 90, h = 30, static = true, angle = -6 })
    W:Add("box", 100, -405, { w = 340, h = 20, static = true, angle = 16 })
    W:Add("box", 360, -300, { w = 200, h = 20, static = true })
    local x, wy = -600, -400
    local chassis = W:Add("box", x, wy + 46, { w = 190, h = 30, mass = 6, color = CORAL, friction = 0.4 })
    local cab = W:Add("box", x - 14, wy + 78, { w = 90, h = 32, mass = 0.8, color = SUN })
    W:AddJoint("weld", chassis, cab)
    for _, dx in ipairs({ -66, 66 }) do
        local wheel = W:Add("circle", x + dx, wy, { r = 34, mass = 1.5, friction = 1.6, bounce = 0.1, color = SKY })
        W:AddJoint("wheel", chassis, wheel, { px = x + dx, py = wy,
            p = { hz = 3, damp = 0.6, drive = true, mspeed = 16, mtorque = 60000, lmin = -26, lmax = 26 } })
    end
    c.toast("Car: drive with A / D or the arrow keys. Wheels are wheel joints", 4)
end

local function ragdoll(W, x, y, colorIdx)
    local torso = W:Add("box", x, y, { w = 34, h = 80, mass = 2, color = colorIdx })
    local head = W:Add("circle", x, y + 66, { r = 22, mass = 0.8, color = colorIdx, bounce = 0.3 })
    W:AddJoint("hinge", torso, head, { px = x, py = y + 42, p = { limit = true, lmin = -35, lmax = 35 } })
    for _, s in ipairs({ -1, 1 }) do
        local arm = W:Add("box", x + s * 24, y + 9, { w = 14, h = 54, mass = 0.6, color = colorIdx })
        W:AddJoint("hinge", torso, arm, { px = x + s * 24, py = y + 36, p = { limit = true, lmin = -100, lmax = 100 } })
        local leg = W:Add("box", x + s * 9, y - 71, { w = 16, h = 62, mass = 0.9, color = colorIdx })
        W:AddJoint("hinge", torso, leg, { px = x + s * 9, py = y - 40, p = { limit = true, lmin = -70, lmax = 70 } })
    end
    return torso
end

local function ragdolls(c)
    local W = c.W
    W:Add("box", -300, -150, { w = 380, h = 20, static = true, angle = -8 })
    W:Add("box", 200, -230, { w = 300, h = 20, static = true, angle = 6 })
    ragdoll(W, -330, 160, 3)
    ragdoll(W, 220, 260, 8)
    W:Add("circle", -120, 380, { r = 40, mass = 8, color = 1, bounce = 0.5 })
    c.toast("Ragdolls: hinges with angle limits. Throw something heavy")
end

local function jelly(c)
    local W, A = c.W, c.A
    local cx = (A.x0 + A.x1) / 2
    local sx, sy = cx - 200, 150
    local N, R = 14, 70
    local center = W:Add("circle", sx, sy, { r = 18, mass = 0.6, color = PINK, bounce = 0.3 })
    local ring = {}
    for i = 1, N do
        local a = (i - 1) / N * 2 * math.pi
        ring[i] = W:Add("circle", sx + math.cos(a) * R, sy + math.sin(a) * R,
            { r = 13, mass = 0.5, color = PINK, bounce = 0.4, friction = 0.6 })
    end
    local soft = { softness = 0.002 }
    for i = 1, N do
        W:AddJoint("spring", ring[i], ring[i % N + 1], { p = soft })
        W:AddJoint("spring", ring[i], center, { p = soft })
        W:AddJoint("spring", ring[i], ring[(i + 1) % N + 1], { p = soft })
    end
    local g0x, g0y, s = cx + 140, 230, 46
    local grid = {}
    for i = 0, 3 do
        grid[i] = {}
        for j = 0, 3 do
            grid[i][j] = W:Add("circle", g0x + i * s, g0y + j * s, { r = 16, mass = 0.5, color = AQUA, bounce = 0.4 })
        end
    end
    local gs = { softness = 0.003 }
    for i = 0, 3 do
        for j = 0, 3 do
            if i < 3 then W:AddJoint("spring", grid[i][j], grid[i + 1][j], { p = gs }) end
            if j < 3 then W:AddJoint("spring", grid[i][j], grid[i][j + 1], { p = gs }) end
            if i < 3 and j < 3 then
                W:AddJoint("spring", grid[i][j], grid[i + 1][j + 1], { p = gs })
                W:AddJoint("spring", grid[i + 1][j], grid[i][j + 1], { p = gs })
            end
        end
    end
    W:Add("box", cx, -250, { w = 300, h = 20, static = true, angle = 5 })
    c.toast("Jelly: soft bodies made only of spring joints. Select one and change Softness")
end

local function catapult(c)
    local W, A = c.W, c.A
    local px, py = -120, -330
    W:Add("box", px, py - 63, { w = 40, h = 110, static = true })
    local lever = W:Add("box", px - 100, py, { w = 460, h = 16, mass = 3, color = SKY })
    W:AddJoint("hinge", lever, W:Anchor(px, py), { px = px, py = py })
    W:Add("circle", px - 330, py + 30, { r = 22, mass = 1, color = CORAL, bounce = 0.3 })
    W:Add("box", px + 110, 120, { w = 64, h = 64, mass = 22, color = VIOLET })
    for row = 0, 3 do
        for i = 0, 3 - row do
            W:Add("box", 330 + (i - (3 - row) / 2) * 44, A.y0 + 21 + row * 42,
                { w = 40, h = 40, mass = 0.6, color = SUN })
        end
    end
    c.toast("Catapult: the weight drops on the short arm of a hinged lever")
end

local function dominoes(c)
    local W, A = c.W, c.A
    W:Add("box", -560, -250, { w = 320, h = 16, static = true, angle = -20 })
    W:Add("circle", -690, -130, { r = 30, mass = 3, color = CORAL, friction = 0.4 })
    for i = 0, 21 do
        W:Add("box", -330 + i * 40, A.y0 + 40,
            { w = 20, h = 80, mass = 0.8, color = 1 + (i % 8), friction = 0.6, bounce = 0.05 })
    end
    c.toast("Dominoes: a ball rolls down the ramp and starts the chain")
end

local function wrecking(c)
    local W, A = c.W, c.A
    local ax, ay, L = -250, 250, 420
    local base = 160
    for row = 0, 7 do
        local n = 8 - row
        for i = 0, n - 1 do
            W:Add("box", base + (i - (n - 1) / 2) * 46, A.y0 + 22 + row * 44,
                { w = 44, h = 44, mass = 0.8, friction = 0.7, bounce = 0.05, color = 1 + (row % 8) })
        end
    end
    local a = math.rad(70)
    local ball = W:Add("circle", ax - L * math.sin(a), ay - L * math.cos(a),
        { r = 46, mass = 30, color = VIOLET, bounce = 0.2 })
    W:AddJoint("rod", W:Anchor(ax, ay), ball)
    c.toast("Wrecking ball: one rod joint, 36 boxes")
end

local function plinko(c)
    local W, A = c.W, c.A
    for row = 0, 7 do
        local off = (row % 2) * 45
        for i = 0, 12 do
            local x = -650 + off + i * 90
            if x < A.x1 - 30 then
                W:Add("circle", x, 280 - row * 62, { r = 9, static = true, bounce = 0.5 })
            end
        end
    end
    for x = -650, 500, 100 do
        W:Add("box", x, A.y0 + 55, { w = 10, h = 110, static = true })
    end
    for i = 1, 30 do
        W:Add("circle", rnd(-600, 450), rnd(330, 420), { r = 14, mass = 0.5, bounce = 0.45, color = 1 + (i % 8) })
    end
    c.toast("Plinko: 30 balls and 90 static pegs")
end

local function spinners(c)
    local W, A = c.W, c.A
    local cx = (A.x0 + A.x1) / 2
    local function spinner(x, y, len, speed, torque, color)
        local b1 = W:Add("box", x, y, { w = len, h = 16, mass = 3, color = color })
        local b2 = W:Add("box", x, y, { w = 16, h = len, mass = 3, color = color })
        W:AddJoint("weld", b1, b2)
        W:AddJoint("hinge", b1, W:Anchor(x, y),
            { px = x, py = y, p = { motor = true, mspeed = speed, mtorque = torque } })
    end
    spinner(cx - 220, -120, 340, 2.2, 600000, SKY)
    spinner(cx + 260, -200, 230, -3.5, 300000, CORAL)
    for i = 1, 40 do
        W:Add("circle", rnd(-650, 500), rnd(200, 420), { r = rnd(10, 16), mass = 0.4, bounce = 0.3, color = 1 + (i % 8) })
    end
    c.toast("Spinners: hinges with motors. Select a hinge to change speed and torque")
end

local function chains(c)
    local W = c.W
    local links = W:AddRope(nil, nil, -520, 360, 280, 360, 24, 8)
    if links then
        local mid = links[math.ceil(#links / 2)]
        local mx, my = mid.entity:GetPosition()
        local weight = W:Add("circle", mx, my - 70, { r = 40, mass = 8, color = VIOLET, bounce = 0.2 })
        W:AddJoint("hinge", mid, weight, { px = mx, py = my })
    end
    local ball = W:Add("circle", -300, -110, { r = 40, mass = 10, color = CORAL, bounce = 0.3 })
    W:AddRope(nil, ball, -60, 330, -300, -70, 24, 8)
    c.toast("Chains: use the Rope tool to string your own between any two points")
end

local function zones(c)
    local W, A = c.W, c.A
    for i = 1, 3 do
        W:Add("box", -380 + i * 200, -330 + i * 120, { w = 260, h = 14, static = true, oneWay = true, color = SKY })
    end
    for i = 1, 4 do
        local b = W:Add("circle", -480 + i * 40 + (i * 55), -400, { r = 18, mass = 0.6, color = MINT, bounce = 0.3 })
        velocity(b, rnd(-30, 30), 1100)
    end
    W:Add("box", 380, A.y0 + 50, { w = 220, h = 100, static = true, sensor = true, color = AMBER })
    for i = 1, 4 do
        W:Add("box", 360 + i * 8, 200 + i * 70, { w = 40, h = 40, mass = 1, color = CORAL })
    end
    for i = 1, 14 do
        W:Add("circle", -560 + i * 4, 300 + (i % 3) * 3,
            { r = 20, mass = 0.5, color = PINK, layer = 1, maskMode = 3, bounce = 0.2 })
    end
    W:Add("circle", -600, 120, { r = 26, mass = 1, color = SUN, noGravity = true })
    c.toast("Zones: one-way platforms, a sensor zone, ghost layer, a floating ball")
end

Scenes.list = {
    { name = "Blank",           short = "Just the arena",              build = function() end },
    { name = "Toy Box",         short = "A bit of everything",         build = toyBox },
    { name = "Newton's Cradle", short = "Rods and perfect bounces",    build = cradle },
    { name = "Chaos Pendulum",  short = "Triple hinge with a tracer",  build = chaos },
    { name = "Rope Bridge",     short = "12 planks, 13 hinges",        build = bridge },
    { name = "Car",             short = "Wheel joints, motors, A / D", build = car },
    { name = "Ragdolls",        short = "Hinges with angle limits",    build = ragdolls },
    { name = "Jelly",           short = "Soft bodies from springs",    build = jelly },
    { name = "Catapult",        short = "Lever, counterweight, stack", build = catapult },
    { name = "Dominoes",        short = "A chain reaction",            build = dominoes },
    { name = "Wrecking Ball",   short = "Pyramid of 36 boxes",         build = wrecking },
    { name = "Plinko",          short = "Pegs and 30 balls",           build = plinko },
    { name = "Spinners",        short = "Motorised hinges",            build = spinners },
    { name = "Chains",          short = "Rope tool, hanging weights",  build = chains },
    { name = "Zones & Filters", short = "One-way, sensors, layers",    build = zones },
}

-- =========================================================================
-- MAIN
-- =========================================================================
local Main = {}

local A = World.ARENA

Main.TOOL_GROUPS = {
    { title = "INTERACT", tools = {
        { id = "grab",   label = "Grab",   key = "Q", kind = "grab" },
        { id = "select", label = "Select", key = "V", kind = "select" },
        { id = "erase",  label = "Erase",  key = "X", kind = "erase" },
    } },
    { title = "SHAPES", tools = {
        { id = "box",     label = "Box",     key = "NUM_1", kind = "shape" },
        { id = "circle",  label = "Ball",    key = "NUM_2", kind = "shape" },
        { id = "capsule", label = "Pill",    key = "NUM_3", kind = "shape" },
        { id = "polygon", label = "Polygon", key = "NUM_4", kind = "shape" },
    } },
    { title = "JOINTS", tools = {
        { id = "hinge",  label = "Hinge",  key = "NUM_5", kind = "joint" },
        { id = "spring", label = "Spring", key = "NUM_6", kind = "joint" },
        { id = "rod",    label = "Rod",    key = "NUM_7", kind = "joint" },
        { id = "rope",   label = "Rope",   key = "NUM_8", kind = "joint" },
        { id = "weld",   label = "Weld",   key = "NUM_9", kind = "joint" },
        { id = "wheel",  label = "Wheel",  key = "NUM_0", kind = "joint" },
    } },
    { title = "FORCES", tools = {
        { id = "blast", label = "Blast", key = "E", kind = "force" },
        { id = "pull",  label = "Pull",  key = "T", kind = "force" },
        { id = "ray",   label = "Ray",   key = "R", kind = "force" },
    } },
}

Main.TOOLS = {}
for _, g in ipairs(Main.TOOL_GROUPS) do
    for _, t in ipairs(g.tools) do Main.TOOLS[t.id] = t end
end

local HINTS = {
    grab    = "Drag a body with the mouse spring. Static bodies can be moved too. Shift+click adds to the selection. Right-click inspects.",
    select  = "Drag a box to select several bodies. The Edit tab changes all of them at once. Del removes them.",
    erase   = "Hold the mouse and sweep over bodies or joint handles to delete them.",
    box     = "Click to drop a box. Hold Shift (or turn on Spray) to pour them. Right-click inspects a body.",
    circle  = "Click to drop a ball. Hold Shift (or turn on Spray) to pour them.",
    capsule = "Click to drop a pill (capsule). Hold Shift (or turn on Spray) to pour them.",
    polygon = "Click to drop a regular polygon. Change the number of sides in the Create tab.",
    hinge   = "Click a body where the pin goes, then click a second body. Click empty space to pin to the world.",
    spring  = "Click a body (or empty space for a fixed point), then a second one. Tune Softness in the Edit tab.",
    rod     = "Click two bodies (or empty space for a fixed point) to join them with a rigid rod.",
    rope    = "Click a body or empty space, then another: a rope of hinged links is strung between them.",
    weld    = "Click two bodies to glue them together.",
    wheel   = "Click the chassis, then the wheel (a ball). Drive with the left / right arrow keys.",
    blast   = "Click to set off an explosion. Strength and radius are in the Create tab.",
    pull    = "Hold left mouse to pull bodies toward the cursor, right mouse to push them away.",
    ray     = "Press and drag to cast a ray from the start point. It reports what it hits and the surface normal.",
}

local GRAVITY_PRESETS = {
    { "Earth", 0, -980 }, { "Moon", 0, -160 }, { "Mars", 0, -370 }, { "Jupiter", 0, -2400 },
    { "Zero-G", 0, 0 }, { "Reverse", 0, 980 }, { "Left", -980, 0 }, { "Right", 980, 0 },
}

local function palette_names(withAuto)
    local out = withAuto and { "Auto" } or {}
    for _, p in ipairs(World.PALETTE) do out[#out + 1] = p[1] end
    return out
end

local function segDist(px, py, x1, y1, x2, y2)
    local dx, dy = x2 - x1, y2 - y1
    local l2 = dx * dx + dy * dy
    if l2 < 1e-6 then return dist(px, py, x1, y1) end
    local t = clamp(((px - x1) * dx + (py - y1) * dy) / l2, 0, 1)
    return dist(px, py, x1 + dx * t, y1 + dy * t)
end

local function header(label) return { kind = "header", label = label } end
local function info(fn) return { kind = "info", text = fn } end
local function toggle(label, get, set) return { kind = "toggle", label = label, get = get, set = set } end

local function step(label, get, set, delta, lo, hi, fmt, mult)
    fmt = fmt or "%.2f"
    return {
        kind = "step", label = label,
        value = function() return string.format(fmt, get()) end,
        dec = function() set(clamp(mult and get() / mult or get() - delta, lo, hi)) end,
        inc = function() set(clamp(mult and get() * mult or get() + delta, lo, hi)) end,
    }
end

local function choice(label, names, get, set)
    return {
        kind = "step", label = label,
        value = function() return names[get()] or "?" end,
        dec = function() local i = get() - 1; if i < 1 then i = #names end; set(i) end,
        inc = function() local i = get() + 1; if i > #names then i = 1 end; set(i) end,
    }
end

local function actions(...) return { kind = "actions", items = { ... } } end

function Main:OnCreate()
    math.randomseed(os.time())

    self.cfg = {
        gx = 0, gy = -980, gravIdx = 1, substeps = nil, velIter = nil, posIter = nil, sleep = true,
        wind = false, windX = 400,
        joints = true, grid = true, flash = true, vectors = false, trails = false,
        grab = 14, maxBodies = World.CONFIG.maxBodies,
    }
    self.create = {
        w = 90, h = 60, r = 36, sides = 6, capL = 140, capR = 26,
        mass = 1, static = false, bounce = 0.25, friction = 0.5, angle = 0, color = 0,
        spray = false, snap = false,
    }
    self.jdef = {
        hinge = { motor = false, mspeed = 3, drive = false },
        spring = { softness = 0.01 },
        wheel = { hz = 4, damp = 0.7, mspeed = 14, drive = true },
        rope = { seg = 24, thick = 8 },
    }
    self.force = { blast = 900, blastR = 280, pull = 3000, pullR = 320 }
    self.eraseR = 14

    self.tool, self.tab = "grab", "create"
    self.sel, self.selJoint = {}, nil
    self.fx, self.time, self.fps, self.statT, self.sprayT = {}, 0, 60, 0, 0
    self.modal = false

    self.world = World.new(self)
    self.scenes = Scenes.list
    self.ui = UI.new(self)
    self.ui:Build()
    self:SetupCamera()
    self.world:BuildArena()
    self:SetTool("grab")
    self:ApplyGravity()
    self:LoadPreset(2)
end

function Main:SetupCamera()
    local cam = Scene.Find("Camera")
    if not cam then return end
    pcall(function()
        Scene.Spawn("CameraAnchor", 0, 0)
        cam:GetCamera():SetFollow("CameraAnchor")
    end)
end

function Main:SetTool(id)
    self.tool = id
    self.jp, self.grab, self.box, self.ray = nil, nil, nil, nil
    self.ui:SetActiveTool(id)
    local kind = Main.TOOLS[id].kind
    if kind ~= "grab" and kind ~= "select" and kind ~= "erase" then
        self.tab = "create"
    elseif self.tab == "create" and (#self.sel > 0 or self.selJoint) then
        self.tab = "edit"
    end
    self.ui:SetActiveTab(self.tab)
    self:UpdateHint()
    self.rowsDirty = true
end

function Main:SetTab(tab)
    self.tab = tab
    self.rowsDirty = true
    local ok, err = pcall(function() self.ui:SetActiveTab(tab) end)
    if not ok then
        log.warn("[sandbox] SetActiveTab failed: " .. tostring(err))
    end
end

function Main:UpdateHint()
    local text = HINTS[self.tool] or ""
    if self.jp then text = "Now click the second point or body.   (Esc or right-click cancels)" end
    self.ui:SetText(self.ui.hintLabel, text)
end

function Main:AfterRowEdit(spec)
    if spec.rebuild then self.rowsDirty = true end
end

function Main:Select(list, joint, keepTab)
    self.sel = list or {}
    self.selJoint = joint
    if (#self.sel > 0 or joint) and not keepTab then self:SetTab("edit") end
    self.rowsDirty = true
end

function Main:IsSelected(rec)
    for _, r in ipairs(self.sel) do if r == rec then return true end end
    return false
end

function Main:ToggleSelect(rec)
    for i, r in ipairs(self.sel) do
        if r == rec then table.remove(self.sel, i); self.rowsDirty = true; return end
    end
    self.sel[#self.sel + 1] = rec
    self.selJoint = nil
    self:SetTab("edit")
    self.rowsDirty = true
end

function Main:EachSel(f)
    for _, r in ipairs(self.sel) do if not r.removed then f(r) end end
end

function Main:PickBody(x, y, allowArena)
    local rec = self.world:Find(Physics.QueryPoint(x, y))
    if not rec then
        local best, bestD = nil, 1e9
        for _, e in ipairs(Physics.QueryCircle(x, y, 7)) do
            local r = self.world:Find(e)
            if r and not (r.arena and not allowArena) then
                local rx, ry = r.entity:GetPosition()
                local d = dist(x, y, rx, ry)
                if d < bestD then best, bestD = r, d end
            end
        end
        rec = best
    end
    if rec and rec.arena and not allowArena then return nil end
    return rec
end

function Main:PickJoint(x, y)
    local best, bestD = nil, 1e9
    for _, j in ipairs(self.world.joints) do
        local ax, ay, bx, by, hx, hy = self.world:JointEnds(j)
        local d = dist(x, y, hx, hy) - 4
        if j.kind == "spring" or j.kind == "rod" or j.kind == "wheel" then
            d = math.min(d, segDist(x, y, ax, ay, bx, by) + 2)
        end
        if d < 14 and d < bestD then best, bestD = j, d end
    end
    return best
end

function Main:DeleteSelection()
    if self.selJoint then
        self.world:RemoveJoint(self.selJoint)
        self.selJoint = nil
    end
    local list = {}
    for _, r in ipairs(self.sel) do list[#list + 1] = r end
    for _, r in ipairs(list) do
        if not r.arena and not r.removed then self.world:Remove(r) end
    end
    self.sel = {}
    self.rowsDirty = true
end

function Main:DuplicateSelection()
    local out = {}
    self:EachSel(function(r)
        if self.world:UserBodyCount() < self.cfg.maxBodies then out[#out + 1] = self.world:Duplicate(r, 40, 40) end
    end)
    if #out > 0 then self:Select(out) end
end

function Main:OpenScenes()
    self.modal = true
    self.ui:Show("modal", true)
end

function Main:CloseScenes()
    self.modal = false
    self.ui:Show("modal", false)
end

function Main:ApplyDefaults()
    local c = self.cfg
    c.gx, c.gy, c.gravIdx, c.wind = 0, -980, 1, false
    c.trails = false
    self:ApplyGravity()
end

function Main:ClearScene()
    self.grab, self.jp, self.box, self.ray = nil, nil, nil, nil
    self.world:Clear()
    self.sel, self.selJoint = {}, nil
    self.fx = {}
    self.rowsDirty = true
end

function Main:SetSceneName(name)
    self.sceneName = name
    self.ui:SetText(self.ui.sceneLabel, "Scene:  " .. name)
end

function Main:LoadPreset(i)
    local sc = self.scenes[i]
    if not sc then return end
    self:CloseScenes()
    self:ClearScene()
    self:ApplyDefaults()
    self.source = { kind = "preset", index = i }
    local ctx = { W = self.world, A = A, ctl = self, toast = function(t) self.ui:Toast(t, 4) end }
    sc.build(ctx)
    self:SetSceneName(sc.name)
    self:SetTab("create")
end

function Main:ResetScene()
    local s = self.source
    if s and s.kind == "preset" then self:LoadPreset(s.index)
    elseif s and s.kind == "slot" then self:LoadSlot(s.index)
    else self:ClearAll() end
end

function Main:ClearAll()
    self:ClearScene()
    self.source = nil
    self:SetSceneName("Empty")
    self.ui:Toast("Cleared")
end

function Main:SlotPath(slot) return string.format("sandbox_slot%d.lua", slot) end

function Main:SaveSlot(slot)
    local ok, err = self.world:SaveToFile(self:SlotPath(slot), self.sceneName or "Saved scene")
    self.ui:Toast(ok and ("Saved to slot " .. slot) or ("Could not save: " .. tostring(err)))
end

function Main:LoadSlot(slot)
    self:CloseScenes()
    self:ClearScene()
    local ok, name = self.world:LoadFromFile(self:SlotPath(slot))
    if ok then
        self.source = { kind = "slot", index = slot }
        self:SetSceneName("Slot " .. slot .. " - " .. tostring(name))
        self.ui:Toast("Loaded slot " .. slot)
    else
        self.ui:Toast("Slot " .. slot .. " is empty")
    end
end

function Main:SetTrails(on) self.cfg.trails = on; self.rowsDirty = true end

function Main:ApplyGravity()
    Physics.SetGravity(self.cfg.gx, self.cfg.gy)
end

function Main:SetGravity(x, y)
    self.cfg.gx, self.cfg.gy = x, y
    self.cfg.gravIdx = 0
    for i, g in ipairs(GRAVITY_PRESETS) do
        if g[2] == x and g[3] == y then self.cfg.gravIdx = i end
    end
    self:ApplyGravity()
    Physics.WakeAll()
end

function Main:InField()
    if self.modal then return false end
    local x, y = self.mx, self.my
    return x > A.x0 - 8 and x < A.x1 + 8 and y > A.y0 - 8 and y < A.y1 + 8
end

function Main:OnUpdate(dt)
    dt = math.min(dt, 1 / 20)
    self.time = self.time + dt
    self.fps = self.fps * 0.92 + (1 / math.max(dt, 1e-4)) * 0.08
    self.mx, self.my = self.entity:GetMouseWorld()

    self:PurgeSelection()
    self:HandleKeys()
    self:UpdateTools(dt)
    self:UpdatePassive(dt)

    if self.rowsDirty then
        self.rowsDirty = false
        self.ui:SetRows(self:BuildRows())
    end
    self.ui:Update(dt)
    self.statT = self.statT - dt
    if self.statT <= 0 then
        self.statT = 0.2
        self.ui:RefreshRows()
        self.ui:SetText(self.ui.statsLabel, string.format("Bodies %d    Awake %d    Joints %d    %d fps",
            self.world:UserBodyCount(), Physics.GetAwakeCount(), #self.world.joints, math.floor(self.fps + 0.5)))
    end
end

function Main:PurgeSelection()
    local dirty = false
    for i = #self.sel, 1, -1 do
        if self.sel[i].removed then table.remove(self.sel, i); dirty = true end
    end
    if self.selJoint and self.selJoint.removed then self.selJoint = nil; dirty = true end
    if dirty then self.rowsDirty = true end
end

function Main:HandleKeys()
    if self.modal then
        if Input.IsKeyJustPressed(Key.ESCAPE) then self:CloseScenes() end
        return
    end
    local ctrl = Input.IsKeyDown(Key.LCTRL) or Input.IsKeyDown(Key.RCTRL)
    if not ctrl then
        for _, g in ipairs(Main.TOOL_GROUPS) do
            for _, t in ipairs(g.tools) do
                local k = Key[t.key]
                if k and Input.IsKeyJustPressed(k) then self:SetTool(t.id) end
            end
        end
    else
        if Input.IsKeyJustPressed(Key.D) then self:DuplicateSelection() end
        if Input.IsKeyJustPressed(Key.R) then self:ResetScene() end
    end
    if Input.IsKeyJustPressed(Key.DELETE) or Input.IsKeyJustPressed(Key.BACKSPACE) then self:DeleteSelection() end
    if Input.IsKeyJustPressed(Key.ESCAPE) then
        if self.jp then self.jp = nil; self:UpdateHint()
        else self:Select({}, nil, true) end
    end
end

function Main:UpdateTools(dt)
    local kind = Main.TOOLS[self.tool].kind
    local mx, my = self.mx, self.my
    local field = self:InField()
    local pressed = Input.IsMouseJustPressed(Key.MOUSE_LEFT)
    local down = Input.IsMouseDown(Key.MOUSE_LEFT)
    local released = Input.IsMouseJustReleased(Key.MOUSE_LEFT)
    local rpressed = Input.IsMouseJustPressed(Key.MOUSE_RIGHT)
    local shift = Input.IsKeyDown(Key.LSHIFT) or Input.IsKeyDown(Key.RSHIFT)

    self.hover = nil
    if field and (kind == "grab" or kind == "erase" or kind == "select" or kind == "joint") and not down then
        self.hover = self:PickBody(mx, my, kind == "joint")
    end

    if rpressed and field then
        if self.jp then
            self.jp = nil
            self:UpdateHint()
        elseif kind ~= "force" or self.tool ~= "pull" then
            local j = self:PickJoint(mx, my)
            if j then self:Select({}, j)
            else
                local r = self:PickBody(mx, my, false)
                if r then self:Select({ r }) end
            end
        end
    end

    if kind == "grab" then
        if pressed and field then self:BeginGrab(mx, my, shift) end
        if self.grab then
            if down then self:UpdateGrab(dt) else self.grab = nil end
        end
    elseif kind == "select" then
        if pressed and field then self.box = { x0 = mx, y0 = my } end
        if self.box then
            self.box.x1, self.box.y1 = mx, my
            if released or not down then self:EndBoxSelect(shift) end
        end
    elseif kind == "erase" then
        if down and field then self:EraseAt(mx, my) end
    elseif kind == "shape" then
        if field then
            local spray = self.create.spray or shift
            if pressed then
                self:SpawnAt(mx, my, false)
                self.sprayT = 0.12
            elseif down and spray then
                self.sprayT = self.sprayT - dt
                if self.sprayT <= 0 then
                    self.sprayT = 0.055
                    self:SpawnAt(mx + (math.random() - 0.5) * 18, my + (math.random() - 0.5) * 18, true)
                end
            end
        end
    elseif kind == "joint" then
        if pressed and field then self:JointClick(mx, my) end
    elseif kind == "force" then
        self:UpdateForceTool(dt, pressed, down, field)
    end
end

function Main:BeginGrab(x, y, additive)
    local rec = self:PickBody(x, y, false)
    if not rec then
        if not additive then self:Select({}, nil, true) end
        return
    end
    if additive then self:ToggleSelect(rec)
    elseif not self:IsSelected(rec) or #self.sel > 1 then self:Select({ rec }, nil, self.tab ~= "create") end
    local px, py, ang = self.world:Pose(rec)
    local lx, ly = rot(x - px, y - py, -ang)
    self.grab = { rec = rec, lx = lx, ly = ly, ox = px - x, oy = py - y, static = rec.static }
end

function Main:UpdateGrab(dt)
    local g = self.grab
    local rec = g.rec
    if rec.removed then self.grab = nil; return end
    if g.static then
        rec.entity:SetPosition(self.mx + g.ox, self.my + g.oy)
        return
    end
    local px, py, ang = self.world:Pose(rec)
    local ox, oy = rot(g.lx, g.ly, ang)
    local ax, ay = px + ox, py + oy
    g.ax, g.ay = ax, ay
    local b = rec.body
    local vx, vy = b:GetVelocity()
    local w = self.cfg.grab
    local fx = (self.mx - ax) * w * w - 2 * w * vx
    local fy = (self.my - ay) * w * w - 2 * w * vy
    local len = math.sqrt(fx * fx + fy * fy)
    local limit = 9000
    if len > limit then fx, fy = fx / len * limit, fy / len * limit end
    local m = math.max(rec.mass, 0.05)
    b:AddImpulseAt(fx * m * dt, fy * m * dt, ax, ay)
    b:SetSpin(b:GetSpin() * (1 - math.min(1, 2.5 * dt)))
end

function Main:EndBoxSelect(additive)
    local b = self.box
    self.box = nil
    local list = {}
    if math.abs(b.x1 - b.x0) < 6 and math.abs(b.y1 - b.y0) < 6 then
        local rec = self:PickBody(b.x1, b.y1, false)
        if rec then list = { rec } end
    else
        for _, e in ipairs(Physics.QueryBox(math.min(b.x0, b.x1), math.min(b.y0, b.y1),
                                            math.max(b.x0, b.x1), math.max(b.y0, b.y1))) do
            local r = self.world:Find(e)
            if r and not r.arena and not r.anchor then list[#list + 1] = r end
        end
    end
    if additive then
        for _, r in ipairs(self.sel) do
            local dup = false
            for _, q in ipairs(list) do if q == r then dup = true end end
            if not dup then list[#list + 1] = r end
        end
    end
    self:Select(list, nil, #list == 0)
end

function Main:EraseAt(x, y)
    local hit = false
    for _, e in ipairs(Physics.QueryCircle(x, y, self.eraseR)) do
        local r = self.world:Find(e)
        if r and not r.arena and not r.removed then
            self.world:Remove(r)
            hit = true
        end
    end
    if not hit then
        local j = self:PickJoint(x, y)
        if j then self.world:RemoveJoint(j) end
    end
end

function Main:SpawnAt(x, y, jitter)
    local c = self.create
    if self.world:UserBodyCount() >= self.cfg.maxBodies then
        self.ui:Toast("Body limit reached (" .. self.cfg.maxBodies .. ")")
        return
    end
    if c.snap then x, y = math.floor(x / 20 + 0.5) * 20, math.floor(y / 20 + 0.5) * 20 end
    local o = {
        mass = c.mass, static = c.static, bounce = c.bounce, friction = c.friction,
        angle = jitter and math.random() * 360 or c.angle,
        color = (c.color == 0) and self.world:NextColor() or c.color,
    }
    local shape = self.tool
    if shape == "box" then o.w, o.h = c.w, c.h
    elseif shape == "circle" then o.r = c.r
    elseif shape == "capsule" then o.w, o.h = c.capL, c.capR * 2
    else o.r, o.sides = c.r, c.sides end
    local rec = self.world:Add(shape, x, y, o)
    self.ui:SetText(self.ui.hintLabel, HINTS[self.tool])
    return rec
end

function Main:JointClick(x, y)
    local W, tool = self.world, self.tool
    local rec = self:PickBody(x, y, true)
    local p = self.jp
    if not p then
        if (tool == "weld" or tool == "wheel") and not rec then
            self.ui:Toast("Click a body first")
            return
        end
        self.jp = { rec = rec, x = x, y = y }
        self:UpdateHint()
        return
    end

    self.jp = nil
    self:UpdateHint()
    if not p.rec and not rec then self.ui:Toast("At least one end has to be a body"); return end
    if rec and rec == p.rec then self.ui:Toast("Pick two different bodies"); return end
    if (tool == "weld" or tool == "wheel") and not rec then self.ui:Toast("Click a body for the second end"); return end
    if W:UserBodyCount() + 2 >= self.cfg.maxBodies then self.ui:Toast("Body limit reached"); return end

    local pd = function(kind)
        local t = {}
        for k, v in pairs(self.jdef[kind] or {}) do t[k] = v end
        return t
    end

    local j
    if tool == "hinge" then
        local a = p.rec or W:Anchor(p.x, p.y)
        local b = rec or W:Anchor(p.x, p.y)
        j = W:AddJoint("hinge", a, b, { px = p.x, py = p.y, p = pd("hinge") })
    elseif tool == "spring" or tool == "rod" then
        local a = p.rec or W:Anchor(p.x, p.y)
        local b = rec or W:Anchor(x, y)
        j = W:AddJoint(tool, a, b, { ax = p.x, ay = p.y, bx = x, by = y, p = pd(tool) })
    elseif tool == "weld" then
        j = W:AddJoint("weld", p.rec, rec)
    elseif tool == "wheel" then
        local wx, wy = rec.entity:GetPosition()
        j = W:AddJoint("wheel", p.rec, rec, { px = wx, py = wy, p = pd("wheel") })
    elseif tool == "rope" then
        local d = self.jdef.rope
        local links = W:AddRope(p.rec, rec, p.x, p.y, x, y, d.seg, d.thick)
        if links then self.ui:Toast("Rope with " .. #links .. " links") else self.ui:Toast("Too short for a rope") end
        return
    end
    if j then
        self:Select({}, j)
        self.ui:Toast(World.KIND_NAMES[j.kind] .. " created - tune it in the Edit tab")
    else
        self.ui:Toast("The engine refused that joint")
    end
end

function Main:AddFx(x, y, r0, r1, dur, color)
    self.fx[#self.fx + 1] = { x = x, y = y, r0 = r0, r1 = r1, t = 0, dur = dur, color = color }
end

function Main:UpdateForceTool(dt, pressed, down, field)
    local f, mx, my = self.force, self.mx, self.my
    if self.tool == "blast" then
        if pressed and field then
            local R, S = f.blastR, f.blast
            for _, e in ipairs(Physics.QueryCircle(mx, my, R)) do
                local r = self.world:Find(e)
                if r and not r.static and not r.removed then
                    local x, y = r.entity:GetPosition()
                    local d = math.max(12, dist(mx, my, x, y))
                    local fall = clamp(1 - d / R, 0.12, 1)
                    local k = math.max(r.mass, 0.05) * S * fall / d
                    r.body:AddImpulse((x - mx) * k, (y - my) * k)
                end
            end
            self:AddFx(mx, my, 10, R, 0.4, { 255, 190, 90 })
            self:AddFx(mx, my, 4, R * 0.6, 0.28, { 255, 120, 80 })
        end
    elseif self.tool == "pull" then
        local rdown = Input.IsMouseDown(Key.MOUSE_RIGHT)
        if (down or rdown) and field then
            local R, S = f.pullR, f.pull
            local sign = rdown and -1 or 1
            for _, e in ipairs(Physics.QueryCircle(mx, my, R)) do
                local r = self.world:Find(e)
                if r and not r.static and not r.removed then
                    local x, y = r.entity:GetPosition()
                    local d = math.max(20, dist(mx, my, x, y))
                    local fall = clamp(1 - d / R, 0.15, 1)
                    local k = math.max(r.mass, 0.05) * S * fall / d * sign
                    r.body:AddForce((mx - x) * k, (my - y) * k)
                end
            end
        end
    elseif self.tool == "ray" then
        if pressed and field then self.ray = { x0 = mx, y0 = my, live = true } end
        local r = self.ray
        if r and r.live then
            if down then
                r.x1, r.y1 = mx, my
                local len = dist(r.x0, r.y0, mx, my)
                r.hit = nil
                if len > 3 then
                    local dx, dy = Math.Normalize(mx - r.x0, my - r.y0)
                    r.dx, r.dy = dx, dy
                    local e, hx, hy, nx, ny, d = Physics.Raycast(r.x0, r.y0, dx, dy, 4000)
                    if e then r.hit = { rec = self.world:Find(e), x = hx, y = hy, nx = nx, ny = ny, d = d } end
                end
            else
                r.live = false
                if r.hit then
                    local who = r.hit.rec and ("body #" .. r.hit.rec.id) or "something"
                    self.ui:Toast(string.format("Ray hit %s after %.0f px, normal (%.2f, %.2f)",
                        who, r.hit.d, r.hit.nx, r.hit.ny), 3)
                end
            end
        end
    end
end

function Main:UpdatePassive(dt)
    local W, cfg = self.world, self.cfg

    if cfg.wind then
        for _, r in ipairs(W.bodies) do
            if not r.static then r.body:AddForce(math.max(r.mass, 0.05) * cfg.windX, 0) end
        end
    end

    local dir = Input.GetAxis(Key.LEFT, Key.RIGHT)
    W:Drive(dir)

    for _, r in ipairs(W.bodies) do
        if r.hit > 0 then r.hit = math.max(0, r.hit - dt * 3.5) end
    end

    for i = #self.fx, 1, -1 do
        local f = self.fx[i]
        f.t = f.t + dt
        if f.t >= f.dur then table.remove(self.fx, i) end
    end

    -- Trail sampling
    self.trailT = (self.trailT or 0) - dt
    if self.trailT <= 0 then
        self.trailT = 0.02
        for _, r in ipairs(W.bodies) do
            local track = cfg.trails and (r.tracer or self:IsSelected(r))
            if track and not r.static then
                local x, y = r.entity:GetPosition()
                local t = r.trail
                t[#t + 1] = x
                t[#t + 1] = y
                if #t > 260 then table.remove(t, 1); table.remove(t, 1) end
            elseif #r.trail > 0 then
                r.trail = {}
            end
        end
    end

    -- Poll sensor bodies for "inside" count (replaces body.lua OnTrigger*)
    for _, r in ipairs(W.bodies) do
        if r.sensor and not r.anchor and not r.removed then
            local x, y = r.entity:GetPosition()
            local list
            if r.shape == "circle" then
                list = Physics.QueryCircle(x, y, r.r)
            else
                local hw = (r.vw or r.w) * 0.5
                local hh = (r.vh or r.h) * 0.5
                list = Physics.QueryBox(x - hw, y - hh, x + hw, y + hh)
            end
            local inside = 0
            for _, e in ipairs(list or {}) do
                if e ~= r.entity then inside = inside + 1 end
            end
            local wasInside = r.inside > 0
            r.inside = inside
            if wasInside ~= (inside > 0) then W:Style(r) end
        end
    end

    -- Approximate contact flash via wake-from-sleep transition (replaces body.lua OnCollision)
    for _, r in ipairs(W.bodies) do
        if not r.static and not r.arena and not r.anchor and r.body then
            local ok, awake = pcall(r.body.IsAwake, r.body)
            if ok then
                if awake and r._wasAsleep then r.hit = 1 end
                r._wasAsleep = not awake
            end
        end
    end
end

function Main:OnBodyCollision(entity, other)
    if not self.cfg.flash then return end
    local r = self.world:Find(entity)
    if r then r.hit = 1 end
end

function Main:OnSensor(entity, other, delta)
    local r = self.world:Find(entity)
    if not r or r.removed then return end
    local before = r.inside
    r.inside = math.max(0, r.inside + delta)
    if (before > 0) ~= (r.inside > 0) then self.world:Style(r) end
end

function Main:BuildRows()
    if self.tab == "world" then return self:WorldRows() end
    if self.tab == "edit" then
        if self.selJoint then return self:JointRows(self.selJoint) end
        if #self.sel > 0 then return self:BodyRows() end
        return {
            header("NOTHING SELECTED"),
            info(function() return "Click a body to edit it," end),
            info(function() return "or click a joint handle (the pin," end),
            info(function() return "coil or rod) to tune that joint." end),
            info(function() return "Right-click inspects with any tool." end),
        }
    end
    return self:CreateRows()
end

function Main:CreateRows()
    local c, t = self.create, self.tool
    local kind = Main.TOOLS[t].kind
    local rows = {}
    local function add(r) rows[#rows + 1] = r end

    if kind == "shape" then
        add(header(string.upper(Main.TOOLS[t].label) .. " SETTINGS"))
        if t == "box" then
            add(step("Width", function() return c.w end, function(v) c.w = v end, 10, 10, 600, "%.0f"))
            add(step("Height", function() return c.h end, function(v) c.h = v end, 10, 10, 600, "%.0f"))
        elseif t == "circle" then
            add(step("Radius", function() return c.r end, function(v) c.r = v end, 4, 5, 200, "%.0f"))
        elseif t == "capsule" then
            add(step("Length", function() return c.capL end, function(v) c.capL = v end, 10, 30, 600, "%.0f"))
            add(step("Radius", function() return c.capR end, function(v) c.capR = v end, 2, 6, 100, "%.0f"))
        else
            add(step("Sides", function() return c.sides end, function(v) c.sides = v end, 1, 3, 8, "%.0f"))
            add(step("Radius", function() return c.r end, function(v) c.r = v end, 4, 10, 200, "%.0f"))
        end
        add(step("Mass", function() return c.mass end, function(v) c.mass = v end, 0, 0.05, 500, "%.2f", 1.25))
        add(toggle("Static (anchored)", function() return c.static end, function(v) c.static = v end))
        add(step("Bounce", function() return c.bounce end, function(v) c.bounce = v end, 0.05, 0, 1.2, "%.2f"))
        add(step("Friction", function() return c.friction end, function(v) c.friction = v end, 0.05, 0, 2, "%.2f"))
        add(step("Angle", function() return c.angle end, function(v) c.angle = v end, 15, -180, 180, "%.0f"))
        add(choice("Colour", palette_names(true), function() return c.color + 1 end, function(i) c.color = i - 1 end))
        add(toggle("Spray while held", function() return c.spray end, function(v) c.spray = v end))
        add(toggle("Snap to grid", function() return c.snap end, function(v) c.snap = v end))
        add(info(function() return "Shift also sprays. Max " .. self.cfg.maxBodies .. " bodies." end))
    elseif kind == "joint" then
        local d = self.jdef
        add(header(string.upper(self:JointToolName(t)) .. " DEFAULTS"))
        if t == "hinge" then
            add(toggle("Motor on", function() return d.hinge.motor end, function(v) d.hinge.motor = v end))
            add(step("Motor speed", function() return d.hinge.mspeed end, function(v) d.hinge.mspeed = v end, 0.5, -30, 30, "%.1f"))
            add(toggle("Arrow-key drive", function() return d.hinge.drive end, function(v) d.hinge.drive = v end))
        elseif t == "spring" then
            add(step("Softness", function() return d.spring.softness end, function(v) d.spring.softness = v end, 0, 0.0001, 1, "%.4f", 1.5))
            add(info(function() return "Higher softness = looser spring." end))
        elseif t == "wheel" then
            add(step("Spring Hz", function() return d.wheel.hz end, function(v) d.wheel.hz = v end, 0.5, 0.5, 30, "%.1f"))
            add(step("Damping", function() return d.wheel.damp end, function(v) d.wheel.damp = v end, 0.1, 0, 2, "%.1f"))
            add(step("Motor speed", function() return d.wheel.mspeed end, function(v) d.wheel.mspeed = v end, 2, 1, 80, "%.0f"))
            add(toggle("Arrow-key drive", function() return d.wheel.drive end, function(v) d.wheel.drive = v end))
        elseif t == "rope" then
            add(step("Link length", function() return d.rope.seg end, function(v) d.rope.seg = v end, 4, 12, 80, "%.0f"))
            add(step("Thickness", function() return d.rope.thick end, function(v) d.rope.thick = v end, 2, 4, 24, "%.0f"))
        else
            add(info(function() return "No settings. Click two bodies." end))
        end
        add(info(function() return "Everything else is tuned after creation:" end))
        add(info(function() return "select the joint and open the Edit tab." end))
    elseif kind == "force" then
        local f = self.force
        add(header(string.upper(Main.TOOLS[t].label)))
        if t == "blast" then
            add(step("Strength", function() return f.blast end, function(v) f.blast = v end, 0, 100, 6000, "%.0f", 1.25))
            add(step("Radius", function() return f.blastR end, function(v) f.blastR = v end, 20, 60, 700, "%.0f"))
        elseif t == "pull" then
            add(step("Strength", function() return f.pull end, function(v) f.pull = v end, 0, 200, 20000, "%.0f", 1.25))
            add(step("Radius", function() return f.pullR end, function(v) f.pullR = v end, 20, 60, 700, "%.0f"))
        else
            add(info(function() return "Physics.Raycast from the press point" end))
            add(info(function() return "toward the cursor. Release to read it." end))
        end
    else
        add(header(string.upper(Main.TOOLS[t].label)))
        if t == "grab" then
            add(step("Grab strength", function() return self.cfg.grab end, function(v) self.cfg.grab = v end, 2, 4, 40, "%.0f"))
        elseif t == "erase" then
            add(step("Brush radius", function() return self.eraseR end, function(v) self.eraseR = v end, 4, 6, 120, "%.0f"))
        else
            add(info(function() return "Drag a rectangle over bodies." end))
        end
        add(info(function() return "Pick a shape or joint tool on the left," end))
        add(info(function() return "or press 1-9, 0, E, T, R, Q, V, X." end))
    end
    return rows
end

function Main:JointToolName(t)
    return World.KIND_NAMES[t] or (t == "rope" and "Rope") or t
end

function Main:BodyRows()
    local W = self.world
    local p = self.sel[1]
    local function each(f) self:EachSel(f) end
    local n = #self.sel
    local rows = {}
    local function add(r) rows[#rows + 1] = r end

    add(header(string.format("%s #%d%s", string.upper(p.shape), p.id, n > 1 and ("  (+" .. (n - 1) .. " more)") or "")))
    add(info(function()
        local vx, vy = p.body:GetVelocity()
        return string.format("v %.0f, %.0f   spin %.1f   %s", vx, vy, p.body:GetSpin(),
            p.body:IsAwake() and "awake" or "asleep")
    end))
    add(toggle("Static", function() return p.static end, function(v) each(function(r) W:SetStatic(r, v) end) end))
    add(step("Mass", function() return p.mass end, function(v) each(function(r) W:SetMass(r, v) end) end, 0, 0.05, 500, "%.2f", 1.25))
    add(step("Bounce", function() return p.bounce end, function(v) each(function(r) W:SetBounce(r, v) end) end, 0.05, 0, 1.2, "%.2f"))
    add(step("Friction", function() return p.friction end, function(v) each(function(r) W:SetFriction(r, v) end) end, 0.05, 0, 2, "%.2f"))
    add(step("Linear damping", function() return p.linDamp or 1 end,
        function(v) each(function(r) W:SetDamping(r, v, r.angDamp or 1) end) end, 0.05, 0, 1, "%.2f"))
    add(step("Angular damping", function() return p.angDamp or 1 end,
        function(v) each(function(r) W:SetDamping(r, r.linDamp or 1, v) end) end, 0.05, 0, 1, "%.2f"))
    add(toggle("Sensor", function() return p.sensor end, function(v) each(function(r) W:SetSensor(r, v) end) end))
    add(toggle("Lock rotation", function() return p.lockRot end, function(v) each(function(r) W:SetLocks(r, v, r.lockPos) end) end))
    add(toggle("Lock position", function() return p.lockPos end, function(v) each(function(r) W:SetLocks(r, r.lockRot, v) end) end))
    add(toggle("No gravity", function() return p.noGravity end, function(v) each(function(r) W:SetNoGravity(r, v) end) end))
    add(toggle("One-way platform", function() return p.oneWay end, function(v) each(function(r) W:SetOneWay(r, v) end) end))
    add(toggle("Can sleep", function() return p.canSleep end, function(v) each(function(r) W:SetCanSleep(r, v) end) end))
    add(step("Layer", function() return p.layer end, function(v) each(function(r) W:SetLayer(r, v) end) end, 1, 0, 15, "%.0f"))
    add(choice("Collides with", World.MASK_MODES, function() return p.maskMode end, function(i) each(function(r) W:SetMaskMode(r, i) end) end))
    add(choice("Colour", palette_names(false), function() return p.color end, function(i) each(function(r) W:SetColor(r, i) end) end))
    add(actions(
        { label = "Launch", fn = function() each(function(r) if not r.static then r.body:AddImpulse(0, math.max(r.mass, 0.05) * 700) end end) end },
        { label = "Spin", fn = function() each(function(r) if not r.static then r.body:SetSpin(10) end end) end },
        { label = "Stop", fn = function() each(function(r) r.body:Stop() end) end }))
    add(actions(
        { label = "Wake / Sleep", fn = function() each(function(r) if r.body:IsAwake() then r.body:Sleep() else r.body:Wake() end end) end },
        { label = "Duplicate", tone = "primary", fn = function() self:DuplicateSelection() end },
        { label = "Delete", tone = "danger", fn = function() self:DeleteSelection() end }))
    return rows
end

function Main:JointRows(j)
    local W = self.world
    local p = j.p
    local rows = {}
    local function add(r) rows[#rows + 1] = r end
    local function apply() W:ApplyJoint(j) end
    local function set(key) return function(v) p[key] = v; apply() end end

    add(header(string.format("%s JOINT #%d", string.upper(World.KIND_NAMES[j.kind]), j.id)))
    add(info(function()
        local ax, ay, bx, by = W:JointEnds(j)
        if j.kind == "hinge" then
            local ok, a = pcall(j.h.GetAngle, j.h)
            return string.format("bodies #%d, #%d   angle %.1f", j.a.id, j.b.id, ok and a or 0)
        elseif j.kind == "wheel" then
            local ok, t = pcall(j.h.GetTranslation, j.h)
            return string.format("bodies #%d, #%d   travel %.1f", j.a.id, j.b.id, ok and t or 0)
        end
        return string.format("bodies #%d, #%d   length %.0f", j.a.id, j.b.id, dist(ax, ay, bx, by))
    end))

    if j.kind == "hinge" then
        add(toggle("Motor", function() return p.motor end, set("motor")))
        add(step("Motor speed", function() return p.mspeed end, set("mspeed"), 0.5, -40, 40, "%.1f"))
        add(step("Max torque", function() return p.mtorque end, set("mtorque"), 0, 100, 1e9, "%.0f", 1.5))
        add(toggle("Arrow-key drive", function() return p.drive end, set("drive")))
        add(toggle("Angle limits", function() return p.limit end, set("limit")))
        add(step("Min angle", function() return p.lmin end, function(v) p.lmin = math.min(v, p.lmax); apply() end, 5, -360, 360, "%.0f"))
        add(step("Max angle", function() return p.lmax end, function(v) p.lmax = math.max(v, p.lmin); apply() end, 5, -360, 360, "%.0f"))
    elseif j.kind == "spring" then
        add(step("Rest length", function() return p.length end, set("length"), 10, 5, 2000, "%.0f"))
        add(step("Softness", function() return p.softness end, set("softness"), 0, 0.00001, 10, "%.5f", 1.5))
        add(toggle("Length limits", function() return p.limit end, set("limit")))
        add(step("Min length", function() return p.lmin end, set("lmin"), 10, 0, 2000, "%.0f"))
        add(step("Max length", function() return p.lmax end, set("lmax"), 10, 0, 4000, "%.0f"))
    elseif j.kind == "rod" then
        add(step("Length", function() return p.length end, set("length"), 10, 5, 2000, "%.0f"))
    elseif j.kind == "wheel" then
        add(step("Spring Hz", function() return p.hz end, set("hz"), 0.5, 0.5, 40, "%.1f"))
        add(step("Spring damping", function() return p.damp end, set("damp"), 0.1, 0, 3, "%.1f"))
        add(toggle("Motor", function() return p.motor end, set("motor")))
        add(step("Motor speed", function() return p.mspeed end, set("mspeed"), 2, -120, 120, "%.0f"))
        add(step("Max torque", function() return p.mtorque end, set("mtorque"), 0, 100, 1e9, "%.0f", 1.5))
        add(toggle("Arrow-key drive", function() return p.drive end, set("drive")))
        add(toggle("Travel limits", function() return p.limit end, set("limit")))
        add(step("Min travel", function() return p.lmin end, function(v) p.lmin = math.min(v, p.lmax); apply() end, 5, -300, 300, "%.0f"))
        add(step("Max travel", function() return p.lmax end, function(v) p.lmax = math.max(v, p.lmin); apply() end, 5, -300, 300, "%.0f"))
    else
        add(info(function() return "A weld has no settings." end))
    end
    add(toggle("Bodies collide", function() return p.collide end, set("collide")))
    add(actions(
        { label = "Select A", fn = function() self:Select({ j.a }) end },
        { label = "Select B", fn = function() self:Select({ j.b }) end },
        { label = "Delete", tone = "danger", fn = function() self:DeleteSelection() end }))
    return rows
end

function Main:WorldRows()
    local cfg = self.cfg
    local rows = {}
    local function add(r) rows[#rows + 1] = r end
    local names = {}
    for i, g in ipairs(GRAVITY_PRESETS) do names[i] = g[1] end
    names[0] = "Custom"

    add(header("GRAVITY"))
    add({
        kind = "step", label = "Preset",
        value = function() return names[cfg.gravIdx] end,
        dec = function() local i = cfg.gravIdx - 1; if i < 1 then i = #GRAVITY_PRESETS end; local g = GRAVITY_PRESETS[i]; self:SetGravity(g[2], g[3]) end,
        inc = function() local i = cfg.gravIdx + 1; if i > #GRAVITY_PRESETS then i = 1 end; local g = GRAVITY_PRESETS[i]; self:SetGravity(g[2], g[3]) end,
    })
    add(step("Gravity X", function() return cfg.gx end, function(v) self:SetGravity(v, cfg.gy) end, 100, -4000, 4000, "%.0f"))
    add(step("Gravity Y", function() return cfg.gy end, function(v) self:SetGravity(cfg.gx, v) end, 100, -4000, 4000, "%.0f"))

    add(header("SOLVER"))
    add(step("Substeps", function() return cfg.substeps or Physics.GetSubsteps() end,
        function(v) cfg.substeps = v; Physics.SetSubsteps(v) end, 1, 1, 24, "%.0f"))
    add(step("Velocity iterations", function() return cfg.velIter or 8 end,
        function(v) cfg.velIter = v; Physics.SetIterations(v, cfg.posIter or 3) end, 1, 1, 40, "%.0f"))
    add(step("Position iterations", function() return cfg.posIter or 3 end,
        function(v) cfg.posIter = v; Physics.SetIterations(cfg.velIter or 8, v) end, 1, 1, 40, "%.0f"))
    add(toggle("Sleeping", function() return cfg.sleep end, function(v) cfg.sleep = v; Physics.SetSleepEnabled(v) end))
    add(actions(
        { label = "Wake all", fn = function() Physics.WakeAll() end },
        { label = "Sleep all", fn = function() for _, r in ipairs(self.world.bodies) do if not r.static then r.body:Sleep() end end end }))

    add(header("WIND"))
    add(toggle("Wind", function() return cfg.wind end, function(v) cfg.wind = v end))
    add(step("Wind strength", function() return cfg.windX end, function(v) cfg.windX = v end, 100, -3000, 3000, "%.0f"))

    add(header("DISPLAY"))
    add(toggle("Show joints", function() return cfg.joints end, function(v) cfg.joints = v end))
    add(toggle("Grid", function() return cfg.grid end, function(v) cfg.grid = v end))
    add(toggle("Contact flash", function() return cfg.flash end, function(v) cfg.flash = v end))
    add(toggle("Velocity arrows", function() return cfg.vectors end, function(v) cfg.vectors = v end))
    add(toggle("Trails (selected)", function() return cfg.trails end, function(v) cfg.trails = v end))
    add(info(function()
        return string.format("engine: %d bodies, %d awake", Physics.GetBodyCount(), Physics.GetAwakeCount())
    end))
    return rows
end

-- =========================================================================
-- Rendering
-- =========================================================================
local function coil(x1, y1, x2, y2, turns, amp)
    local dx, dy = x2 - x1, y2 - y1
    local len = math.sqrt(dx * dx + dy * dy)
    if len < 1 then return { x1, y1, x2, y2 } end
    local ux, uy = dx / len, dy / len
    local nx, ny = -uy, ux
    local lead = math.min(14, len * 0.15)
    local sx, sy = x1 + ux * lead, y1 + uy * lead
    local body = len - 2 * lead
    local pts = { x1, y1, sx, sy }
    local n = turns * 2
    for i = 1, n do
        local t = (i - 0.5) / n
        local s = (i % 2 == 0) and amp or -amp
        pts[#pts + 1] = sx + ux * body * t + nx * s
        pts[#pts + 1] = sy + uy * body * t + ny * s
    end
    pts[#pts + 1] = x2 - ux * lead
    pts[#pts + 1] = y2 - uy * lead
    pts[#pts + 1] = x2
    pts[#pts + 1] = y2
    return pts
end

local function mix(a, b, t)
    return { a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t, a[3] + (b[3] - a[3]) * t }
end

function Main:OnRender()
    local W, cfg = self.world, self.cfg
    self:DrawBackground()

    Draw.SetZ(-1)
    for _, r in ipairs(W.bodies) do
        if #r.trail >= 6 then Draw.Polyline(r.trail, 2.5, 150, 170, 255, 150) end
    end

    Draw.SetZ(2)
    for _, r in ipairs(W.bodies) do
        if r.shape == "circle" and not r.anchor and not r.static then
            local x, y, a = W:Pose(r)
            local ox, oy = rot(r.r * 0.78, 0, a)
            local c = World.Shade(W:BaseColor(r), -0.35)
            Draw.Line(x, y, x + ox, y + oy, 3, c[1], c[2], c[3], 170)
        end
    end

    if cfg.vectors then
        Draw.SetZ(3)
        for _, r in ipairs(W.bodies) do
            if not r.static then
                local x, y = r.entity:GetPosition()
                local vx, vy = r.body:GetVelocity()
                if vx * vx + vy * vy > 400 then
                    Draw.Line(x, y, x + vx * 0.12, y + vy * 0.12, 2, 90, 230, 255, 200)
                end
            end
        end
    end

    if cfg.joints then
        Draw.SetZ(6)
        self:DrawJoints()
    end

    Draw.SetZ(7)
    for _, r in ipairs(W.bodies) do
        if r.hit > 0 and not r.arena then
            local x, y, a = W:Pose(r)
            Draw.Polyline(World.Outline(r.shape, r.vw, r.vh, r.r, r.sides, x, y, a),
                3, 255, 255, 255, math.floor(210 * r.hit), true)
        end
    end

    Draw.SetZ(8)
    for _, r in ipairs(self.sel) do self:DrawOutline(r, 255, 255, 255, 235, 3) end
    if self.hover and not self:IsSelected(self.hover) then self:DrawOutline(self.hover, 255, 255, 255, 90, 2) end

    Draw.SetZ(9)
    self:DrawToolOverlay()
    for _, f in ipairs(self.fx) do
        local t = f.t / f.dur
        local e = 1 - (1 - t) ^ 3
        Draw.Ring(f.x, f.y, f.r0 + (f.r1 - f.r0) * e, 6 * (1 - t) + 1,
            f.color[1], f.color[2], f.color[3], math.floor(230 * (1 - t)))
    end
end

function Main:DrawOutline(r, cr, cg, cb, ca, w)
    local x, y, a = self.world:Pose(r)
    Draw.Polyline(World.Outline(r.shape, r.vw, r.vh, r.r, r.sides, x, y, a), w, cr, cg, cb, ca, true)
end

function Main:DrawBackground()
    local cx, cy = (A.x0 + A.x1) / 2, (A.y0 + A.y1) / 2
    local w, h = A.x1 - A.x0, A.y1 - A.y0
    Draw.SetZ(-20)
    Draw.Rect(0, 0, 7000, 4000, 8, 10, 18)
    Draw.SetZ(-10)
    Draw.Rect(cx, cy, w, h, 17, 21, 34)
    if self.cfg.grid then
        Draw.SetZ(-9)
        local step = 80
        local x = A.x0 + step
        local i = 1
        while x < A.x1 do
            Draw.Line(x, A.y0, x, A.y1, (i % 5 == 0) and 1.6 or 1, 120, 140, 200, (i % 5 == 0) and 26 or 13)
            x = x + step
            i = i + 1
        end
        local y = A.y0 + step
        i = 1
        while y < A.y1 do
            Draw.Line(A.x0, y, A.x1, y, (i % 5 == 0) and 1.6 or 1, 120, 140, 200, (i % 5 == 0) and 26 or 13)
            y = y + step
            i = i + 1
        end
    end
    Draw.SetZ(-8)
    Draw.Line(A.x0, A.y0, A.x1, A.y0, 3, 104, 120, 255, 90)
    Draw.Line(A.x0, A.y1, A.x1, A.y1, 3, 104, 120, 255, 40)
end

function Main:DrawJoints()
    local W = self.world
    for _, j in ipairs(W.joints) do
        local ax, ay, bx, by, hx, hy = W:JointEnds(j)
        local sel = (j == self.selJoint)
        local k, p = j.kind, j.p
        if k == "hinge" then
            Draw.Circle(hx, hy, 12, 18, 22, 36, 255)
            Draw.Ring(hx, hy, 10, 3, 255, 255, 255, sel and 255 or 215)
            Draw.Circle(hx, hy, 3.5, 104, 120, 255, 255)
            if p.motor or p.drive then Draw.Ring(hx, hy, 19, 2, 40, 200, 160, 220) end
            if p.limit then Draw.Ring(hx, hy, 26, 1.5, 255, 190, 90, 150) end
            if sel then Draw.Ring(hx, hy, 32, 2, 104, 120, 255, 255) end
        elseif k == "spring" or k == "wheel" then
            local len = dist(ax, ay, bx, by)
            local rest = (k == "spring") and p.length or math.max(len, 1)
            local strain = (k == "spring") and clamp(math.abs(len - rest) / math.max(rest, 1) * 1.6, 0, 1) or 0
            local c = mix({ 60, 220, 190 }, { 255, 110, 90 }, strain)
            local turns = clamp(math.floor(len / 14), 3, 14)
            Draw.Polyline(coil(ax, ay, bx, by, turns, k == "wheel" and 9 or 11), sel and 4 or 3, c[1], c[2], c[3], 235)
            Draw.Circle(ax, ay, 5, 18, 22, 36, 255)
            Draw.Ring(ax, ay, 5, 2, c[1], c[2], c[3], 255)
            if k == "wheel" then
                Draw.Ring(bx, by, 10, 3, 255, 255, 255, 200)
                if p.motor or p.drive then Draw.Ring(bx, by, 17, 2, 40, 200, 160, 220) end
            else
                Draw.Circle(bx, by, 5, 18, 22, 36, 255)
                Draw.Ring(bx, by, 5, 2, c[1], c[2], c[3], 255)
            end
            if sel then Draw.Ring(hx, hy, 18, 2, 104, 120, 255, 255) end
        elseif k == "rod" then
            Draw.Line(ax, ay, bx, by, 6, 18, 22, 36, 255)
            Draw.Line(ax, ay, bx, by, 3.5, 200, 212, 240, 255)
            Draw.Circle(ax, ay, 5, 236, 241, 255, 255)
            Draw.Circle(bx, by, 5, 236, 241, 255, 255)
            if sel then Draw.Ring(hx, hy, 14, 2, 104, 120, 255, 255) end
        else
            Draw.Line(ax, ay, bx, by, 2, 255, 190, 90, 150)
            Draw.Rect(hx, hy, 13, 13, 255, 190, 90, 255, 45)
            Draw.Rect(hx, hy, 6, 6, 18, 22, 36, 255, 45)
            if sel then Draw.Ring(hx, hy, 16, 2, 104, 120, 255, 255) end
        end
    end
end

function Main:DrawToolOverlay()
    local W = self.world
    local mx, my = self.mx, self.my
    local field = self:InField()
    local tool = self.tool
    local kind = Main.TOOLS[tool].kind
    local acc = UI.THEME.accent

    local g = self.grab
    if g and not g.static and g.ax then
        Draw.Line(g.ax, g.ay, mx, my, 3, acc[1], acc[2], acc[3], 200)
        Draw.Circle(g.ax, g.ay, 5, 255, 255, 255, 255)
        Draw.Ring(mx, my, 9, 2, acc[1], acc[2], acc[3], 255)
    end

    local b = self.box
    if b and b.x1 then
        Draw.Rect((b.x0 + b.x1) / 2, (b.y0 + b.y1) / 2, math.abs(b.x1 - b.x0), math.abs(b.y1 - b.y0),
            acc[1], acc[2], acc[3], 34)
        Draw.Polyline({ b.x0, b.y0, b.x1, b.y0, b.x1, b.y1, b.x0, b.y1 }, 2, acc[1], acc[2], acc[3], 220, true)
    end

    if kind == "shape" and field then
        local c = self.create
        local col = (c.color == 0) and { 200, 210, 240 } or World.PALETTE[c.color][2]
        local w, h = c.w, c.h
        if tool == "capsule" then w, h = c.capL, c.capR * 2 end
        Draw.Polyline(World.Outline(tool, w, h, c.r, c.sides, mx, my, c.angle), 2.5, col[1], col[2], col[3], 170, true)
    end

    local p = self.jp
    if p then
        Draw.Line(p.x, p.y, mx, my, 2.5, acc[1], acc[2], acc[3], 200)
        Draw.Circle(p.x, p.y, 6, 255, 255, 255, 255)
        Draw.Ring(p.x, p.y, 12, 2, acc[1], acc[2], acc[3], 255)
        if p.rec and not p.rec.removed then self:DrawOutline(p.rec, acc[1], acc[2], acc[3], 255, 3) end
    end

    if field then
        if tool == "blast" then Draw.Ring(mx, my, self.force.blastR, 2, 255, 190, 90, 90)
        elseif tool == "pull" then
            Draw.Ring(mx, my, self.force.pullR, 2, 104, 120, 255, 90)
            Draw.Circle(mx, my, 4, 104, 120, 255, 255)
        elseif tool == "erase" then Draw.Ring(mx, my, self.eraseR, 2, 255, 100, 120, 200) end
    end

    local r = self.ray
    if r and r.x1 then
        local ex, ey = r.x1, r.y1
        if r.hit then ex, ey = r.hit.x, r.hit.y
        elseif r.dx then ex, ey = r.x0 + r.dx * 3000, r.y0 + r.dy * 3000 end
        local alpha = r.live and 255 or 150
        Draw.Line(r.x0, r.y0, ex, ey, 2.5, 255, 90, 110, alpha)
        Draw.Circle(r.x0, r.y0, 6, 255, 255, 255, alpha)
        if r.hit then
            Draw.Ring(ex, ey, 9, 2, 255, 255, 255, alpha)
            Draw.Line(ex, ey, ex + r.hit.nx * 46, ey + r.hit.ny * 46, 3, 90, 230, 255, alpha)
            Draw.Circle(ex + r.hit.nx * 46, ey + r.hit.ny * 46, 4, 90, 230, 255, alpha)
            if r.hit.rec then self:DrawOutline(r.hit.rec, 255, 90, 110, alpha, 3) end
        end
    end
end

return Main
