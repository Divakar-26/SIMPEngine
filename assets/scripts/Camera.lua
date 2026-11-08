local Script = {}

function Script:OnCreate()
    local p = Path.New()
    p:MoveTo(0, 0)
    p:LineTo(100, 0)
    p:Close()
    
    local v = self.entity
    local vecteor = v:Add("Vector")
    vecteor:SetPath(p)
    
    vecteor:SetStroke(255,255,255)
    vecteor:SetStrokeWidth(6)
    vecteor:SetCap("round")
    vecteor:SetJoin("round")
    
end

function Script:OnUpdate(dt)

end


return Script
