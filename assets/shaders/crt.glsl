uniform float uCurvature; // range(0,0.5) default(0.12)
uniform float uScanlines; // range(0,1) default(0.35)
uniform float uAberration; // range(0,6) default(1.2)
uniform float uVignette; // range(0,1) default(0.45)

vec2 Curve(vec2 p)
{
    p = p * 2.0 - 1.0;
    p *= 1.0 + uCurvature * (p.yx * p.yx);
    return p * 0.5 + 0.5;
}

void main()
{
    vec2 p = Curve(uv);
    if (p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0)
    {
        color = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    vec2 shift = vec2(uAberration / uResolution.x, 0.0);
    vec3 c = vec3(texture(uTexture, p + shift).r, texture(uTexture, p).g, texture(uTexture, p - shift).b);
    float scan = 0.5 + 0.5 * sin(p.y * uResolution.y * 3.14159265);
    c *= 1.0 - uScanlines * (1.0 - scan);
    float vignette = smoothstep(0.95, 0.25, length(p - 0.5) * 1.25);
    c *= mix(1.0, vignette, uVignette);
    color = vec4(c, 1.0);
}
