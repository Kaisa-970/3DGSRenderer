#pragma once

#include "TypeDef.h"
#include <string>

RENDERER_NAMESPACE_BEGIN

class PlyLoader
{
public:
    PlyLoader();
    ~PlyLoader();

    bool LoadGaussianPlyAoS(const std::string &path, std::vector<GaussianPoint<0>> &gaussianPoints);
    bool LoadGaussianPlySoA(const std::string &path, GaussianData<0> &gaussianData);
};

RENDERER_NAMESPACE_END
