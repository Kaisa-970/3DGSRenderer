#version 430 core

in vec3 outColor;
in float opacity;
in vec3 conic;
in vec2 coordxy;
in float quadId;
in float distance;
in float viewDepth;
out vec4 FragColor;

uniform sampler2D u_solidDepthTexture;
uniform int u_useSolidDepth;
uniform vec2 u_screenSize;

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

    FragColor = vec4(outColor, alpha);
    //FragColor = vec4(outColor, 1.0);
    float value = quadId / 559263.0;
    // value = distance;
    // value = 1.0 / (1.0 + exp(-value));
    //FragColor = vec4(value, value, value, 1.0);
}