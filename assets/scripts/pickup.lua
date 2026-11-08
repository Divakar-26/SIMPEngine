local Pickup = {}

Pickup.heal = 25
Pickup.bobHeight = 6
Pickup.bobSpeed = 3

function Pickup:OnCreate()
    local x, y = self.entity:GetPosition()
    self.baseY = y
    self.startTime = Time.Now()
end

function Pickup:OnUpdate(dt)
    local x = self.entity:GetPosition()
    local t = Time.Now() - self.startTime
    self.entity:SetPosition(x, self.baseY + math.sin(t * self.bobSpeed) * self.bobHeight)
end

function Pickup:OnTriggerEnter(other)
    if not other then
        return
    end

    local health = other:GetHealth()
    if health and not health:IsDead() then
        health:Heal(self.heal)
        self.entity:Destroy()
    end
end

return Pickup
