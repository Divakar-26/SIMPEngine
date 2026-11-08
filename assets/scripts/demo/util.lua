local util = {}

util.halfWidth = 780
util.halfHeight = 430

function util.Script(name)
    return "assets://scripts/demo/" .. name .. ".lua"
end

function util.Box(name, x, y, w, h, r, g, b, z, parent)
    local entity = Scene.Spawn(name, x, y, parent)
    local shape = entity:Add("Shape")
    shape:SetSize(w, h)
    shape:SetColor(r, g, b)
    entity:SetZIndex(z or 0)
    return entity
end

function util.RandomPoint(rangeX, rangeY)
    return (math.random() * 2 - 1) * rangeX, (math.random() * 2 - 1) * rangeY
end

function util.Game()
    local demo = Scene.Find("Demo")
    return demo and demo:GetScript()
end

return util
