local Platform = {}

Platform.travel = 200
Platform.speed = 80

function Platform:OnCreate()
    local path = self.entity:Add("Path")
    path:ClearPoints()
    path:AddPoint(self.travel, 0)
    path:AddPoint(0, 0)
    path:SetSpeed(self.speed)
    path:Restart()
end

function Platform:OnPathPoint(index)
    log.info(self.entity:GetName(), "reached point", index)
end

return Platform
