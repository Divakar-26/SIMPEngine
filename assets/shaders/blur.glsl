uniform float uRadius; // range(0,12) default(3)

void main()
{
    vec2 texel = 1.0 / uResolution;
    vec4 sum = vec4(0.0);
    float total = 0.0;
    for (int x = -3; x <= 3; ++x)
    {
        for (int y = -3; y <= 3; ++y)
        {
            vec2 offset = vec2(float(x), float(y));
            float w = exp(-dot(offset, offset) / 6.0);
            sum += texture(uTexture, uv + offset * texel * (uRadius / 3.0)) * w;
            total += w;
        }
    }
    color = sum / total;
}
