# 图元 (Primitives) 使用指南

## 概述

本项目提供了三种基础几何图元：
- **CubePrimitive**: 立方体
- **SpherePrimitive**: 球体
- **QuadPrimitive**: 四边形平面

所有图元都包含完整的顶点属性：位置、颜色、法线、纹理坐标。

---

## SpherePrimitive - 球体

### 构造函数

```cpp
SpherePrimitive(float radius = 1.0f, 
                int sectorCount = 36, 
                int stackCount = 18, 
                bool colored = false);
```

### 参数说明

- **radius**: 球体半径（默认 1.0）
- **sectorCount**: 经度方向的分段数（水平圈数，默认 36）
  - 值越大，球体越圆滑
  - 建议范围：16 - 64
- **stackCount**: 纬度方向的分段数（垂直层数，默认 18）
  - 值越大，球体越圆滑
  - 建议范围：8 - 32
- **colored**: 是否使用渐变色（默认 false）
  - `true`: 基于顶点位置的彩虹色渐变
  - `false`: 纯白色

### 几何特性

- **顶点数**: `(sectorCount + 1) × (stackCount + 1)`
- **三角形数**: `sectorCount × stackCount × 2`
- **法线**: 所有法线指向球心外侧（归一化）
- **纹理坐标**: UV 球形映射 (0-1 范围)

### 使用示例

```cpp
// 标准球体
Renderer::SpherePrimitive sphere(1.0f);

// 高质量球体
Renderer::SpherePrimitive sphereHQ(1.0f, 64, 32);

// 彩色球体（用于调试）
Renderer::SpherePrimitive coloredSphere(1.0f, 36, 18, true);

// 绘制
sphere.draw(shader);
```

### 性能建议

| 用途 | sectorCount | stackCount | 顶点数 |
|------|-------------|------------|--------|
| 低精度（远景） | 16 | 8 | 153 |
| 中等精度 | 36 | 18 | 703 |
| 高精度 | 64 | 32 | 2,145 |
| 超高精度 | 128 | 64 | 8,321 |

---

## QuadPrimitive - 四边形

### 构造函数

```cpp
QuadPrimitive(float size = 1.0f, bool colored = false);
```

### 参数说明

- **size**: 四边形的边长（默认 1.0）
- **colored**: 是否使用彩色（默认 false）
  - `true`: 四个顶点分别为红、绿、蓝、黄
  - `false`: 纯白色

### 几何特性

- **位置**: 位于 XY 平面上，中心在原点
- **法线**: 指向 +Z 方向 (0, 0, 1)
- **顶点顺序**: 
  - 0: 左下 (-s, -s, 0)
  - 1: 右下 (+s, -s, 0)
  - 2: 右上 (+s, +s, 0)
  - 3: 左上 (-s, +s, 0)
- **纹理坐标**: 标准 UV 映射 (0,0) 到 (1,1)
- **顶点数**: 4
- **三角形数**: 2

### 使用示例

```cpp
// 标准白色四边形
Renderer::QuadPrimitive quad(2.0f);

// 彩色四边形（用于调试）
Renderer::QuadPrimitive coloredQuad(1.0f, true);

// 绘制
quad.draw(shader);
```

### 常见用途

- 地面平面
- UI 背景
- 粒子效果
- 天空盒的一个面
- 阴影接收平面

---

## CubePrimitive - 立方体

### 构造函数

```cpp
CubePrimitive(float size = 1.0f, bool colored = true);
```

### 参数说明

- **size**: 立方体的边长（默认 1.0）
- **colored**: 是否使用彩色（默认 true）

### 几何特性

- **顶点数**: 24 (每个面 4 个顶点)
- **三角形数**: 12 (每个面 2 个三角形)
- **法线**: 每个面有独立的法线
- **颜色**: 每个顶点有不同的颜色（如果 colored = true）

---

## 着色器兼容性

### Lambert 着色器 (推荐用于光照)

```glsl
// lambert.vs.glsl
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;
```

所有图元都兼容 Lambert 着色器，提供：
- 环境光
- 漫反射（Lambert）
- 镜面反射（Phong）

### 简单着色器 (用于调试)

```glsl
// cube.vs.glsl
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
```

---

## 完整使用示例

### 示例 1: 带光照的球体

```cpp
#include "Renderer/Primitives/SpherePrimitive.h"
#include "Renderer/Shader.h"
#include "Renderer/Camera.h"

// 创建球体
Renderer::SpherePrimitive sphere(1.5f, 48, 24, false);

// 加载 Lambert 着色器
Renderer::Shader shader = Renderer::Shader::fromFiles(
    "res/shaders/lambert.vs.glsl",
    "res/shaders/lambert.fs.glsl"
);

// 在渲染循环中
shader.use();

// 设置变换
shader.setMat4("model", Renderer::Matrix4::identity().m);
shader.setMat4("view", camera.getViewMatrix().m);
shader.setMat4("projection", projectionMatrix);

// 设置光照
shader.setVec3("lightPos", 3.0f, 3.0f, 3.0f);
shader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
shader.setVec3("objectColor", 0.8f, 0.3f, 0.3f);
shader.setVec3("viewPos", camera.getPosition());
shader.setFloat("ambientStrength", 0.2f);
shader.setFloat("specularStrength", 0.5f);
shader.setInt("shininess", 32);

// 绘制
sphere.draw(shader);
```

### 示例 2: 地面平面

```cpp
#include "Renderer/Primitives/QuadPrimitive.h"

// 创建大的地面平面
Renderer::QuadPrimitive ground(20.0f, false);

// 旋转 90 度使其平行于 XZ 平面
Renderer::Matrix4 groundModel = 
    Renderer::Matrix4::rotationX(-90.0f * PI / 180.0f);

shader.use();
shader.setMat4("model", groundModel.m);
shader.setVec3("objectColor", 0.5f, 0.5f, 0.5f);  // 灰色地面

ground.draw(shader);
```

### 示例 3: 多个球体组成的场景

```cpp
// 创建不同大小的球体
Renderer::SpherePrimitive spheres[] = {
    Renderer::SpherePrimitive(1.0f, 36, 18),
    Renderer::SpherePrimitive(0.5f, 24, 12),
    Renderer::SpherePrimitive(0.3f, 16, 8)
};

float positions[][3] = {
    {0.0f, 0.0f, 0.0f},
    {2.5f, 0.0f, 0.0f},
    {-2.5f, 0.0f, 0.0f}
};

float colors[][3] = {
    {1.0f, 0.0f, 0.0f},  // 红
    {0.0f, 1.0f, 0.0f},  // 绿
    {0.0f, 0.0f, 1.0f}   // 蓝
};

shader.use();

for (int i = 0; i < 3; i++) {
    Renderer::Matrix4 model = Renderer::Matrix4::translation(
        positions[i][0], positions[i][1], positions[i][2]
    );
    
    shader.setMat4("model", model.m);
    shader.setVec3("objectColor", colors[i][0], colors[i][1], colors[i][2]);
    
    spheres[i].draw(shader);
}
```

---

## 顶点数据布局

所有图元使用统一的 Vertex 结构：

```cpp
struct Vertex {
    float position[3];   // location = 0
    float color[3];      // location = 1
    float normal[3];     // location = 2
    float texCoord[2];   // location = 3 (预留)
};
```

### 内存布局

```
| position (12 bytes) | color (12 bytes) | normal (12 bytes) | texCoord (8 bytes) |
|        vec3         |       vec3       |       vec3        |       vec2         |
```

总大小: 44 bytes per vertex

---

## 注意事项

1. **法线方向**: 
   - SpherePrimitive: 法线指向外侧
   - QuadPrimitive: 法线指向 +Z
   - CubePrimitive: 每个面的法线垂直于该面

2. **性能优化**:
   - 对于远处的球体，降低 sectorCount 和 stackCount
   - 使用索引绘制减少顶点重复

3. **颜色使用**:
   - `colored = true` 主要用于调试和测试
   - 在 Lambert 着色器中，顶点颜色会被 objectColor 覆盖

4. **坐标系**:
   - 右手坐标系
   - Y 轴向上

---

## 扩展

如果需要其他图元，可以参考现有实现：

- **圆柱体**: 类似球体，但顶部和底部是圆形
- **圆锥体**: 一端收缩的圆柱体
- **环形**: 使用参数化方程
- **自定义网格**: 继承 Primitive 类

示例：

```cpp
class TorusPrimitive : public Primitive {
public:
    TorusPrimitive(float majorRadius, float minorRadius, 
                   int majorSegments, int minorSegments);
private:
    void generateTorus(...);
};
```

