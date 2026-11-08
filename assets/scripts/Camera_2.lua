local Script = {}

function Script:OnCreate()
    local line = self.entity:Add("Vector")
    
    line:SetPath(Path.Line(0 ,0 ,100 ,0))
    line:SetFillEnabled(false)
    line:SetStroke(255,255,255)
    line:SetStrokeWidth(6)
    line:SetCap("round")
    line:SetTrim(0,0)
    
    Tween.Sequence({
        Tween.To(self.entity, {trimEnd = 1}, 0.6, "easeOutCubic"),
        0.3,
        Tween.To(self.entity, {x = "+=1200"}, 0.8, "easeOutCubic"),
    })
    
end

function Script:OnUpdate(dt)
end

return Script
