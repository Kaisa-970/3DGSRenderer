#include "PostProcessChain.h"
#include "RenderContext.h"
#include "RenderHelper/RenderHelper.h"
#include "Logger/Log.h"
#include <glad/glad.h>
#include <cstring>

RENDERER_NAMESPACE_BEGIN

static const float quadVertices[] = {
    // pos(xy)  uv(st)
    -1.0f, -1.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 1.0f, 0.0f,
     1.0f,  1.0f, 1.0f, 1.0f,
    -1.0f,  1.0f, 0.0f, 1.0f
};

PostProcessChain::PostProcessChain(int width, int height)
{
    // 创建共享全屏 Quad
    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // 创建 Ping-Pong 双缓冲纹理
    m_texA = RenderHelper::CreateTexture2D(width, height, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    m_texB = RenderHelper::CreateTexture2D(width, height, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    m_fboA.Attach(FrameBuffer::Attachment::Color0, m_texA);
    m_fboB.Attach(FrameBuffer::Attachment::Color0, m_texB);
}

PostProcessChain::~PostProcessChain()
{
    m_fboA.Detach(FrameBuffer::Attachment::Color0);
    m_fboB.Detach(FrameBuffer::Attachment::Color0);
    if (m_texA != 0) glDeleteTextures(1, &m_texA);
    if (m_texB != 0) glDeleteTextures(1, &m_texB);
    if (m_quadVAO != 0) glDeleteVertexArrays(1, &m_quadVAO);
    if (m_quadVBO != 0) glDeleteBuffers(1, &m_quadVBO);
}

void PostProcessChain::Execute(RenderContext &ctx)
{
    glDisable(GL_DEPTH_TEST);

    // 收集启用的效果
    std::vector<PostProcessEffect *> activeEffects;
    for (auto &effect : m_effects)
    {
        if (effect && effect->enabled)
            activeEffects.push_back(effect.get());
    }

    // 如果没有启用的效果，直接透传 lightingTex
    if (activeEffects.empty())
    {
        ctx.postProcessColorTex = ctx.lightingTex;
        return;
    }

    // Ping-Pong 执行
    // 第一个效果的输入 = ctx.lightingTex
    // 奇数效果写 texA，偶数效果写 texB（从 0 计）
    unsigned int inputTex = ctx.lightingTex;
    FrameBuffer *writeFBOs[] = {&m_fboA, &m_fboB};
    unsigned int writeTex[] = {m_texA, m_texB};
    int writeIdx = 0;

    for (size_t i = 0; i < activeEffects.size(); ++i)
    {
        // 绑定写入端 FBO
        writeFBOs[writeIdx]->Bind();
        glViewport(0, 0, ctx.width, ctx.height);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 执行效果
        activeEffects[i]->Apply(inputTex, m_quadVAO, ctx.width, ctx.height, ctx);

        writeFBOs[writeIdx]->Unbind();

        // 下一轮的输入 = 本轮的输出
        inputTex = writeTex[writeIdx];
        // 切换 ping-pong
        writeIdx = 1 - writeIdx;
    }

    // 最终结果 = 最后写入的纹理
    ctx.postProcessColorTex = inputTex;
}

void PostProcessChain::Resize(int width, int height)
{
    // 调整 ping-pong 纹理尺寸
    glBindTexture(GL_TEXTURE_2D, m_texA);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, m_texB);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    // 通知所有效果
    for (auto &effect : m_effects)
    {
        if (effect)
            effect->Resize(width, height);
    }
}

void PostProcessChain::AddEffect(std::unique_ptr<PostProcessEffect> effect)
{
    if (effect)
    {
        LOG_CORE_INFO("PostProcessChain: added effect '{}'", effect->GetName());
        m_effects.push_back(std::move(effect));
    }
}

PostProcessEffect *PostProcessChain::GetEffect(const char *name) const
{
    if (!name) return nullptr;
    for (auto &effect : m_effects)
    {
        if (effect && std::strcmp(effect->GetName(), name) == 0)
            return effect.get();
    }
    return nullptr;
}

bool PostProcessChain::RemoveEffect(const char *name)
{
    if (!name) return false;
    for (auto it = m_effects.begin(); it != m_effects.end(); ++it)
    {
        if (*it && std::strcmp((*it)->GetName(), name) == 0)
        {
            LOG_CORE_INFO("PostProcessChain: removed effect '{}'", name);
            m_effects.erase(it);
            return true;
        }
    }
    return false;
}

RENDERER_NAMESPACE_END
