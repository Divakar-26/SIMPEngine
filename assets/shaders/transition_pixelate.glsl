uniform float uMaxBlock; // range(2,128) default(48)

void main()
{
    float block = max(1.0, mix(1.0, uMaxBlock, uProgress));
    vec2 cell = block / uResolution;
    vec2 snapped = (floor(uv / cell) + 0.5) * cell;
    vec4 c = texture(uTexture, snapped);
    color = vec4(mix(c.rgb, vec3(0.0), smoothstep(0.6, 1.0, uProgress)), c.a);
}
