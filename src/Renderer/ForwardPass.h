#pragma once

#include "Core/RenderCore.h"
#include "FrameBuffer.h"
#include "Shader.h"
#include "Renderable.h"
#include <vector>
#include <memory>

RENDERER_NAMESPACE_BEGIN

class RENDERER_API ForwardPass
{
public:
    ForwardPass();
    ~ForwardPass() = default;

    void Render(int width,
                int height,
                const float *view,
                const float *projection,
                unsigned int colorTexture,
                unsigned int depthTexture,
                const std::vector<std::shared_ptr<Renderable>> &renderables,
                std::shared_ptr<Shader> shader,
                float timeSeconds);

private:
    FrameBuffer m_frameBuffer;
};

RENDERER_NAMESPACE_END

