local Enemy = {}

function Enemy:Hit(other)
    if not other then
        return
    end

    local health = other:GetHealth()
    if health then
        health:Damage(1, self.entity)
    end
end

function Enemy:OnTriggerEnter(other)
    self:Hit(other)
end

function Enemy:OnTriggerStay(other)
    self:Hit(other)
end

function Enemy:OnUpdate(dt)
    self.entity:SetRotation(Time.Now() * 120 % 360)
end

return Enemy
