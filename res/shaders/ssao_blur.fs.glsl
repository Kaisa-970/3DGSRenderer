#version 430 core

in vec2 texCoord;
out float fragAO;

uniform sampler2D u_inputTexture;
uniform int u_horizontal; // 1 = 水平模糊, 0 = 垂直模糊

// 9-tap 高斯核权重（sigma ≈ 2）
const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(u_inputTexture, 0));
    float result = texture(u_inputTexture, texCoord).r * weights[0];

    vec2 direction = (u_horizontal != 0) ? vec2(texelSize.x, 0.0) : vec2(0.0, texelSize.y);

    for (int i = 1; i < 5; ++i)
    {
        vec2 offset = direction * float(i);
        result += texture(u_inputTexture, texCoord + offset).r * weights[i];
        result += texture(u_inputTexture, texCoord - offset).r * weights[i];
    }

    fragAO = result;
}
