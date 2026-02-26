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
///     → ForwardPass    读取 lightingTex + depthTex，写回 lightingTex
///     → PostProcessPass 读取 G-Buffer + lightingTex，写入 postProcessColorTex
///     → FinalPass      读取 displayTex，输出到屏幕
struct RENDERER_API RenderContext
{
    enum class ForwardBlendMode
    {
        Alpha,    // src * a + dst * (1 - a)
        Additive, // src * a + dst
    };

    struct ForwardRenderState
    {
        ForwardBlendMode blendMode = ForwardBlendMode::Alpha;
        bool blending = true;
        bool depthTest = true;
        bool depthWrite = false;
        bool cullFace = false;
    };

    struct ForwardRenderItem
    {
        std::shared_ptr<Renderable> renderable;
        std::shared_ptr<Shader> shader;
        ForwardRenderState state;
    };

    // ==== 输入（由 RenderPipeline 在执行 Pass 之前填充）====
    Camera *camera = nullptr;
    int width = 0;
    int height = 0;
    float currentTime = 0.0f;
    int selectedUID = -1;
    const std::vector<std::shared_ptr<Light>> *lights = nullptr;

    // 场景物体
    const std::vector<std::shared_ptr<Renderable>> *sceneRenderables = nullptr;

    // 前向渲染资源（每个物体可选独立 shader，未设置时回退到 forwardShader）
    const std::vector<ForwardRenderItem> *forwardRenderables = nullptr;
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
    unsigned int gUIDTex = 0;
    unsigned int gDepthTex = 0;
    unsigned int gMetallicRoughnessTex = 0;

    // 阴影结果（ShadowPass 输出）
    unsigned int shadowTex = 0;

    // SSAO 结果（SSAOPass 输出，用于调制环境光）
    unsigned int ssaoTex = 0;
    bool ssaoEnabled = true;
    /// SSAO 强度 [0,1]，用于 mix(1, ao, strength)，避免整体过暗
    float ssaoStrength = 0.75f;

    // 光照结果（LightingPass 输出）
    unsigned int lightingTex = 0;

    // 后处理结果（PostProcessPass 输出）
    unsigned int postProcessColorTex = 0;

    // 光照参数
    float shininess = 32.0f;
    float ambientStrength = 0.1f;
    float diffuseStrength = 0.9f;
    float specularStrength = 0.5f;

    // ==== HDR / Tone Mapping 参数 ====
    float exposure = 1.0f; // 曝光度（默认 1.0）
    int tonemapMode = 2;   // 0 = None, 1 = Reinhard, 2 = ACES Filmic（默认 ACES）

    // ==== 最终输出 ====

    // 显示纹理（由 RenderPipeline 根据 ViewMode 选择，FinalPass 的输入）
    unsigned int displayTex = 0;
    /// 为 true 时 FinalPass 将 displayTex 按单通道 R 复制为 RGB（用于 SSAO 等灰度预览）
    bool displaySingleChannelR = false;

    // FinalPass 输出（经过 ToneMapping + Gamma 的 LDR 纹理）
    unsigned int finalTex = 0;
};

RENDERER_NAMESPACE_END
