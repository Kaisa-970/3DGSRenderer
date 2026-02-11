#version 430 core

in vec2 texCoord;

out vec4 FragColor;

uniform sampler2D u_positionTexture;
uniform sampler2D u_normalTexture;
uniform sampler2D u_lightingTexture;
uniform sampler2D u_depthTexture;
uniform vec3 viewPos;

// 描边参数
uniform float u_edgeThreshold = 0.2;      // 边缘检测阈值
uniform vec3 u_edgeColor = vec3(1.0, 0.0, 0.0);  // 描边颜色（黑色）
uniform float u_edgeWidth = 1.0;          // 描边宽度
uniform float u_depthSensitivity = 0.2;   // 深度敏感度
uniform float u_normalSensitivity = 0.2;  // 法线敏感度

uniform isampler2D u_uidTexture;
uniform int u_currentSelectedUID;

// 相机近/远平面（与 projection matrix 保持一致）
uniform float u_nearPlane = 0.01;
uniform float u_farPlane = 1000.0;

float linearizeDepth(float depth, float near, float far) {
    float z = depth * 2.0 - 1.0; // 转换到 NDC [-1, 1]
    return (2.0 * near * far) / (far + near - z * (far - near));
}

// Sobel算子 - 深度边缘检测
float depthEdgeDetection(vec2 uv, vec2 texelSize) {
    // Sobel X核
    float sobelX[9];
    sobelX[0] = -1.0; sobelX[1] = 0.0; sobelX[2] = 1.0;
    sobelX[3] = -2.0; sobelX[4] = 0.0; sobelX[5] = 2.0;
    sobelX[6] = -1.0; sobelX[7] = 0.0; sobelX[8] = 1.0;

    // Sobel Y核
    float sobelY[9];
    sobelY[0] = -1.0; sobelY[1] = -2.0; sobelY[2] = -1.0;
    sobelY[3] =  0.0; sobelY[4] =  0.0; sobelY[5] =  0.0;
    sobelY[6] =  1.0; sobelY[7] =  2.0; sobelY[8] =  1.0;

    float edgeX = 0.0;
    float edgeY = 0.0;

    // 采样3x3邻域
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            vec2 offset = vec2(float(i), float(j)) * texelSize * u_edgeWidth;
            float depth = texture(u_depthTexture, uv + offset).r;
            depth = linearizeDepth(depth, u_nearPlane, u_farPlane);

            int index = (i + 1) * 3 + (j + 1);
            edgeX += depth * sobelX[index];
            edgeY += depth * sobelY[index];
        }
    }

    return sqrt(edgeX * edgeX + edgeY * edgeY) * u_depthSensitivity;
}

// 法线边缘检测
float normalEdgeDetection(vec2 uv, vec2 texelSize) {
    vec3 normalCenter = texture(u_normalTexture, uv).rgb;

    float edge = 0.0;

    // Roberts Cross算子（简化版）
    vec3 n1 = texture(u_normalTexture, uv + vec2(texelSize.x, 0.0) * u_edgeWidth).rgb;
    vec3 n2 = texture(u_normalTexture, uv + vec2(0.0, texelSize.y) * u_edgeWidth).rgb;
    vec3 n3 = texture(u_normalTexture, uv + vec2(-texelSize.x, 0.0) * u_edgeWidth).rgb;
    vec3 n4 = texture(u_normalTexture, uv + vec2(0.0, -texelSize.y) * u_edgeWidth).rgb;

    // 计算法线差异
    float diff1 = length(normalCenter - n1);
    float diff2 = length(normalCenter - n2);
    float diff3 = length(normalCenter - n3);
    float diff4 = length(normalCenter - n4);

    edge = max(max(diff1, diff2), max(diff3, diff4)) * u_normalSensitivity;

    return edge;
}

void main()
{
    vec3 position = texture(u_positionTexture, texCoord).rgb;
    float distance = length(position - viewPos);

    float depth = texture(u_depthTexture, texCoord).r;
    depth = linearizeDepth(depth, u_nearPlane, u_farPlane) / u_farPlane;

    vec3 normal = texture(u_normalTexture, texCoord).rgb;
    normal = normalize(normal);

    // 获取纹理尺寸（动态获取，不再硬编码分辨率）
    vec2 texelSize = 1.0 / vec2(textureSize(u_depthTexture, 0));

    // 检测深度边缘
    float depthEdge = depthEdgeDetection(texCoord, texelSize);

    // 检测法线边缘
    float normalEdge = normalEdgeDetection(texCoord, texelSize);

    // 综合边缘强度
    float edge = max(depthEdge, normalEdge);

        // 应用阈值
    float edgeFactor = smoothstep(u_edgeThreshold - 0.05, u_edgeThreshold + 0.05, edge);

    // 获取原始颜色
    vec4 originalColor = texture(u_lightingTexture, texCoord);
    int uid = texture(u_uidTexture, texCoord).r;
    if (uid > 0 && uid == u_currentSelectedUID) {
        FragColor = mix(originalColor, vec4(u_edgeColor, 1.0), edgeFactor);
        return;
    }

    FragColor = originalColor;
}
