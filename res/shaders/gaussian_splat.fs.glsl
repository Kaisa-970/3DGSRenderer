#version 330 core

// 从顶点着色器传入
in vec2 vQuadCoord;        // 四边形局部坐标 [-1, 1]
in vec3 vColor;            // 颜色
in float vAlpha;           // 不透明度
in vec2 vCov2DParam1;      // 2D协方差矩阵第一行
in vec2 vCov2DParam2;      // 2D协方差矩阵第二行

// 输出
out vec4 FragColor;

void main() {
    // === 完整椭圆版本（更准确的高斯渲染） ===
    
    // 1. 重建2D协方差矩阵
    // 注意：GLSL mat2 是列主序，vCov2DParam1和vCov2DParam2分别是第0列和第1列
    mat2 cov2D = mat2(vCov2DParam1, vCov2DParam2);
    
    // 2. 检查协方差矩阵是否有效
    float det = cov2D[0][0] * cov2D[1][1] - cov2D[0][1] * cov2D[1][0];
    
    // 如果行列式太小，退回到圆形
    if (abs(det) < 1e-6 || isnan(det) || isinf(det)) {
        float dist = length(vQuadCoord);
        if (dist > 1.0) discard;
        float gaussian = exp(-dist * dist * 0.5);
        float alpha = gaussian * vAlpha * 1.0;
        if (alpha < 0.01) discard;
        FragColor = vec4(vColor * alpha, alpha);
        return;
    }

    // 计算逆矩阵：对于2x2矩阵 [a b; c d]，逆为 (1/det) * [d -b; -c a]
    // 注意GLSL列主序：mat2的参数是按列填充的
    float invDet = 1.0 / det;
    mat2 invCov2D = mat2(
        cov2D[1][1] * invDet,  -cov2D[1][0] * invDet,  // 第0列
        -cov2D[0][1] * invDet, cov2D[0][0] * invDet    // 第1列
    );
    
    // 3. 计算马氏距离（在椭圆空间中）
    vec2 d = vQuadCoord;
    vec2 invCov_d = invCov2D * d;
    float power = -0.5 * dot(d, invCov_d);
    
    // 范围检查（使用合理的截断阈值）
    if (power < -9.0 || isnan(power) || isinf(power)) discard;
    
    // 计算高斯权重
    float gaussian = exp(power);
    
    // 应用不透明度（适度的强度以获得良好覆盖）
    float alpha = gaussian * vAlpha * 0.6;
    
    // 丢弃几乎完全透明的像素
    if (alpha < 0.005) discard;
    
    // 输出颜色（预乘alpha）
    FragColor = vec4(vColor * alpha, alpha);
}
