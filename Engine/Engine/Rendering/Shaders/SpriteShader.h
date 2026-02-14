#pragma once

namespace SIMPEngine::Shaders {

static constexpr const char* SPRITE_VERT = R"(
#version 330 core

layout(location=0) in vec2 aLocalPos;
layout(location=1) in vec3 aCenter;
layout(location=2) in vec2 aSize;
layout(location=3) in float aRot;
layout(location=4) in vec4 aColor;
layout(location=5) in vec2 aTex;

uniform mat4 uVP;

out vec4 vColor;
out vec2 vTex;

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

    gl_Position = uVP * vec4(world,1);

    vColor = aColor;
    vTex = aTex;
})";
    
static constexpr const char* SPRITE_FRAG = R"(
    #version 330 core

in vec2 vTex;
in vec4 vColor;

uniform sampler2D uTexture;
uniform bool uUseTexture;

out vec4 FragColor;

void main()
{
    if(uUseTexture)
    {
        vec4 tex = texture(uTexture, vTex);
        FragColor = tex * vColor;
    }
    else
    {
        FragColor = vColor;
    }
}
)";

}