#pragma once

#include "Core/BaseDef.h"
#include <vector>

RENDERER_NAMESPACE_BEGIN

template <int D>
struct SHs
{
    FLOAT shs[(D + 1) * (D + 1) * 3];
};

struct GSPosition
{
    FLOAT position[3];
};

struct GSNormal
{
    FLOAT normal[3];
};

struct GSOpacity
{
    FLOAT opacity;
};

struct GSScale
{
    FLOAT scale[3];
};

struct GSRotation
{
    FLOAT rotation[4];
};

template <int D>
struct GaussianPoint
{
    GSPosition position;
    GSNormal normal;
    SHs<D> shs;
    GSOpacity opacity;
    GSScale scale;
    GSRotation rotation;
};

template <int D>
struct GaussianData
{
    std::vector<GSPosition> positions;
    std::vector<GSNormal> normals;
    std::vector<SHs<D>> shs;
    std::vector<GSOpacity> opacitys;
    std::vector<GSScale> scales;
    std::vector<GSRotation> rotations;
};

#pragma pack(push, 1)
struct NormalPoint
{
    FLOAT position[3];
    FLOAT normal[3];
    UCHAR color[3];
};
#pragma pack(pop)

RENDERER_NAMESPACE_END
