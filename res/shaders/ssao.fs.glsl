#version 430 core

in vec2 texCoord;
out float fragAO;

uniform sampler2D u_positionTexture;
uniform sampler2D u_normalTexture;
uniform sampler2D u_noiseTexture;

uniform mat4 viewMat;
uniform mat4 projMat;

// 半球采样核（在 C++ 中生成并传入，这里用 16 个）
const int KERNEL_SIZE = 16;
uniform vec3 u_kernel[16];

uniform vec2 u_noiseScale;  // (width/4, height/4) 用于噪声 UV 平铺
uniform float u_radius;      // 采样半径
uniform float u_bias;       // 深度偏移，避免自遮挡

void main()
{
    vec3 fragPos = texture(u_positionTexture, texCoord).rgb;
    vec3 normal = normalize(texture(u_normalTexture, texCoord).rgb);
    if (length(normal) < 0.01) { fragAO = 1.0; return; }

    // 变换到视图空间
    vec4 viewPos = viewMat * vec4(fragPos, 1.0);
    vec3 viewFragPos = viewPos.xyz;
    vec3 viewNormal = normalize(mat3(viewMat) * normal);

    // 用噪声旋转采样方向，减少带状伪影
    vec2 noiseUV = texCoord * u_noiseScale;
    vec3 randomVec = normalize(texture(u_noiseTexture, noiseUV).xyz);

    // TBN: 切线空间到视图空间，使采样沿法线半球
    vec3 tangent = normalize(randomVec - viewNormal * dot(randomVec, viewNormal));
    vec3 bitangent = cross(viewNormal, tangent);
    mat3 TBN = mat3(tangent, bitangent, viewNormal);

    float occlusion = 0.0;
    for (int i = 0; i < KERNEL_SIZE; ++i)
    {
        vec3 sampleOffset = TBN * u_kernel[i];
        vec3 sampleViewPos = viewFragPos + sampleOffset * u_radius;

        // 投影到屏幕 UV
        vec4 sampleClip = projMat * vec4(sampleViewPos, 1.0);
        vec3 sampleNDC = sampleClip.xyz / sampleClip.w;
        vec2 sampleUV = sampleNDC.xy * 0.5 + 0.5;

        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0)
            continue;

        float sampleDepth = (viewMat * vec4(texture(u_positionTexture, sampleUV).rgb, 1.0)).z;
        float rangeCheck = smoothstep(0.0, 1.0, u_radius / abs(viewFragPos.z - sampleDepth));
        if (sampleDepth <= viewFragPos.z - u_bias)
            occlusion += rangeCheck;
    }

    occlusion = 1.0 - (occlusion / float(KERNEL_SIZE));
    fragAO = occlusion;
}
