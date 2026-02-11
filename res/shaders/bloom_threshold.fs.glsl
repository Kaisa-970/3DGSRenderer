#version 430 core

in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D u_inputTexture;
uniform float u_threshold = 1.0; // 亮度阈值

void main()
{
    vec3 color = texture(u_inputTexture, texCoord).rgb;

    // 计算亮度（BT.709）
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));

    // 软阈值：smoothstep 平滑过渡，避免硬边
    float soft = smoothstep(u_threshold - 0.1, u_threshold + 0.1, brightness);

    FragColor = vec4(color * soft, 1.0);
}
