local Enemy = {}

Enemy.contactDamage = 10

function Enemy:OnCreate()
    local health = self.entity:Add("Health")
    health:SetMax(30)
    health:SetCurrent(30)
    health:SetInvulnerability(0.2)
end

function Enemy:OnTriggerEnter(other)
    if not other or other:GetName() ~= "Player" then
        return
    end

    local health = other:GetHealth()
    if health then
        health:Damage(self.contactDamage, self.entity)
    end
end

function Enemy:OnDamaged(amount, source)
    local sprite = self.entity:GetSprite()
    if sprite then
        sprite:SetTint(255, 90, 90)
    end
    self.flash = 0.1
end

function Enemy:OnUpdate(dt)
    if self.flash then
        self.flash = self.flash - dt
        if self.flash <= 0 then
            self.flash = nil
            local sprite = self.entity:GetSprite()
            if sprite then
                sprite:SetTint(255, 255, 255)
            end
        end
    end
end

function Enemy:OnDeath(killer)
    local particles = self.entity:GetParticles()
    if particles then
        particles:Burst(24)
    end
    self.entity:Destroy(0.2)
end

return Enemy
