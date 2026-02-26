#include "SSAOPass.h"
#include "RenderContext.h"
#include "RenderHelper/RenderHelper.h"
#include "MathUtils/Random.h"
#include <glad/glad.h>
#include <cmath>

RENDERER_NAMESPACE_BEGIN

static const float s_quadVertices[] = {
    -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,
    1.0f,  1.0f,  1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 1.0f};

SSAOPass::SSAOPass(int width, int height, const std::shared_ptr<Shader> &shader)
    : m_shader(shader), m_width(width), m_height(height)
{
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_quadVertices), s_quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_aoTexture = RenderHelper::CreateTexture2D(width, height, GL_R32F, GL_RED, GL_FLOAT);
    m_frameBuffer.Attach(FrameBuffer::Attachment::Color0, m_aoTexture);

    initNoiseTexture();
    initKernel();
}

void SSAOPass::initNoiseTexture()
{
    const int noiseSize = 4;
    float noiseData[noiseSize * noiseSize * 4];
    for (int i = 0; i < noiseSize * noiseSize; ++i)
    {
        float x = Random::randomFloat(-1.0f, 1.0f);
        float y = Random::randomFloat(-1.0f, 1.0f);
        float len = std::sqrt(x * x + y * y);
        if (len > 1e-5f)
        {
            x /= len;
            y /= len;
        }
        noiseData[i * 4 + 0] = x;
        noiseData[i * 4 + 1] = y;
        noiseData[i * 4 + 2] = 0.0f;
        noiseData[i * 4 + 3] = 1.0f;
    }

    glGenTextures(1, &m_noiseTexture);
    glBindTexture(GL_TEXTURE_2D, m_noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, noiseSize, noiseSize, 0, GL_RGBA, GL_FLOAT, noiseData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void SSAOPass::initKernel()
{
    for (int i = 0; i < KERNEL_SIZE; ++i)
    {
        float x = Random::randomFloat(-1.0f, 1.0f);
        float y = Random::randomFloat(-1.0f, 1.0f);
        float z = Random::randomFloat(0.0f, 1.0f);
        float len = std::sqrt(x * x + y * y + z * z);
        if (len < 1e-5f)
            len = 1.0f;
        x /= len;
        y /= len;
        z /= len;
        float scale = static_cast<float>(i) / static_cast<float>(KERNEL_SIZE);
        scale = 0.1f + 0.9f * scale * scale;
        m_kernel[i * 3 + 0] = x * scale;
        m_kernel[i * 3 + 1] = y * scale;
        m_kernel[i * 3 + 2] = z * scale;
    }
}

SSAOPass::~SSAOPass()
{
    m_frameBuffer.Detach(FrameBuffer::Attachment::Color0);
    if (m_aoTexture != 0)
        glDeleteTextures(1, &m_aoTexture);
    if (m_noiseTexture != 0)
        glDeleteTextures(1, &m_noiseTexture);
    if (m_vao != 0)
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != 0)
        glDeleteBuffers(1, &m_vbo);
}

void SSAOPass::Execute(RenderContext &ctx)
{
    if (!ctx.ssaoEnabled || ctx.gPositionTex == 0 || ctx.gNormalTex == 0)
    {
        ctx.ssaoTex = 0;
        return;
    }

    m_frameBuffer.Bind();
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    m_shader->use();

    m_shader->setMat4("viewMat", ctx.viewMatrix);
    m_shader->setMat4("projMat", ctx.projMatrix);
    m_shader->setVec2("u_noiseScale", static_cast<float>(ctx.width) / 4.0f, static_cast<float>(ctx.height) / 4.0f);
    m_shader->setFloat("u_radius", 0.5f);
    // 适当增大 bias，避免平整地面等因深度误差被判成遮挡导致整体偏暗
    m_shader->setFloat("u_bias", 0.065f);

    for (int i = 0; i < KERNEL_SIZE; ++i)
    {
        char name[32];
        snprintf(name, sizeof(name), "u_kernel[%d]", i);
        m_shader->setVec3(name, m_kernel[i * 3], m_kernel[i * 3 + 1], m_kernel[i * 3 + 2]);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ctx.gPositionTex);
    m_shader->setInt("u_positionTexture", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ctx.gNormalTex);
    m_shader->setInt("u_normalTexture", 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_noiseTexture);
    m_shader->setInt("u_noiseTexture", 2);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);

    m_shader->unuse();
    m_frameBuffer.Unbind();

    ctx.ssaoTex = m_aoTexture;
}

void SSAOPass::Resize(int width, int height)
{
    if (width <= 0 || height <= 0)
        return;
    m_width = width;
    m_height = height;
    glBindTexture(GL_TEXTURE_2D, m_aoTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}

RENDERER_NAMESPACE_END
