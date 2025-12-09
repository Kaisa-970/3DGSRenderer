#include "CubePrimitive.h"

RENDERER_NAMESPACE_BEGIN

CubePrimitive::CubePrimitive(float size) {
    generateCube(size);
}

void CubePrimitive::generateCube(float size) {
    float s = size * 0.5f;
    
    std::vector<Vertex> vertices = {
        // 前面 (z = -s)
        {{-s, -s, -s}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{ s, -s, -s}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
        {{ s,  s, -s}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
        {{-s,  s, -s}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
        
        // 后面 (z = s)
        {{-s, -s,  s}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ s, -s,  s}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ s,  s,  s}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-s,  s,  s}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        
        // 左面 (x = -s)
        {{-s, -s, -s}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{-s,  s, -s}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{-s,  s,  s}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{-s, -s,  s}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        
        // 右面 (x = s)
        {{ s, -s, -s}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{ s,  s, -s}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{ s,  s,  s}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{ s, -s,  s}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        
        // 底面 (y = -s)
        {{-s, -s, -s}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ s, -s, -s}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
        {{ s, -s,  s}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-s, -s,  s}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
        
        // 顶面 (y = s)
        {{-s,  s, -s}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ s,  s, -s}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{ s,  s,  s}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-s,  s,  s}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
    };
    
    std::vector<unsigned int> indices = {
        0,  2,  1,  0,  3,  2,   // 前面
        4,  5,  6,  4,  6,  7,   // 后面
        8,  10,  9,  8,  11, 10,   // 左面
        12, 13, 14, 12, 14, 15,  // 右面
        16, 17, 18, 16, 18, 19,  // 底面
        20, 22, 21, 20, 23, 22   // 顶面
    };
    
    setupBuffers(vertices, indices);
}

RENDERER_NAMESPACE_END