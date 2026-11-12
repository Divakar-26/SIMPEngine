uniform float uAmount; // range(0,1) default(1)

void main()
{
    vec4 c = texture(uTexture, uv);
    float g = dot(c.rgb, vec3(0.299, 0.587, 0.114));
    color = vec4(mix(c.rgb, vec3(g) * c.a, uAmount), c.a);
}
