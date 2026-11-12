void main()
{
    vec4 c = texture(uTexture, uv);
    color = vec4(mix(c.rgb, vec3(0.0), uProgress), c.a);
}
