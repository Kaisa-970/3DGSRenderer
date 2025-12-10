#pragma once

#include "Core/RenderCore.h"
#include "Renderable.h"
#include <vector>
#include <memory>
#include <unordered_map>

RENDERER_NAMESPACE_BEGIN

class RENDERER_API Scene 
{
public:
    Scene();
    ~Scene();

    unsigned int AddRenderable(const std::shared_ptr<Renderable>& renderable);
    void RemoveRenderable(const std::shared_ptr<Renderable>& renderable);
    void ClearRenderables();

    std::shared_ptr<Renderable> GetRenderableByUID(unsigned int uid) const;
    const std::vector<std::shared_ptr<Renderable>>& GetRenderables() const { return renderables_; }

    //void draw(const Shader& shader);
private:
    unsigned int nextUid_;
    std::vector<std::shared_ptr<Renderable>> renderables_;
    std::unordered_map<unsigned int, std::weak_ptr<Renderable>> uidMap_;
};

RENDERER_NAMESPACE_END