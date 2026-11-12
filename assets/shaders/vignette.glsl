uniform float uIntensity; // range(0,1) default(0.5)
uniform float uSoftness; // range(0.05,1) default(0.55)

void main()
{
    vec4 c = texture(uTexture, uv);
    float d = length((uv - 0.5) * vec2(uResolution.x / uResolution.y, 1.0));
    float v = smoothstep(0.85, 0.85 - uSoftness, d);
    color = vec4(c.rgb * mix(1.0, v, uIntensity), c.a);
}
