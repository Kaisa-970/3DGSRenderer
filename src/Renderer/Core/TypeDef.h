#pragma once

#include "RenderCore.h"

RENDERER_NAMESPACE_BEGIN

typedef float FLOAT;
typedef unsigned char UCHAR;

template<int D>
struct SHs{
    FLOAT shs[(D+1)*(D+1)*3];
};

template<int D>
struct GaussianPoint {
    FLOAT position[3];
    FLOAT normal[3];
    SHs<D> shs;
    FLOAT opacity;
    FLOAT scale[3];
    FLOAT rotation[4];
};

#pragma pack(push, 1)
struct NormalPoint {
    FLOAT position[3];
    FLOAT normal[3];
    UCHAR color[3];
};
#pragma pack(pop)

RENDERER_NAMESPACE_END