#include "TestFramework.h"

#include <Engine/Animation/Tween.h>
#include <Engine/Core/FileSystem.h>
#include <Engine/Core/Window.h>
#include <Engine/Rendering/ShaderEffect.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneSerializer.h>

#include <SDL3/SDL.h>

#include <filesystem>

using namespace SIMPEngine;

TEST_CASE("Shader sources get the engine header and declared uniforms are found")
{
    const std::string source =
        "uniform float uAmount; // range(0,2) default(0.5)\n"
        "uniform vec4 uTint; // color default(1,0.5,0.25,1)\n"
        "uniform vec2 uCenter; // default(0.25)\n"
        "uniform sampler2D uNoise;\n"
        "uniform float uTime;\n"
        "uniform bool uFlag;\n"
        "void main() { color = texture(uTexture, uv); }\n";

    const std::string built = ShaderEffect::BuildSource(source);
    CHECK(built.rfind("#version", 0) == 0);
    CHECK(built.find("uniform sampler2D uTexture;") != std::string::npos);
    CHECK(built.find("#line 1") != std::string::npos);
    CHECK(built.find("void main()") != std::string::npos);

    const std::string full = "#version 400 core\nvoid main() {}\n";
    CHECK(ShaderEffect::BuildSource(full) == full);

    const auto declared = ShaderEffect::ParseUniforms(source);
    CHECK(declared.size() == 5);
    CHECK(declared[0].Name == "uAmount");
    CHECK(declared[0].Type == ShaderUniformType::Float);
    CHECK_NEAR(declared[0].Max, 2.0f, 1e-4);
    CHECK_NEAR(declared[0].Default[0], 0.5f, 1e-4);
    CHECK(declared[1].Type == ShaderUniformType::Color);
    CHECK_NEAR(declared[1].Default[1], 0.5f, 1e-4);
    CHECK(declared[2].Type == ShaderUniformType::Vec2);
    CHECK_NEAR(declared[2].Default[1], 0.25f, 1e-4);
    CHECK(declared[3].Type == ShaderUniformType::Texture);
    CHECK(declared[4].Type == ShaderUniformType::Bool);
}

TEST_CASE("Shader canvas components round-trip and tween their uniforms")
{
    Scene scene("shaders");
    Entity entity = scene.BuildEntity("CRT").Build();
    auto &canvas = entity.AddComponent<ShaderCanvasComponent>();
    canvas.shader = "assets://shaders/crt.glsl";
    canvas.scope = 2;
    canvas.zMin = -2.0f;
    canvas.zMax = 3.0f;
    canvas.order = 4;
    canvas.resolutionScale = 0.5f;
    canvas.progress = 0.25f;
    canvas.limitArea = true;
    canvas.areaSize = {640.0f, 220.0f};
    ShaderUniformValue amount;
    amount.name = "uAmount";
    amount.type = 0;
    amount.value[0] = 0.75f;
    canvas.uniforms.push_back(amount);
    ShaderUniformValue tint;
    tint.name = "uTint";
    tint.type = 4;
    tint.value[0] = 0.1f;
    tint.value[1] = 0.2f;
    tint.value[2] = 0.3f;
    tint.value[3] = 1.0f;
    canvas.uniforms.push_back(tint);

    const std::string yaml = SceneSerializer(&scene).SerializeToString();
    Scene loaded("loaded");
    CHECK(SceneSerializer(&loaded).DeserializeFromString(yaml));
    auto view = loaded.GetRegistry().view<ShaderCanvasComponent>();
    CHECK(view.size() == 1);
    for (auto handle : view)
    {
        const auto &copy = view.get<ShaderCanvasComponent>(handle);
        CHECK(copy.shader == "assets://shaders/crt.glsl");
        CHECK(copy.scope == 2);
        CHECK_NEAR(copy.zMin, -2.0f, 1e-4);
        CHECK(copy.order == 4);
        CHECK_NEAR(copy.resolutionScale, 0.5f, 1e-4);
        CHECK_NEAR(copy.progress, 0.25f, 1e-4);
        CHECK(copy.limitArea);
        CHECK_NEAR(copy.areaSize.y, 220.0f, 1e-4);
        CHECK(copy.uniforms.size() == 2);
        CHECK(copy.uniforms[0].name == "uAmount");
        CHECK_NEAR(copy.uniforms[0].value[0], 0.75f, 1e-4);
        CHECK(copy.uniforms[1].type == 4);
        CHECK_NEAR(copy.uniforms[1].value[2], 0.3f, 1e-4);
    }

    TweenSystem &tweens = scene.GetTweens();
    float out[4] = {};
    const std::string name = "uAmount";
    CHECK(tweens.Read(entity.GetHandle(), TweenProperty::ShaderUniform, out, &name));
    CHECK_NEAR(out[0], 0.75f, 1e-4);
    const float target[4] = {2.0f, 0.0f, 0.0f, 0.0f};
    tweens.Write(entity.GetHandle(), TweenProperty::ShaderUniform, target, &name);
    CHECK_NEAR(canvas.uniforms[0].value[0], 2.0f, 1e-4);
    const float progress[4] = {0.9f, 0.0f, 0.0f, 0.0f};
    tweens.Write(entity.GetHandle(), TweenProperty::ShaderProgress, progress);
    CHECK_NEAR(canvas.progress, 0.9f, 1e-4);
}

TEST_CASE("Example shaders compile and a broken shader fails safely")
{
    Window window;
    if (!window.Init("shader-test", 64, 64, SDL_WINDOW_HIDDEN))
    {
        std::printf("  (no OpenGL context available, skipping the GPU compile check)\n");
        return;
    }

    const std::filesystem::path directory = std::filesystem::path(SIMP_TEST_ASSETS) / "shaders";
    int compiled = 0;
    for (const auto &entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.path().extension() != ".glsl")
            continue;
        const std::optional<std::string> text = FileSystem::ReadText(entry.path().string());
        CHECK(text.has_value());
        if (!text)
            continue;
        auto effect = ShaderEffect::FromSource(entry.path().filename().string(), *text);
        if (!effect->IsValid())
            std::printf("  %s\n", effect->GetError().c_str());
        CHECK(effect->IsValid());
        ++compiled;
    }
    CHECK(compiled >= 8);

    auto broken = ShaderEffect::FromSource("broken.glsl", "void main()\n{\n    color = vec4(notDeclared);\n}\n");
    CHECK(!broken->IsValid());
    CHECK(broken->GetError().find("broken.glsl") != std::string::npos);
    CHECK(broken->GetProgram() == 0);
}
