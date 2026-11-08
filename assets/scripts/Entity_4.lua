local Script = {}

function Script:OnCreate()
end

function DrawCircle()
    local v = self.entity:Add("Vector")
        v:SetPath(Path.Circle(200))
    
        v:SetStrokeEnabled()
        v:SetFillEnabled(false)
        v:SetStroke(255,255,255)
        v:SetStrokeWidth(6)
        v:SetTrim(0, 0)
    
        Tween.To(self.entity, {trimEnd = 1}, 3.0, "easeOutCubic")
end

function Script:OnUpdate(dt)
    if Input.IsMouseJustPressed(Key.MOUSE_LEFT) then
        DrawCircle()
    end
end

return Script
