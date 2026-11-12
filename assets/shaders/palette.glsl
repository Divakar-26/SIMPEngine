uniform vec4 uShadow; // color default(0.08,0.06,0.18,1)
uniform vec4 uMid; // color default(0.75,0.35,0.45,1)
uniform vec4 uLight; // color default(1,0.93,0.65,1)
uniform float uAmount; // range(0,1) default(1)

void main()
{
    vec4 c = texture(uTexture, uv);
    float l = dot(c.rgb, vec3(0.299, 0.587, 0.114));
    vec3 mapped = l < 0.5 ? mix(uShadow.rgb, uMid.rgb, l * 2.0) : mix(uMid.rgb, uLight.rgb, (l - 0.5) * 2.0);
    color = vec4(mix(c.rgb, mapped * c.a, uAmount), c.a);
}
