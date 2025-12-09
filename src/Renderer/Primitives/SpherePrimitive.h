#pragma once

#include "Primitive.h"

RENDERER_NAMESPACE_BEGIN

class RENDERER_API SpherePrimitive : public Primitive {
public:
    // radius: 球体半径
    // sectorCount: 经度方向的分段数（水平）
    // stackCount: 纬度方向的分段数（垂直）
    explicit SpherePrimitive(float radius = 1.0f, 
                            int sectorCount = 36, 
                            int stackCount = 18);
    ~SpherePrimitive() override = default;

private:
    void generateSphere(float radius, int sectorCount, int stackCount);
};

RENDERER_NAMESPACE_END

