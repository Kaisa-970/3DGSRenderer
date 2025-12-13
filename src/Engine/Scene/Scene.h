#pragma once

#include "Core.h"
#include "Renderer/Renderable.h"
#include <vector>
#include <memory>
#include <unordered_map>

GSENGINE_NAMESPACE_BEGIN

class GSENGINE_API Scene 
{
public:
    Scene();
    ~Scene();

    unsigned int AddRenderable(const std::shared_ptr<Renderer::Renderable>& renderable);
    void RemoveRenderable(const std::shared_ptr<Renderer::Renderable>& renderable);
    void ClearRenderables();

    std::shared_ptr<Renderer::Renderable> GetRenderableByUID(unsigned int uid) const;
    const std::vector<std::shared_ptr<Renderer::Renderable>>& GetRenderables() const { return renderables_; }

    //void draw(const Shader& shader);
private:
    unsigned int nextUid_;
    std::vector<std::shared_ptr<Renderer::Renderable>> renderables_;
    std::unordered_map<unsigned int, std::weak_ptr<Renderer::Renderable>> uidMap_;
};

GSENGINE_NAMESPACE_END