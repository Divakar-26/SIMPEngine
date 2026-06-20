#pragma once

namespace SIMPEngine::Shaders {

static constexpr const char* LINE_VERT = R"(
#version 330 core
layout(location = 0) in vec2 aPos;

uniform mat4 uVP;

void main()
{
    gl_Position = uVP * vec4(aPos, 0.0, 1.0);
}
)";

static constexpr const char* LINE_FRAG = R"(
#version 330 core
out vec4 FragColor;

uniform vec4 uColor;

void main()
{
    FragColor = uColor;
}
)";
}