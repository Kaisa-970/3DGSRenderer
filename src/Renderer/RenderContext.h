#pragma once

#include "Core/RenderCore.h"
#include "Camera.h"
#include "Light.h"
#include "Renderable.h"
#include "Shader.h"
#include <memory>
#include <vector>

RENDERER_NAMESPACE_BEGIN

/// RenderContext: Pass 之间的共享黑板
/// 
/// 管线执行时，RenderPipeline 先填充输入数据，然后依次执行各 Pass。
/// 每个 Pass 从 ctx 读取它需要的输入，执行渲染后将输出写回 ctx。
/// 后续 Pass 就能从 ctx 读取前面 Pass 的产出，形成数据流。
///
/// 数据流向:
///   RenderPipeline 填充输入
///     → GeometryPass   写入 G-Buffer 纹理
///     → LightingPass   读取 G-Buffer，写入 lightingTex
///     → PostProcessPass 读取 G-Buffer + lightingTex，写入 postProcessColorTex
///     → ForwardPass    读取 postProcessColorTex + depthTex，写入 postProcessColorTex
///     → FinalPass      读取 displayTex，输出到屏幕
struct RENDERER_API RenderContext
{
    // ==== 输入（由 RenderPipeline 在执行 Pass 之前填充）====
    Camera* camera = nullptr;
    int width = 0;
    int height = 0;
    float currentTime = 0.0f;
    unsigned int selectedUID = 0;
    const Light* light = nullptr;

    // 场景物体
    const std::vector<std::shared_ptr<Renderable>>* sceneRenderables = nullptr;

    // 前向渲染资源
    const std::vector<std::shared_ptr<Renderable>>* forwardRenderables = nullptr;
    std::shared_ptr<Shader> forwardShader;

    // 预计算的矩阵
    float viewMatrix[16] = {};
    float projMatrix[16] = {};

    // ==== 中间产物（由各 Pass 写入，后续 Pass 读取）====

    // G-Buffer（GeometryPass 输出）
    unsigned int gPositionTex = 0;
    unsigned int gNormalTex = 0;
    unsigned int gDiffuseTex = 0;
    unsigned int gSpecularTex = 0;
    unsigned int gShininessTex = 0;
    unsigned int gUIDTex = 0;
    unsigned int gDepthTex = 0;

    // 光照结果（LightingPass 输出）
    unsigned int lightingTex = 0;

    // 后处理结果（PostProcessPass 输出）
    unsigned int postProcessColorTex = 0;

    // ==== 最终输出 ====

    // 显示纹理（由 RenderPipeline 根据 ViewMode 选择）
    unsigned int displayTex = 0;
};

RENDERER_NAMESPACE_END
