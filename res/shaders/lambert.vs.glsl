#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;

// 输出到片段着色器
out vec3 FragPos;    // 世界空间中的片段位置
out vec3 Normal;     // 世界空间中的法线

// 变换矩阵
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // 计算世界空间中的片段位置
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    // 计算世界空间中的法线（使用法线矩阵处理非均匀缩放）
    // 法线矩阵 = transpose(inverse(model))
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    // 计算最终的顶点位置
    gl_Position = projection * view * vec4(FragPos, 1.0);
}

