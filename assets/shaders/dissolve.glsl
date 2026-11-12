uniform float uEdgeWidth; // range(0,0.3) default(0.06)
uniform vec4 uEdgeColor; // color default(1,0.55,0.1,1)
uniform float uNoiseScale; // range(1,60) default(14)

float Hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float Noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(Hash(i), Hash(i + vec2(1.0, 0.0)), f.x),
               mix(Hash(i + vec2(0.0, 1.0)), Hash(i + vec2(1.0, 1.0)), f.x), f.y);
}

void main()
{
    vec4 c = texture(uTexture, uv);
    float n = Noise(uv * uNoiseScale * vec2(uResolution.x / uResolution.y, 1.0));
    float threshold = uProgress * (1.0 + uEdgeWidth) - uEdgeWidth * 0.5;
    float alive = smoothstep(threshold, threshold + 0.01, n);
    float edge = smoothstep(threshold - uEdgeWidth, threshold, n) * (1.0 - alive);
    vec3 rgb = c.rgb * alive + uEdgeColor.rgb * edge * c.a;
    float a = c.a * max(alive, edge);
    color = vec4(rgb, a);
}
