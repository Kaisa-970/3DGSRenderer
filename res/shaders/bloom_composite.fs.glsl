#version 430 core

in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D u_sceneTexture;  // 原始 HDR 场景
uniform sampler2D u_bloomTexture;  // 模糊后的高亮区域
uniform float u_intensity = 0.5;   // Bloom 强度

void main()
{
    vec3 scene = texture(u_sceneTexture, texCoord).rgb;
    vec3 bloom = texture(u_bloomTexture, texCoord).rgb;

    // 叠加合成
    vec3 result = scene + bloom * u_intensity;

    FragColor = vec4(result, 1.0);
}
