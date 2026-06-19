#pragma once

namespace SIMPEngine::Shaders {

static constexpr const char* SHAPE_VERT = R"(
#version 330 core
layout (location = 0) in vec2 aPos;

out vec2 vPos;

uniform mat4 uVP;

void main() {
    vPos = aPos;
    gl_Position = uVP * vec4(aPos, 0.0, 1.0);
}
)";

static constexpr const char* SHAPE_FRAG = R"(
#version 330 core
out vec4 FragColor;

in vec2 vPos;

uniform int   u_shapeType;   // 0=circle, 1=roundedrect, 2=capsule
uniform vec4  u_color;
uniform float u_aa;

uniform vec2  u_circleCenter;
uniform float u_circleRadius;

uniform vec2  u_rectCenter;
uniform vec2  u_rectHalfSize;
uniform float u_rectRadius;

uniform vec2  u_lineA;
uniform vec2  u_lineB;
uniform float u_lineWidth;

float sdCircle(vec2 p, vec2 center, float r) {
    return length(p - center) - r;
}

float sdRoundedRect(vec2 p, vec2 center, vec2 halfSize, float r) {
    vec2 q = abs(p - center) - halfSize + vec2(r);
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

float sdCapsule(vec2 p, vec2 a, vec2 b, float r) {
    vec2 ab = b - a;
    vec2 ap = p - a;
    float t = clamp(dot(ap, ab) / dot(ab, ab), 0.0, 1.0);
    return length(p - (a + t * ab)) - r;
}

void main() {
    float d = 1e9;

    if      (u_shapeType == 0) d = sdCircle(vPos, u_circleCenter, u_circleRadius);
    else if (u_shapeType == 1) d = sdRoundedRect(vPos, u_rectCenter, u_rectHalfSize, u_rectRadius);
    else if (u_shapeType == 2) d = sdCapsule(vPos, u_lineA, u_lineB, u_lineWidth);

    float alpha = 1.0 - smoothstep(-u_aa, u_aa, d);
    if (alpha < 0.001) discard;

    FragColor = vec4(u_color.rgb, u_color.a * alpha);
}
)";

} // namespace SIMPEngine::Shaders