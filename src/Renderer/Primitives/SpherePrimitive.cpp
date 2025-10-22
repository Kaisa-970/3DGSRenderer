#include "SpherePrimitive.h"
#include <cmath>

RENDERER_NAMESPACE_BEGIN

static const float PI = 3.14159265358979323846f;

SpherePrimitive::SpherePrimitive(float radius, int sectorCount, int stackCount, bool colored) {
    generateSphere(radius, sectorCount, stackCount, colored);
}

void SpherePrimitive::generateSphere(float radius, int sectorCount, int stackCount, bool colored) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    
    float x, y, z, xy;                              // 顶点位置
    float nx, ny, nz, lengthInv = 1.0f / radius;   // 法线
    float s, t;                                     // 纹理坐标
    
    float sectorStep = 2 * PI / sectorCount;
    float stackStep = PI / stackCount;
    float sectorAngle, stackAngle;
    
    // 生成顶点
    for (int i = 0; i <= stackCount; ++i) {
        stackAngle = PI / 2 - i * stackStep;        // 从 π/2 到 -π/2
        xy = radius * std::cos(stackAngle);         // r * cos(u)
        z = radius * std::sin(stackAngle);          // r * sin(u)
        
        // 为当前 stack 添加 (sectorCount+1) 个顶点
        for (int j = 0; j <= sectorCount; ++j) {
            sectorAngle = j * sectorStep;           // 从 0 到 2π
            
            // 顶点位置 (x, y, z)
            x = xy * std::cos(sectorAngle);         // r * cos(u) * cos(v)
            y = xy * std::sin(sectorAngle);         // r * cos(u) * sin(v)
            
            // 归一化后的法线
            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            
            // 纹理坐标
            s = (float)j / sectorCount;
            t = (float)i / stackCount;
            
            // 颜色（基于位置的渐变色或白色）
            float r, g, b;
            if (colored) {
                r = (nx + 1.0f) * 0.5f;  // 将 -1~1 映射到 0~1
                g = (ny + 1.0f) * 0.5f;
                b = (nz + 1.0f) * 0.5f;
            } else {
                r = g = b = 1.0f;
            }
            
            Vertex vertex;
            vertex.position[0] = x;
            vertex.position[1] = y;
            vertex.position[2] = z;
            vertex.color[0] = r;
            vertex.color[1] = g;
            vertex.color[2] = b;
            vertex.normal[0] = nx;
            vertex.normal[1] = ny;
            vertex.normal[2] = nz;
            vertex.texCoord[0] = s;
            vertex.texCoord[1] = t;
            
            vertices.push_back(vertex);
        }
    }
    
    // 生成索引
    // k1--k1+1
    // |  / |
    // | /  |
    // k2--k2+1
    unsigned int k1, k2;
    for (int i = 0; i < stackCount; ++i) {
        k1 = i * (sectorCount + 1);     // 当前 stack 的起始索引
        k2 = k1 + sectorCount + 1;      // 下一个 stack 的起始索引
        
        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            // 每个四边形由两个三角形组成
            // k1 => k2 => k1+1
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }
            
            // k1+1 => k2 => k2+1
            if (i != (stackCount - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
    
    setupBuffers(vertices, indices);
}

RENDERER_NAMESPACE_END

