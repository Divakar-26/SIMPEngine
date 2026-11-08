local Demo = {}

Demo.angularSpeed = 90

function Demo:OnFixedUpdate(dt)
    self.entity:SetRotation(self.entity:GetRotation() + self.angularSpeed * dt)
end

return Demo
