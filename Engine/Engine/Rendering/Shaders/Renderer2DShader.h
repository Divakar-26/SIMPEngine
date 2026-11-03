#pragma once

namespace SIMPEngine::Shaders
{
    static constexpr const char *RENDERER2D_VERT = R"(
#version 400 core

layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aUV;
layout(location=2) in vec4 aColor;
layout(location=3) in vec2 aLocal;
layout(location=4) in vec4 aShape;
layout(location=5) in vec3 aMisc;

uniform mat4 uVP;

out vec2 vUV;
out vec4 vColor;
out vec2 vLocal;
flat out vec4 vShape;
flat out vec3 vMisc;

void main()
{
    gl_Position = uVP * vec4(aPosition, 1.0);
    vUV = aUV;
    vColor = aColor;
    vLocal = aLocal;
    vShape = aShape;
    vMisc = aMisc;
}
)";

    static constexpr const char *RENDERER2D_FRAG = R"(
#version 400 core

in vec2 vUV;
in vec4 vColor;
in vec2 vLocal;
flat in vec4 vShape;
flat in vec3 vMisc;

uniform sampler2D uTextures[16];

out vec4 FragColor;

float sdRoundBox(vec2 p, vec2 b, float r)
{
    vec2 q = abs(p) - b + vec2(r);
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

void main()
{
    int slot = int(vMisc.x + 0.5);
    int mode = int(vMisc.y + 0.5);
    float softness = max(vMisc.z, 0.0001);

    vec4 tex = texture(uTextures[slot], vUV);

    float r = min(vShape.z, min(vShape.x, vShape.y));
    float d = sdRoundBox(vLocal, vShape.xy, r);
    if (vShape.w > 0.0)
        d = abs(d + vShape.w * 0.5) - vShape.w * 0.5;
    float dw = max(fwidth(d) * softness, 1e-6);

    float sdf = tex.a;
    float sw = max(fwidth(sdf) * 0.7 * softness, 1e-4);

    if (mode == 0)
    {
        FragColor = tex * vColor;
    }
    else if (mode == 1)
    {
        float a = 1.0 - smoothstep(-dw, dw, d);
        if (a <= 0.001)
            discard;
        FragColor = vec4(vColor.rgb * tex.rgb, vColor.a * tex.a * a);
    }
    else
    {
        float a = smoothstep(0.5 - sw, 0.5 + sw, sdf);
        if (a <= 0.001)
            discard;
        FragColor = vec4(vColor.rgb, vColor.a * a);
    }
}
)";
}
