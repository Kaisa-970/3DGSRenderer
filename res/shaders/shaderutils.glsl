float linearizeDepth(float depth, float near, float far) {
    float z = depth * 2.0 - 1.0; // 转换到 NDC [-1, 1]
    return (2.0 * near * far) / (far + near - z * (far - near));
}