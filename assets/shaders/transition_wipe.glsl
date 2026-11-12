uniform float uSoftness; // range(0.001,0.5) default(0.08)

void main()
{
    vec4 c = texture(uTexture, uv);
    float edge = uProgress * (1.0 + uSoftness) - uSoftness;
    float covered = 1.0 - smoothstep(edge, edge + uSoftness, uv.x);
    color = vec4(mix(c.rgb, vec3(0.0), covered), c.a);
}
