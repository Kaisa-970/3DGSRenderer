#include "Effects/OutlineEffect.h"
#include "RenderContext.h"
#include <glad/glad.h>

RENDERER_NAMESPACE_BEGIN

OutlineEffect::OutlineEffect(const std::shared_ptr<Shader> &shader)
    : m_shader(shader)
{
}

void OutlineEffect::Apply(unsigned int inputTex, unsigned int quadVAO,
                           int width, int height, const RenderContext &ctx)
{
    m_shader->use();

    // 输入：上一个效果的输出（即 lightingTex 或前一个效果处理后的结果）
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTex);
    m_shader->setInt("u_lightingTexture", 0);

    // G-Buffer 额外数据（描边需要位置、法线、深度、UID）
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ctx.gPositionTex);
    m_shader->setInt("u_positionTexture", 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, ctx.gNormalTex);
    m_shader->setInt("u_normalTexture", 2);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, ctx.gDepthTex);
    m_shader->setInt("u_depthTexture", 3);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, ctx.gUIDTex);
    m_shader->setInt("u_uidTexture", 4);

    // 相机位置 + 选中 UID
    if (ctx.camera)
    {
        m_shader->setVec3("viewPos", ctx.camera->getPosition().x,
                          ctx.camera->getPosition().y,
                          ctx.camera->getPosition().z);
    }
    m_shader->setInt("u_currentSelectedUID", ctx.selectedUID);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);

    m_shader->unuse();
}

RENDERER_NAMESPACE_END
