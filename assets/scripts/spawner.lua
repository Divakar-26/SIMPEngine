local Spawner = {}

Spawner.script = "assets://scripts/pickup.lua"
Spawner.limit = 5

function Spawner:OnCreate()
    self.spawned = 0
    local timer = self.entity:Add("Timer")
    timer:SetInterval(2.0)
    timer:SetRepeat(true)
    timer:Restart()
end

function Spawner:OnTimer()
    if self.spawned >= self.limit then
        self.entity:GetTimer():Stop()
        return
    end

    local x, y = self.entity:GetPosition()
    local item = Scene.Spawn("Pickup", x + math.random(-200, 200), y)
    item:Add("Shape"):SetColor(120, 255, 160)
    local trigger = item:Add("Trigger")
    trigger:SetSize(32, 32)
    item:SetScript(self.script)
    self.spawned = self.spawned + 1
end

return Spawner
