local Script = {}

local x = 0
local y = 0

function Script:OnCreate()
    -- self.entity:SetName("HELLO")
end

function Script:OnUpdate(dt)
    self.entity:GetText():SetText("HELLO HI HOW ARE YOU" + dt)
end

return Script
