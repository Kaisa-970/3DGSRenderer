#include "QuadPrimitive.h"

RENDERER_NAMESPACE_BEGIN

QuadPrimitive::QuadPrimitive(float size, bool colored) {
    generateQuad(size, colored);
}

void QuadPrimitive::generateQuad(float size, bool colored) {
    float s = size * 0.5f;
    
    // 四边形在 XY 平面上，法线指向 +Z
    std::vector<Vertex> vertices = {
        // 位置                 颜色                      法线              纹理坐标
        {{-s, -s, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // 左下 - 红色
        {{ s, -s, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},  // 右下 - 绿色
        {{ s,  s, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},  // 右上 - 蓝色
        {{-s,  s, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},  // 左上 - 黄色
    };
    
    // 如果不需要彩色，统一设置为白色
    if (!colored) {
        for (auto& v : vertices) {
            v.color[0] = v.color[1] = v.color[2] = 1.0f;
        }
    }
    
    // 索引（两个三角形）
    std::vector<unsigned int> indices = {
        0, 1, 2,  // 第一个三角形
        2, 3, 0   // 第二个三角形
    };
    
    setupBuffers(vertices, indices);
}

RENDERER_NAMESPACE_END

