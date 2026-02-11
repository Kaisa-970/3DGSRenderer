#version 430 core

in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D u_inputTexture;
uniform bool u_horizontal; // true = 水平模糊, false = 垂直模糊

// 9-tap 高斯核权重（sigma ≈ 4）
const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(u_inputTexture, 0));
    vec3 result = texture(u_inputTexture, texCoord).rgb * weights[0];

    vec2 direction = u_horizontal ? vec2(texelSize.x, 0.0) : vec2(0.0, texelSize.y);

    for (int i = 1; i < 5; ++i)
    {
        vec2 offset = direction * float(i);
        result += texture(u_inputTexture, texCoord + offset).rgb * weights[i];
        result += texture(u_inputTexture, texCoord - offset).rgb * weights[i];
    }

    FragColor = vec4(result, 1.0);
}
