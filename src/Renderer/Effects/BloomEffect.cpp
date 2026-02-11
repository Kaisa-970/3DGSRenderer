#include "Effects/BloomEffect.h"
#include "RenderContext.h"
#include "RenderHelper/RenderHelper.h"
#include <glad/glad.h>

RENDERER_NAMESPACE_BEGIN

BloomEffect::BloomEffect(int width, int height,
                         const std::shared_ptr<Shader> &thresholdShader,
                         const std::shared_ptr<Shader> &blurShader,
                         const std::shared_ptr<Shader> &compositeShader)
    : m_thresholdShader(thresholdShader),
      m_blurShader(blurShader),
      m_compositeShader(compositeShader)
{
    m_texThreshold = RenderHelper::CreateTexture2D(width, height, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    m_texBlurA = RenderHelper::CreateTexture2D(width, height, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    m_texBlurB = RenderHelper::CreateTexture2D(width, height, GL_RGBA16F, GL_RGBA, GL_FLOAT);

    m_fboThreshold.Attach(FrameBuffer::Attachment::Color0, m_texThreshold);
    m_fboBlurA.Attach(FrameBuffer::Attachment::Color0, m_texBlurA);
    m_fboBlurB.Attach(FrameBuffer::Attachment::Color0, m_texBlurB);
}

BloomEffect::~BloomEffect()
{
    m_fboThreshold.Detach(FrameBuffer::Attachment::Color0);
    m_fboBlurA.Detach(FrameBuffer::Attachment::Color0);
    m_fboBlurB.Detach(FrameBuffer::Attachment::Color0);
    if (m_texThreshold != 0) glDeleteTextures(1, &m_texThreshold);
    if (m_texBlurA != 0) glDeleteTextures(1, &m_texBlurA);
    if (m_texBlurB != 0) glDeleteTextures(1, &m_texBlurB);
}

void BloomEffect::Apply(unsigned int inputTex, unsigned int quadVAO,
                         int width, int height, const RenderContext &ctx)
{
    (void)ctx;

    // 保存链的 FBO（Apply 被调用时，链已绑好写入端 FBO）
    GLint chainFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &chainFBO);

    // ---- 第 1 步：亮度提取 ----
    m_fboThreshold.Bind();
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    m_thresholdShader->use();
    m_thresholdShader->setFloat("u_threshold", threshold);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTex);
    m_thresholdShader->setInt("u_inputTexture", 0);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
    m_thresholdShader->unuse();

    // ---- 第 2 步：分离式高斯模糊（水平 + 垂直 × N 次迭代）----
    bool horizontal = true;
    unsigned int blurInput = m_texThreshold;

    m_blurShader->use();
    for (int i = 0; i < blurIterations * 2; ++i)
    {
        FrameBuffer &writeFBO = horizontal ? m_fboBlurA : m_fboBlurB;
        writeFBO.Bind();
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        m_blurShader->setInt("u_horizontal", horizontal ? 1 : 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, blurInput);
        m_blurShader->setInt("u_inputTexture", 0);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);

        blurInput = horizontal ? m_texBlurA : m_texBlurB;
        horizontal = !horizontal;
    }
    m_blurShader->unuse();

    // blurInput = 最终模糊结果的纹理

    // ---- 第 3 步：合成（原图 + bloom → 链的 FBO）----
    // 恢复链的 FBO，合成结果直接写到链的 ping-pong 纹理
    glBindFramebuffer(GL_FRAMEBUFFER, chainFBO);
    glViewport(0, 0, width, height);
    // 不要 clear —— 链在 Apply 前已经 clear 过了

    m_compositeShader->use();
    m_compositeShader->setFloat("u_intensity", intensity);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTex); // 原始场景
    m_compositeShader->setInt("u_sceneTexture", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blurInput); // 模糊后的高亮
    m_compositeShader->setInt("u_bloomTexture", 1);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);

    m_compositeShader->unuse();
}

void BloomEffect::Resize(int width, int height)
{
    auto resizeTex = [width, height](unsigned int tex) {
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    };
    resizeTex(m_texThreshold);
    resizeTex(m_texBlurA);
    resizeTex(m_texBlurB);
    glBindTexture(GL_TEXTURE_2D, 0);
}

RENDERER_NAMESPACE_END
