#version 430 core

out vec4 FragColor;

uniform vec3 uColor;
uniform float uEmissiveStrength;
uniform float u_time;
void main()
{
    vec3 emissive = uColor * uEmissiveStrength;// * abs(sin(0.3 * u_time));
    FragColor = vec4(emissive * 1.5, 1.0);
}
