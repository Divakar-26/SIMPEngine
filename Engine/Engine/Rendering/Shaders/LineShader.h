#pragma once

namespace SIMPEngine::Shaders {

    // Color moved from a uniform to a per-vertex attribute so many
    // differently-colored segments can share a single GL_LINES draw call.
    static constexpr const char* LINE_VERT = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;

uniform mat4 uVP;

out vec4 vColor;

void main()
{
    gl_Position = uVP * vec4(aPos, 0.0, 1.0);
    vColor = aColor;
}
)";

    static constexpr const char* LINE_FRAG = R"(
#version 330 core
in vec4 vColor;
out vec4 FragColor;

void main()
{
    FragColor = vColor;
}
)";
}