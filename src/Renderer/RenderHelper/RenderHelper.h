#pragma once

#include "Core/RenderCore.h"

RENDERER_NAMESPACE_BEGIN


class RENDERER_API RenderHelper {
public:
    static unsigned int CreateTexture2D(int width, int height, int internalFormat, int format, int type);
};

RENDERER_NAMESPACE_END