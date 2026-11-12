local Restart = {}

function Restart:OnClick()
    Game.Set("coins", 0)
    Scene.Reload({fade = 0.4, shader = "assets://shaders/transition_wipe.glsl"})
end

return Restart
