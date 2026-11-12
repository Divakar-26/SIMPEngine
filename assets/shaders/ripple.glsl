uniform vec2 uCenter; // default(0.5,0.5)
uniform float uStrength; // range(0,0.1) default(0.02)
uniform float uFrequency; // range(1,80) default(30)
uniform float uSpeed; // range(0,10) default(3)
uniform bool uFollowMouse;

void main()
{
    vec2 center = uFollowMouse ? uMouse : uCenter;
    vec2 aspect = vec2(uResolution.x / uResolution.y, 1.0);
    vec2 delta = (uv - center) * aspect;
    float dist = length(delta);
    float wave = sin(dist * uFrequency - uTime * uSpeed);
    float falloff = exp(-dist * 3.0);
    vec2 direction = dist > 0.0001 ? delta / dist : vec2(0.0);
    vec2 offset = direction * wave * uStrength * falloff / aspect;
    color = texture(uTexture, uv + offset);
}
