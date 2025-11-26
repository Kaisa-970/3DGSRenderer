#pragma once

#include "Core/RenderCore.h"
#include "Shader.h"

RENDERER_NAMESPACE_BEGIN

class RENDERER_API FinalPass {
public:
    FinalPass();
    ~FinalPass();

    void render(int width, int height, const unsigned int& colorTexture);

private:
    //unsigned int m_framebuffer;
    unsigned int m_colorTexture;
    Shader m_shader;
    unsigned int m_vao;
    unsigned int m_vbo;
};

RENDERER_NAMESPACE_END