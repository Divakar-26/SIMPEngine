local Rotator = {}

Rotator.speed = 90

function Rotator:OnCreate()
    log.info("Rotator attached to", self.entity:GetName())
end

function Rotator:OnUpdate(dt)
    self.entity:SetRotation(self.entity:GetRotation() + self.speed * dt)

    if Input.IsKeyJustPressed(Key.SPACE) then
        self.speed = -self.speed
    end
end

return Rotator
