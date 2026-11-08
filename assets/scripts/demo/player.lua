local util = require("demo.util")

local Player = {}

Player.speed = 320
Player.dashSpeed = 900
Player.dashTime = 0.14
Player.dashCooldown = 0.6

function Player:OnCreate()
    self.shape = self.entity:GetShape()
    self.health = self.entity:GetHealth()
    self.dash = 0
    self.cooldown = 0
    self.dirX = 1
    self.dirY = 0
end

function Player:OnUpdate(dt)
    if self.health:IsDead() then
        if Input.IsKeyJustPressed(Key.R) then
            self:Restart()
        end
        return
    end

    local x = Math.Clamp(Input.GetAxis(Key.A, Key.D) + Input.GetAxis(Key.LEFT, Key.RIGHT), -1, 1)
    local y = Math.Clamp(Input.GetAxis(Key.S, Key.W) + Input.GetAxis(Key.DOWN, Key.UP), -1, 1)
    local moving = Math.Length(x, y) > 0

    if moving then
        x, y = Math.Normalize(x, y)
        self.dirX, self.dirY = x, y
    end

    self.cooldown = math.max(0, self.cooldown - dt)
    if moving and self.cooldown == 0 and Input.IsKeyJustPressed(Key.SPACE) then
        self.dash = self.dashTime
        self.cooldown = self.dashCooldown
    end

    local speed = self.speed
    if self.dash > 0 then
        self.dash = self.dash - dt
        speed = self.dashSpeed
        x, y = self.dirX, self.dirY
    end

    local px, py = self.entity:GetPosition()
    px = Math.Clamp(px + x * speed * dt, -util.halfWidth + 22, util.halfWidth - 22)
    py = Math.Clamp(py + y * speed * dt, -util.halfHeight + 22, util.halfHeight - 22)
    self.entity:SetPosition(px, py)

    self:UpdateColor()
end

function Player:UpdateColor()
    local alpha = 255
    if self.health:IsInvulnerable() then
        alpha = (math.floor(Time.Now() * 16) % 2 == 0) and 90 or 255
    end

    if self.dash > 0 then
        self.shape:SetColor(220, 245, 255, alpha)
    else
        self.shape:SetColor(90, 200, 255, alpha)
    end
end

function Player:OnDamaged(amount, source)
    local camera = Scene.Find("Camera")
    if camera then
        camera:GetCamera():Shake(0.6)
    end

    local game = util.Game()
    if game then
        game:Refresh()
    end
end

function Player:OnDeath(killer)
    self.shape:SetColor(120, 120, 130, 255)

    local game = util.Game()
    if game then
        game:OnGameOver()
    end
end

function Player:Restart()
    self.health:Revive()
    self.entity:SetPosition(0, 0)
    self.dash = 0
    self.cooldown = 0
    self.shape:SetColor(90, 200, 255, 255)

    local game = util.Game()
    if game then
        game:Restart()
    end
end

return Player
