local Script = {}

local radius = 100
function Script:OnCreate()
    local e = self.entity
    e:Add("Light"):SetRadius(radius)
end

function Script:OnUpdate(dt)
    local e = self.entity
    if Input.IsKeyDown(Key.UP) then
        e:GetLight():SetIntensity(e:GetLight():GetIntensity() + 10 * dt)
    end
    
    if Input.IsKeyDown(Key.RIGHT) then
        e:GetLight():SetRadius(e:GetLight():GetRadius() + 100 * dt)
    end
    
    
    if Input.IsMouseJustPressed(Key.MOUSE_LEFT) and Input.IsMouseInViewport() then
        local mx, my = self.entity:GetMouseWorld()  
        print(mx, my)
    end
end

return Script
