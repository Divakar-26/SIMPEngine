#pragma once

namespace SIMPEngine::Shaders {

// Instanced SDF shapes: a single static unit-quad (divisor 0) combined
// with a per-instance attribute stream (divisor 1) so any number of
// circles/rounded-rects/capsules draw in ONE glDrawArraysInstanced call
// instead of one flush+draw per shape.
static constexpr const char* SHAPE_VERT = R"(
#version 330 core

layout(location=0) in vec2 aUnit;          // static unit quad, [-1,1]
layout(location=1) in vec2 iBoundsCenter;  // per-instance
layout(location=2) in vec2 iBoundsHalf;
layout(location=3) in vec2 iDataA;
layout(location=4) in vec2 iDataB;
layout(location=5) in float iRadius;
layout(location=6) in float iShapeType;
layout(location=7) in float iAA;
layout(location=8) in vec4 iColor;

uniform mat4 uVP;

out vec2 vPos;
flat out vec2 vDataA;
flat out vec2 vDataB;
flat out float vRadius;
flat out float vShapeType;
flat out float vAA;
out vec4 vColor;

void main()
{
    vec2 world = iBoundsCenter + aUnit * iBoundsHalf;
    gl_Position = uVP * vec4(world, 0.0, 1.0);

    vPos = world;
    vDataA = iDataA;
    vDataB = iDataB;
    vRadius = iRadius;
    vShapeType = iShapeType;
    vAA = iAA;
    vColor = iColor;
})";

static constexpr const char* SHAPE_FRAG = R"(
#version 330 core
out vec4 FragColor;

in vec2 vPos;
in vec4 vColor;
flat in vec2 vDataA;
flat in vec2 vDataB;
flat in float vRadius;
flat in float vShapeType;
flat in float vAA;

// 0=circle (vDataA=center, vRadius=radius)
// 1=roundedrect (vDataA=center, vDataB=halfSize, vRadius=cornerRadius)
// 2=capsule (vDataA=pointA, vDataB=pointB, vRadius=lineWidth)

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
    int shapeType = int(vShapeType + 0.5);

    if      (shapeType == 0) d = sdCircle(vPos, vDataA, vRadius);
    else if (shapeType == 1) d = sdRoundedRect(vPos, vDataA, vDataB, vRadius);
    else if (shapeType == 2) d = sdCapsule(vPos, vDataA, vDataB, vRadius);

    float alpha = 1.0 - smoothstep(-vAA, vAA, d);
    if (alpha < 0.001) discard;

    FragColor = vec4(vColor.rgb, vColor.a * alpha);
}
)";

} // namespace SIMPEngine::Shaders