#pragma once

namespace SIMPEngine:: Shaders {

static constexpr const char* SPRITE_VERT = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTex;
    uniform mat4 uMVP;
    out vec2 TexCoord;
    void main() {
        gl_Position = uMVP * vec4(aPos, 1.0);
        TexCoord = aTex;
    }
)";
    
static constexpr const char* SPRITE_FRAG = R"(
    #version 330 core
    in vec2 TexCoord;
    out vec4 FragColor;
    uniform vec4 uColor;
    uniform sampler2D uTexture;
    uniform bool uUseTexture;
    void main() {
        if(uUseTexture) {
            vec4 tex = texture(uTexture, TexCoord);
            FragColor = vec4(tex.rgb * uColor.rgb, tex.a * uColor.a);
        } else {
            FragColor = uColor;
        }
    }
)";

}
