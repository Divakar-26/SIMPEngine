local util = require("demo.util")

local Game = {}

Game.maxCoins = 8
Game.coinInterval = 1.2
Game.healEvery = 5

function Game:OnCreate()
    math.randomseed(os.time())

    self.score = 0
    self.best = 0
    self.message = ""

    self:BuildArena()
    self.player = self:SpawnPlayer()
    self:BuildHud()

    self:SpawnEnemy(-500, -250, {{500, 0}, {500, 200}, {0, 200}, {0, 0}})
    self:SpawnEnemy(450, 300, {{-400, 0}, {-400, -300}, {0, -300}, {0, 0}})

    local timer = self.entity:Add("Timer")
    timer:SetInterval(self.coinInterval)
    timer:SetRepeat(true)
    timer:Start()

    for _ = 1, 3 do
        self:SpawnCoin()
    end

    self:Refresh()
end

function Game:BuildArena()
    local w, h = util.halfWidth, util.halfHeight
    local edge = 16

    util.Box("Floor", 0, 0, w * 2, h * 2, 28, 32, 46, -10)
    util.Box("Wall Top", 0, h + edge / 2, w * 2 + edge * 2, edge, 90, 100, 140, -9)
    util.Box("Wall Bottom", 0, -h - edge / 2, w * 2 + edge * 2, edge, 90, 100, 140, -9)
    util.Box("Wall Left", -w - edge / 2, 0, edge, h * 2, 90, 100, 140, -9)
    util.Box("Wall Right", w + edge / 2, 0, edge, h * 2, 90, 100, 140, -9)
end

function Game:SpawnPlayer()
    local player = util.Box("Player", 0, 0, 44, 44, 90, 200, 255, 3)

    local collision = player:Add("Collision")
    collision:SetSize(44, 44)
    collision:SetOffset(-22, -22)

    local health = player:Add("Health")
    health:SetMax(5)
    health:SetCurrent(5)
    health:SetInvulnerability(1.0)

    player:SetScript(util.Script("player"))
    return player
end

function Game:SpawnEnemy(x, y, points)
    local enemy = util.Box("Enemy", x, y, 56, 56, 235, 80, 80, 2)

    local trigger = enemy:Add("Trigger")
    trigger:SetSize(56, 56)
    trigger:SetOffset(-28, -28)
    trigger:SetFilter("Player")

    local path = enemy:Add("Path")
    path:SetSpeed(170)
    for _, point in ipairs(points) do
        path:AddPoint(point[1], point[2])
    end

    enemy:SetScript(util.Script("enemy"))
    return enemy
end

function Game:SpawnCoin()
    local x, y = util.RandomPoint(util.halfWidth - 40, util.halfHeight - 40)
    local coin = util.Box("Coin", x, y, 26, 26, 255, 210, 60, 1)

    local trigger = coin:Add("Trigger")
    trigger:SetSize(34, 34)
    trigger:SetOffset(-17, -17)
    trigger:SetFilter("Player")

    coin:SetScript(util.Script("coin"))
    return coin
end

function Game:BuildHud()
    local hud = Scene.Spawn("HUD", 0, 285, Scene.Find("Camera"))
    hud:SetZIndex(20)

    self.text = hud:Add("Text")
    self.text:SetFontSize(26)
    self.text:SetAlign(1)
end

function Game:OnTimer()
    if #Scene.FindAll("Coin") < self.maxCoins then
        self:SpawnCoin()
    end
end

function Game:AddScore(amount)
    self.score = self.score + amount
    self.best = math.max(self.best, self.score)

    if self.score % self.healEvery == 0 then
        local health = self.player:GetHealth()
        if not health:IsDead() then
            health:Heal(1)
        end
    end

    self:Refresh()
end

function Game:OnGameOver()
    self.message = "Game over - press R"
    self:Refresh()
end

function Game:Restart()
    for _, coin in ipairs(Scene.FindAll("Coin")) do
        coin:Destroy()
    end

    self.score = 0
    self.message = ""
    self:Refresh()
end

function Game:Refresh()
    local health = self.player:GetHealth()
    local line = string.format("Score %d   Best %d   HP %d/%d", self.score, self.best, health:GetCurrent(), health:GetMax())

    if self.message ~= "" then
        line = line .. "   " .. self.message
    end

    self.text:SetText(line)
end

return Game
