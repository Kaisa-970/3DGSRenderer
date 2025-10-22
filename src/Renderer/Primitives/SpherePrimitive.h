#pragma once

#include "Primitive.h"

RENDERER_NAMESPACE_BEGIN

class RENDERER_API SpherePrimitive : public Primitive {
public:
    // radius: 球体半径
    // sectorCount: 经度方向的分段数（水平）
    // stackCount: 纬度方向的分段数（垂直）
    // colored: 是否使用渐变色
    explicit SpherePrimitive(float radius = 1.0f, 
                            int sectorCount = 36, 
                            int stackCount = 18, 
                            bool colored = false);
    ~SpherePrimitive() override = default;

private:
    void generateSphere(float radius, int sectorCount, int stackCount, bool colored);
};

RENDERER_NAMESPACE_END

