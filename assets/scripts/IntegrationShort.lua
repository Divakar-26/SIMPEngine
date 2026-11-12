local S = {}

local function f(x)
    return 1.8 + 1.2 * math.sin(1.2 * x) + 0.25 * x
end

local STAGES = {4, 8, 16, 32, 64, 128, 256, 512}

local tl = Timeline.New({{"axes", 2.2}, {"curve", 5.4}, {"area", 4.6}, {"big", 3.4}})
for i = 1, #STAGES - 1 do
    tl:Add("split" .. i, 1.2):Add("hold" .. i, 0.6)
end
tl:Add("end", 2)

Video.Duration = tl:Total()

local rig = CameraRig.New(tl, {-100, -60, 1.1}):Sway()
rig:Move("axes", {0, -75, 1.02}, "outCubic")
rig:Move("area", {-20, -65, 1.08}, "inOutSine")
rig:Move("big", {0, -75, 1.02}, "inOutSine")

local caps = Captions.New(tl, {footer = "HOW INTEGRATION WORKS"})
caps:Add("axes", "axes", "STEP 1  -  A NON-LINEAR CURVE", "Its height f(x) changes at every point along x")
caps:Add("area", "area", "THE QUESTION", "How much area lies under the curve, between a and b?")
caps:Add("big", "big", "STEP 2  -  BIG SLICES", "Cut the area into a few wide slices")
caps:Add("split1", "hold7", "STEP 3  -  THINNER SLICES", "More slices, thinner slices, smaller error")

local ax = Axes.New{origin = {-675, -330}, scale = {270, 180}, x = {-0.4, 5.55, 1}, y = {-0.111, 3.95, 1}}
local plot = ax:Plot(f, 0, 5)
local area = ax:Area(f, 0, 5)
local slices = ax:Riemann(f, 0, 5)

local function stage(t)
    local name, p = tl:Current(t)
    local i = tonumber(name:match("^%a+(%d+)$"))
    if name:sub(1, 5) == "split" then
        return STAGES[i + 1], STAGES[i], Ease.inOutCubic(p)
    elseif name:sub(1, 4) == "hold" then
        return STAGES[i + 1]
    end
    return STAGES[1]
end

function S:OnCreate()
    self.t = 0
    self.camera = Scene.Find("Camera")
    self.x, self.y, self.z = rig:Apply(0, self.camera)
end

function S:OnUpdate(dt)
    self.t = Video.Step(self.t, math.min(dt, 0.1))
    if not Video.Driven and self.t >= tl:Total() then
        self.t = 0
    end
    self.x, self.y, self.z = rig:Apply(self.t, self.camera)
end

function S:OnRender()
    local t, z = self.t, self.z
    Draw.Layer(-100, function() Draw.Rect(self.x, self.y, 9000, 9000, "bg") end)
    Draw.Layer(0, function() area:Draw(5 * Ease.inOutCubic(tl:Progress(t, "area")), 0.26) end)
    if t >= tl:Span("big") then
        local n, from, u = stage(t)
        Draw.Layer(1, function() slices:Draw(n, {from = from, u = u, zoom = z}) end)
        Draw.Layer(8, function()
            Draw.Text(string.format("slices  n = %d     sum = %.4f", n, slices:Sum(n)), self.x + 470 * z, self.y + 380 * z, {size = 40 * z, color = "gold", align = "center", middle = true})
        end)
    end
    Draw.Layer(2, function() ax:Draw(tl:Progress(t, "axes"), z) end)
    Draw.Layer(3, function()
        local u = Ease.inOutSine(tl:Progress(t, "curve"))
        if t >= tl:Span("curve") then
            plot:Draw(5 * u, z)
        end
        if tl:Active(t, "curve") then
            ax:Tracer(f, 5 * u, {zoom = z, time = t, alpha = 1 - Math.Progress(t, tl:Span("curve") + 4.7, tl:Span("curve") + 5.4)})
        end
    end)
    caps:Draw(t, self.x, self.y, z)
end

return S
