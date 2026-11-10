local Jelly = {}

function Jelly:OnCreate()
    self.width, self.height = 140, 110
    self.homeX, self.homeY = self.entity:GetPosition()
    self.squash = 1
    self.breath = 1
    self.lift = 0
    self.busy = false
    self.blinkIn = 1.5

    local body = self.entity:Add("Vector")
    body:SetRect(self.width, self.height, 46)
    body:SetFill(255, 110, 170)
    body:SetStroke(255, 200, 225)
    body:SetStrokeWidth(5)

    self.eyes = {}
    for i, x in ipairs({-26, 26}) do
        local eye = Scene.Spawn("Eye", x, 12, self.entity)
        eye:SetZIndex(1)
        local shape = eye:Add("Vector")
        shape:SetRect(14, 24, 7)
        shape:SetFill(45, 20, 55)
        self.eyes[i] = eye
    end

    Tween.Value(1, 0.95, 0.9, "easeInOutSine", function(v) self.breath = v end, {loop = true, yoyo = true})
end

function Jelly:Jump()
    self.busy = true

    local function squash(from, to, seconds, curve)
        return Tween.Value(from, to, seconds, curve, function(v) self.squash = v end)
    end
    local function lift(from, to, seconds, curve)
        return Tween.Value(from, to, seconds, curve, function(v) self.lift = v end)
    end

    Tween.Sequence({
        squash(1, 0.6, 0.14, "easeOutQuad"),
        Tween.Parallel({
            squash(0.6, 1.3, 0.16, "easeOutQuad"),
            lift(0, 240, 0.38, "easeOutQuad"),
        }),
        Tween.Parallel({
            squash(1.3, 1, 0.2, "easeOut"),
            lift(240, 0, 0.32, "easeInQuad"),
        }),
        squash(0.55, 1, 0.9, "elasticOut"),
        function() self.busy = false end,
    })
end

function Jelly:Blink()
    for _, eye in ipairs(self.eyes) do
        Tween.Sequence({
            Tween.To(eye, {scaleY = 0.1}, 0.06, "easeIn"),
            Tween.To(eye, {scaleY = 1}, 0.1, "easeOut"),
        })
    end
end

function Jelly:OnUpdate(dt)
    local clicked = Input.IsMouseJustPressed(Key.MOUSE_LEFT) and Input.IsMouseInViewport()
    if not self.busy and (Input.IsKeyJustPressed(Key.SPACE) or clicked) then
        self:Jump()
    end

    self.blinkIn = self.blinkIn - dt
    if self.blinkIn <= 0 then
        self.blinkIn = 2 + math.random() * 2
        self:Blink()
    end

    local sy = self.squash * self.breath
    local sx = math.max(0.6, 1 + (1 - sy) * 0.9)
    self.entity:SetScale(sx, sy)

    local floor = self.homeY - self.height / 2
    self.entity:SetPosition(self.homeX, floor + self.height * sy / 2 + self.lift)
end

function Jelly:OnRender()
    local floor = self.homeY - self.height / 2
    local near = 1 - math.min(self.lift / 300, 0.7)
    Draw.SetZ(-2)
    Draw.Rect(self.homeX, floor - 60, 1000, 120, 60, 45, 90, 255)
    Draw.SetZ(-1)
    Draw.RoundRect(self.homeX, floor, 160 * near, 18 * near, 9 * near, 20, 10, 40, math.floor(110 * near))
end

return Jelly
