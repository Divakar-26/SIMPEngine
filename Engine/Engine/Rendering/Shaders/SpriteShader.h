#pragma once

namespace SIMPEngine::Shaders {

    // NOTE: requires a GL 4.0+ context (see GLRenderingAPI::Init comments).
    // Per-fragment dynamic indexing into a sampler array lets colored quads
    // and textured sprites (including rasterized SVG icons) share one batch
    // and one draw call, instead of texture switches forcing a flush.
    static constexpr const char* SPRITE_VERT = R"(
#version 400 core

layout(location=0) in vec2 aLocalPos;
layout(location=1) in vec3 aCenter;
layout(location=2) in vec2 aSize;
layout(location=3) in float aRot;
layout(location=4) in vec2 aTex;
layout(location=5) in float aTexIndex;
layout(location=6) in vec4 aColor;

uniform mat4 uVP;

out vec4 vColor;
out vec2 vTex;
flat out int vTexIndex;

void main()
{
    vec2 scaled = aLocalPos * aSize;

    float s = sin(aRot);
    float c = cos(aRot);

    vec2 rotated = vec2(
        scaled.x*c - scaled.y*s,
        scaled.x*s + scaled.y*c
    );

    vec3 world = vec3(rotated + aCenter.xy, aCenter.z);

    gl_Position = uVP * vec4(world, 1);

    vColor = aColor;
    vTex = aTex;
    vTexIndex = int(aTexIndex + 0.5);
})";

    static constexpr const char* SPRITE_FRAG = R"(
#version 400 core

in vec2 vTex;
in vec4 vColor;
flat in int vTexIndex;

// Slot 0 is always bound to a 1x1 white pixel, so a plain colored quad
// (texIndex == 0) just multiplies vColor by (1,1,1,1).
uniform sampler2D uTextures[16];

out vec4 FragColor;

void main()
{
    vec4 texColor = texture(uTextures[vTexIndex], vTex);
    FragColor = texColor * vColor;
}
)";

}