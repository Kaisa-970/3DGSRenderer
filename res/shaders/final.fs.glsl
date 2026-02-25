#version 430 core

in vec2 texCoord;
out vec4 finalColor;

uniform sampler2D u_colorTexture;
uniform float u_exposure;       // 曝光度（默认 1.0）
uniform int   u_tonemapMode;    // 0 = None, 1 = Reinhard, 2 = ACES Filmic
uniform int   u_displaySingleChannelR; // 1 = 单通道 R 复制为 RGB（SSAO 等灰度预览）

// ---- ACES Filmic Tone Mapping ----
// 参考：Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
// 这是好莱坞电影工业标准，UE5 和 Unity HDRP 的默认 tone mapper
vec3 ACESFilmic(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// ---- Reinhard Tone Mapping ----
// 最简单的 tone mapper: color / (1 + color)
vec3 Reinhard(vec3 x)
{
    return x / (1.0 + x);
}

void main()
{
    // 1. 采样颜色（单通道 R 时复制为 RGB，用于 SSAO 等灰度预览）
    vec3 hdrColor;
    if (u_displaySingleChannelR != 0)
        hdrColor = vec3(texture(u_colorTexture, texCoord).r);
    else
        hdrColor = texture(u_colorTexture, texCoord).rgb;

    // 2. 应用曝光度（模拟相机曝光控制）
    hdrColor *= u_exposure;

    // 3. Tone Mapping（HDR → LDR）
    vec3 mapped;
    if (u_tonemapMode == 1)
        mapped = Reinhard(hdrColor);
    else if (u_tonemapMode == 2)
        mapped = ACESFilmic(hdrColor);
    else
        mapped = clamp(hdrColor, 0.0, 1.0);  // 不做 tone mapping，直接截断

    // 4. Gamma 校正（线性空间 → sRGB，补偿显示器的 ^2.2 非线性）
    vec3 gammaCorrected = pow(mapped, vec3(1.0 / 2.2));

    finalColor = vec4(gammaCorrected, 1.0);
}
