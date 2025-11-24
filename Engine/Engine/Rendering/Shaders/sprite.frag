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
