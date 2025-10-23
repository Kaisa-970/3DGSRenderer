#pragma once

#include "RenderCore.h"

RENDERER_NAMESPACE_BEGIN

typedef float FLOAT;
typedef unsigned char UCHAR;

struct GaussianPoint {
    FLOAT position[3];
    FLOAT color[3];
    FLOAT normal[3];
    FLOAT texCoord[2];
};

#pragma pack(push, 1)
struct NormalPoint {
    FLOAT position[3];
    FLOAT normal[3];
    UCHAR color[3];
};
#pragma pack(pop)

RENDERER_NAMESPACE_END