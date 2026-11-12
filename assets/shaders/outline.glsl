uniform float uThickness; // range(0,12) default(2)
uniform vec4 uColor; // color default(1,1,1,1)

void main()
{
    vec4 c = texture(uTexture, uv);
    vec2 texel = uThickness / uResolution;
    float neighbors = 0.0;
    for (int i = 0; i < 8; ++i)
    {
        float angle = float(i) * 0.785398163;
        neighbors = max(neighbors, texture(uTexture, uv + vec2(cos(angle), sin(angle)) * texel).a);
    }
    float edge = clamp(neighbors - c.a, 0.0, 1.0) * uColor.a;
    color = vec4(c.rgb + uColor.rgb * edge, max(c.a, edge));
}
