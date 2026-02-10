#include "ForwardPass.h"
#include "RenderContext.h"
#include <algorithm>
#include <glad/glad.h>
#include <vector>

RENDERER_NAMESPACE_BEGIN

ForwardPass::ForwardPass()
{
}

void ForwardPass::Execute(RenderContext &ctx)
{
    if (!ctx.forwardRenderables || ctx.forwardRenderables->empty())
        return;

    // 将颜色写入后处理阶段的颜色纹理，同时复用几何阶段的深度纹理以保持遮挡关系
    m_frameBuffer.Attach(FrameBuffer::Attachment::Color0, ctx.lightingTex);
    m_frameBuffer.Attach(FrameBuffer::Attachment::Depth, ctx.gDepthTex);
    m_frameBuffer.Bind();

    glViewport(0, 0, ctx.width, ctx.height);
    glDepthFunc(GL_LEQUAL);

    struct SortedItem
    {
        const RenderContext::ForwardRenderItem *item;
        float dist2;
    };

    float camX = 0.0f;
    float camY = 0.0f;
    float camZ = 0.0f;
    if (ctx.camera)
        ctx.camera->getPosition(camX, camY, camZ);

    std::vector<SortedItem> sorted;
    sorted.reserve(ctx.forwardRenderables->size());
    for (const auto &item : *ctx.forwardRenderables)
    {
        if (!item.renderable)
            continue;

        const auto &m = item.renderable->getTransform().m;
        // 该工程中的 model 矩阵通常以转置后形式上传，平移分量在 m[12..14]。
        float x = m[12], y = m[13], z = m[14];
        float dx = x - camX;
        float dy = y - camY;
        float dz = z - camZ;
        sorted.push_back({&item, dx * dx + dy * dy + dz * dz});
    }

    std::sort(sorted.begin(), sorted.end(), [](const SortedItem &a, const SortedItem &b) {
        return a.dist2 > b.dist2; // 透明物体通常采用远到近排序
    });

    RenderContext::ForwardRenderState currentState{};
    bool hasState = false;
    auto applyState = [&](const RenderContext::ForwardRenderState &state) {
        if (state.depthTest)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
        glDepthMask(state.depthWrite ? GL_TRUE : GL_FALSE);
        if (state.cullFace)
            glEnable(GL_CULL_FACE);
        else
            glDisable(GL_CULL_FACE);
        if (state.blending)
        {
            glEnable(GL_BLEND);
            if (state.blendMode == RenderContext::ForwardBlendMode::Additive)
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            else
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        else
        {
            glDisable(GL_BLEND);
        }
    };

    std::shared_ptr<Shader> currentShader = nullptr;
    for (const auto &entry : sorted)
    {
        const auto &item = *entry.item;
        auto &r = item.renderable;
        if (!r)
            continue;

        if (!hasState || item.state.blending != currentState.blending || item.state.depthTest != currentState.depthTest ||
            item.state.depthWrite != currentState.depthWrite || item.state.cullFace != currentState.cullFace ||
            item.state.blendMode != currentState.blendMode)
        {
            applyState(item.state);
            currentState = item.state;
            hasState = true;
        }

        // 物体可自带 shader；若为空则回退到全局默认 forward shader
        auto drawShader = item.shader ? item.shader : ctx.forwardShader;
        if (!drawShader)
            continue;

        if (drawShader != currentShader)
        {
            if (currentShader)
                currentShader->unuse();
            currentShader = drawShader;
            currentShader->use();
            currentShader->setMat4("view", ctx.viewMatrix);
            currentShader->setMat4("projection", ctx.projMatrix);
            currentShader->setFloat("u_time", ctx.currentTime);

            if (ctx.camera)
            {
                float vx, vy, vz;
                ctx.camera->getPosition(vx, vy, vz);
                currentShader->setVec3("u_viewPos", vx, vy, vz);
            }
        }

        currentShader->setMat4("model", r->getTransform().m);
        currentShader->setInt("uUID", static_cast<int>(r->getUid()));
        currentShader->setVec3("uColor", r->getColor().x, r->getColor().y, r->getColor().z);

        if (r->getType() == RenderableType::Primitive && r->getPrimitive())
        {
            if (r->getMaterial())
                r->getMaterial()->UpdateShaderParams(currentShader);
            r->getPrimitive()->draw();
        }
        else if (r->getType() == RenderableType::Model && r->getModel())
        {
            r->getModel()->draw(currentShader);
        }
    }

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    if (currentShader)
        currentShader->unuse();
    m_frameBuffer.Unbind();
}

RENDERER_NAMESPACE_END
