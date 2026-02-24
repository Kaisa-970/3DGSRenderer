#pragma once

#include "Core/RenderCore.h"
#include "MathUtils/Vector.h"
#include "MathUtils/Matrix.h"

RENDERER_NAMESPACE_BEGIN

/// 光源类型
enum class LightType
{
    Directional, // 平行光（如太阳光）
    Point,       // 点光源
    Spot         // 聚光灯
};

/// 光源数据结构
/// 描述场景中一个光源的所有属性
class RENDERER_API Light
{
public:
    Light() = default;
    ~Light() = default;

    // --- 基础属性 ---
    LightType type = LightType::Point;
    Vector3 position{0.0f, 10.0f, 10.0f};
    Vector3 direction{0.0f, -1.0f, -1.0f}; // 仅 Directional / Spot 有效
    Vector3 color{1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;

    // --- 衰减参数（Point / Spot）---
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;

    // --- 聚光灯参数（Spot）---
    float innerCutoffDeg = 12.5f;
    float outerCutoffDeg = 17.5f;

    Mat4 GetViewProjectionMatrix() const
    {
        if (type == LightType::Directional)
        {
            Mat4 view = Mat4::LookAt(position, position + direction, Vector3(0.0f, 1.0f, 0.0f));
            float orthoSize = 20.0f;
            Mat4 projection = Mat4::Ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 100.0f);
            return projection * view;
        }
        else if (type == LightType::Point)
        {
            return Mat4::Identity();
        }
        else if (type == LightType::Spot)
        {
            Mat4 view = Mat4::LookAt(position, position + direction, Vector3(0.0f, 1.0f, 0.0f));
            Mat4 projection = Mat4::Perspective(outerCutoffDeg * 2.0f, 1.0f, 0.1f, 100.0f);
            return projection * view;
        }
        return Mat4::Identity();
    }

    // --- 便捷工厂方法 ---
    static Light CreatePointLight(const Vector3 &pos, const Vector3 &color = Vector3(1.0f, 1.0f, 1.0f),
                                  float intensity = 1.0f)
    {
        Light l;
        l.type = LightType::Point;
        l.position = pos;
        l.color = color;
        l.intensity = intensity;
        return l;
    }

    static Light CreateDirectionalLight(const Vector3 &dir, const Vector3 &color = Vector3(1.0f, 1.0f, 1.0f),
                                        float intensity = 1.0f)
    {
        Light l;
        l.type = LightType::Directional;
        l.direction = dir;
        l.color = color;
        l.intensity = intensity;
        return l;
    }
};

RENDERER_NAMESPACE_END
