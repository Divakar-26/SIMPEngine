local util = require("demo.util")

local Coin = {}

function Coin:OnCreate()
    self.phase = math.random() * 6.28
    self.collected = false
end

function Coin:OnUpdate(dt)
    self.entity:SetScale(1 + 0.18 * math.sin(Time.Now() * 6 + self.phase))
end

function Coin:OnTriggerEnter(other)
    if self.collected then
        return
    end

    self.collected = true

    local game = util.Game()
    if game then
        game:AddScore(1)
    end

    self.entity:Destroy()
end

return Coin
