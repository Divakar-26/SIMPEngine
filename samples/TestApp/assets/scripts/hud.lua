local Hud = {}

function Hud:OnCreate()
    self.label = self.entity:GetUILabel()
    if not Game.Has("coins") then
        Game.Set("coins", 0)
    end
end

function Hud:OnUpdate(dt)
    if not self.label then
        return
    end
    local x, y = Input.GetVector("Move")
    local fps = math.floor(1 / math.max(dt, 0.0001) + 0.5)
    local jump = Input.IsActionDown("Jump") and "jump" or "-"
    if Input.IsTouchDevice() then
        self.label:SetText(string.format("touch  move %.1f %.1f  %s  |  %d fps", x, y, jump, fps))
    else
        self.label:SetText(string.format("A D move  Space jump  |  move %.1f %.1f  %s  |  %d fps", x, y, jump, fps))
    end
end

return Hud
