local Script = {}

local r = 0
local g = 255
local b = 255

function Script:OnCreate()
end

function Script:OnUpdate(dt)
    self.entity:GetShape():SetColor(r,g,b,255)
    r = r + 20 * dt
    -- g = g + 20 * dt
    -- b = b + 20 * dt
    
end

return Script
