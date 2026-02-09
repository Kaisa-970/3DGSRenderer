#pragma once

#include "Core.h"
#include "Renderer/Renderable.h"
#include "Renderer/Light.h"
#include <vector>
#include <memory>
#include <unordered_map>

GSENGINE_NAMESPACE_BEGIN

class GSENGINE_API Scene 
{
public:
    Scene();
    ~Scene();

    // ---- Renderable 管理 ----
    unsigned int AddRenderable(const std::shared_ptr<Renderer::Renderable>& renderable);
    void RemoveRenderable(const std::shared_ptr<Renderer::Renderable>& renderable);
    void ClearRenderables();

    std::shared_ptr<Renderer::Renderable> GetRenderableByUID(unsigned int uid) const;
    const std::vector<std::shared_ptr<Renderer::Renderable>>& GetRenderables() const { return renderables_; }

    // ---- Light 管理 ----
    void AddLight(const std::shared_ptr<Renderer::Light>& light);
    void RemoveLight(const std::shared_ptr<Renderer::Light>& light);
    void ClearLights();
    const std::vector<std::shared_ptr<Renderer::Light>>& GetLights() const { return lights_; }

private:
    unsigned int nextUid_;
    std::vector<std::shared_ptr<Renderer::Renderable>> renderables_;
    std::unordered_map<unsigned int, std::weak_ptr<Renderer::Renderable>> uidMap_;
    std::vector<std::shared_ptr<Renderer::Light>> lights_;
};

GSENGINE_NAMESPACE_END