#version 330 core

// 顶点属性（每个高斯椭球的数据）
layout (location = 0) in vec3 aPosition;     // 位置
layout (location = 1) in vec3 aNormal;       // 法线（暂不使用）
layout (location = 2) in vec3 aSH0;          // 球谐系数第0组
layout (location = 3) in float aOpacity;     // 不透明度
layout (location = 4) in vec3 aScale;        // 缩放
layout (location = 5) in vec4 aRotation;     // 四元数旋转

// 四边形顶点偏移（-1,-1, 1,-1, -1,1, 1,1）
layout (location = 6) in vec2 aQuadOffset;   

// Uniform变量
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uCameraPos;
uniform vec2 uViewport;     // 屏幕分辨率
uniform float uFocalX;      // 焦距X
uniform float uFocalY;      // 焦距Y

// 输出到片段着色器
out vec2 vQuadCoord;        // 四边形局部坐标 [-1,1]
out vec3 vColor;            // 颜色
out float vAlpha;           // Alpha值
out vec2 vCov2DParam1;      // 2D协方差参数
out vec2 vCov2DParam2;      // 2D协方差参数

// 球谐函数计算颜色
vec3 evalSH(vec3 sh0) {
    // 简化版：只使用第0阶球谐（DC分量）
    // 在3DGS中，SH系数存储为logit空间，需要sigmoid激活
    vec3 color = 1.0 / (1.0 + exp(-sh0));
    
    return color;
}

// 四元数转旋转矩阵
mat3 quaternionToMatrix(vec4 q) {
    // 归一化
    q = normalize(q);
    
    float xx = q.x * q.x;
    float yy = q.y * q.y;
    float zz = q.z * q.z;
    float xy = q.x * q.y;
    float xz = q.x * q.z;
    float yz = q.y * q.z;
    float wx = q.w * q.x;
    float wy = q.w * q.y;
    float wz = q.w * q.z;

    return mat3(
        1.0 - 2.0*(yy + zz), 2.0*(xy - wz), 2.0*(xz + wy),
        2.0*(xy + wz), 1.0 - 2.0*(xx + zz), 2.0*(yz - wx),
        2.0*(xz - wy), 2.0*(yz + wx), 1.0 - 2.0*(xx + yy)
    );
}

// 计算3D协方差矩阵
mat3 compute3DCovariance(vec3 scale, vec4 rotation) {
    mat3 R = quaternionToMatrix(rotation);
    mat3 S = mat3(
        scale.x, 0.0, 0.0,
        0.0, scale.y, 0.0,
        0.0, 0.0, scale.z
    );
    mat3 M = R * S;
    return M * transpose(M);
}

void main() {
    // === 完整的椭圆投影版本 ===
    
    // 0. 激活函数：scale是log空间的，需要exp激活
    vec3 scale = exp(aScale);
    
    // 1. 计算3D协方差矩阵
    mat3 cov3D = compute3DCovariance(scale, aRotation);
    
    // 2. 变换到世界空间
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    
    // 3. 变换到视图空间
    vec4 viewPos = uView * worldPos;
    float tz = viewPos.z;
    
    // 限制深度避免除零
    float limz = max(tz, 0.001);
    float limz2 = limz * limz;
    
    // 4. 提取视图矩阵的旋转部分
    mat3 W = mat3(uView);
    
    // 5. 将协方差变换到视图空间：Σ' = W * Σ * W^T
    mat3 T = W * cov3D;
    mat3 cov3DView = T * transpose(W);
    
    // 6. 构建雅可比矩阵（投影的导数）
    // J 是 2x3 矩阵 (投影雅可比)
    float tx = viewPos.x;
    float ty = viewPos.y;
    
    float j00 = uFocalX / limz;
    float j02 = -(uFocalX * tx) / limz2;
    float j11 = uFocalY / limz;
    float j12 = -(uFocalY * ty) / limz2;
    
    // 7. 投影到2D：cov2D = J * cov3DView * J^T
    // 手动展开矩阵乘法以避免GLSL矩阵类型问题
    
    // 首先计算 temp = cov3DView * J^T (3x3 * 3x2 = 3x2)
    // J^T 是 3x2:
    // [ j00  0   ]
    // [ 0    j11 ]
    // [ j02  j12 ]
    
    float temp00 = cov3DView[0][0] * j00 + cov3DView[0][2] * j02;
    float temp01 = cov3DView[0][1] * j11 + cov3DView[0][2] * j12;
    float temp10 = cov3DView[1][0] * j00 + cov3DView[1][2] * j02;
    float temp11 = cov3DView[1][1] * j11 + cov3DView[1][2] * j12;
    float temp20 = cov3DView[2][0] * j00 + cov3DView[2][2] * j02;
    float temp21 = cov3DView[2][1] * j11 + cov3DView[2][2] * j12;
    
    // 然后计算 cov2D = J * temp (2x3 * 3x2 = 2x2)
    // J 是 2x3:
    // [ j00  0    j02 ]
    // [ 0    j11  j12 ]
    
    mat2 cov2D;
    cov2D[0][0] = j00 * temp00 + j02 * temp20;  // (0,0)
    cov2D[0][1] = j11 * temp10 + j12 * temp20;  // (1,0)
    cov2D[1][0] = j00 * temp01 + j02 * temp21;  // (0,1)
    cov2D[1][1] = j11 * temp11 + j12 * temp21;  // (1,1)
    
    // 8. 添加低通滤波器防止走样（平衡清晰度和稳定性）
    cov2D[0][0] += 0.3;
    cov2D[1][1] += 0.3;
    
    // 9. 计算椭圆的边界半径（3-sigma）
    float det = cov2D[0][0] * cov2D[1][1] - cov2D[0][1] * cov2D[1][0];
    float trace = cov2D[0][0] + cov2D[1][1];
    float discriminant = max(0.0, trace * trace - 4.0 * det);
    float lambda1 = (trace + sqrt(discriminant)) * 0.5;
    float lambda2 = (trace - sqrt(discriminant)) * 0.5;
    
    // 限制椭圆的长宽比，避免过度拉伸
    float maxAspectRatio = 3.0;  // 最大长宽比
    if (lambda1 > lambda2 * maxAspectRatio * maxAspectRatio) {
        lambda1 = lambda2 * maxAspectRatio * maxAspectRatio;
    }
    
    float radius = 3.0 * sqrt(max(lambda1, lambda2));
    
    // 合理的splat尺寸缩放
    // 如果半径异常（NaN或太小），使用固定值
    if (isnan(radius) || isinf(radius) || radius < 0.3) {
        radius = 2.0;  // 使用合理的默认值
    } else {
        radius = clamp(radius, 0.5, 50.0);  // 保持原始尺寸，适度限制上限
    }
    
    // 10. 投影到屏幕空间
    vec4 clipPos = uProjection * viewPos;
    vec2 ndcCenter = clipPos.xy / clipPos.w;
    
    // 11. 计算四边形顶点位置（在屏幕像素空间偏移）
    // 将半径从像素空间转换到NDC空间
    vec2 pixelOffset = aQuadOffset * radius;
    vec2 ndcOffset = pixelOffset / uViewport * 2.0;
    
    // 在NDC空间应用偏移
    vec2 finalNDC = ndcCenter + ndcOffset;
    
    // 重建裁剪空间坐标
    gl_Position = vec4(finalNDC * clipPos.w, clipPos.z, clipPos.w);
    
    // 12. 计算颜色（从球谐系数）
    vColor = evalSH(aSH0);
    vColor = max(vColor, vec3(0.0));  // 避免负数颜色
    
    // 调试：暂时使用固定颜色看看是否能渲染
    // vColor = vec3(1.0, 0.0, 0.0); // 红色
    
    // 13. 传递参数到片段着色器
    vQuadCoord = aQuadOffset;
    // opacity是logit空间的，需要sigmoid激活: sigmoid(x) = 1 / (1 + exp(-x))
    vAlpha = 1.0 / (1.0 + exp(-aOpacity));
    
    // 存储协方差矩阵（用于片段着色器计算高斯权重）
    vCov2DParam1 = vec2(cov2D[0][0], cov2D[0][1]);
    vCov2DParam2 = vec2(cov2D[1][0], cov2D[1][1]);
}

