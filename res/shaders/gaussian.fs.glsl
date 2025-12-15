#version 430 core

in vec3 outColor;
in float opacity;
in vec3 conic;
in vec2 coordxy;
in float quadId;
in float viewDepth;
in vec3 worldPos;
out vec4 FragColor;

uniform sampler2D u_solidDepthTexture;
uniform int u_useSolidDepth;
uniform vec2 u_screenSize;

uniform vec3 u_selectBoxPos;
uniform vec3 u_selectBoxSize;
uniform int u_deleteSelectPoints;
uniform vec3 u_selectColor;

float linearizeDepth(float depth, float near, float far) {
    float z = depth * 2.0 - 1.0; // 转换到 NDC [-1, 1]
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main() {
    float power = -0.5f * (conic.x * coordxy.x * coordxy.x + conic.z * coordxy.y * coordxy.y) - conic.y * coordxy.x * coordxy.y;
    if(power > 0.0f) discard;
    float alpha = min(0.99f, opacity * exp(power));
    if(alpha < 1.f / 255.f) discard;

    if (u_useSolidDepth == 1) {
        // 获取当前片段的屏幕坐标
        vec2 screenUV = gl_FragCoord.xy / u_screenSize;

        // 采样实体物体的深度
        float solidDepth = texture(u_solidDepthTexture, screenUV).r;

        // 将深度从 [0,1] 转换为线性深度（需要匹配你的投影参数）
        float near = 0.01;
        float far = 1000.0;
        float solidLinearDepth = linearizeDepth(solidDepth, near, far);

        // 如果高斯在实体物体后面，丢弃
        if (viewDepth > solidLinearDepth && solidDepth < 0.9999) {
            discard;
        }
    }

    vec3 finalColor = outColor;
    vec2 screenCoord = gl_FragCoord.xy;
    vec3 halfSize = u_selectBoxSize / 2.0f;
    if (worldPos.x > u_selectBoxPos.x - halfSize.x && worldPos.x < u_selectBoxPos.x + halfSize.x
    && worldPos.y > u_selectBoxPos.y - halfSize.y && worldPos.y < u_selectBoxPos.y + halfSize.y
    && worldPos.z > u_selectBoxPos.z - halfSize.z && worldPos.z < u_selectBoxPos.z + halfSize.z) {
        finalColor = finalColor * 0.5f + u_selectColor * 0.5f;
        finalColor = clamp(finalColor, 0.0f, 1.0f);
        if (u_deleteSelectPoints == 1) {
            alpha = 0.0f;
        }
        //alpha *= 0.0f;
    }
    FragColor = vec4(finalColor, alpha);
}
