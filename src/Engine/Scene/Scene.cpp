#include "Scene.h"
#include <algorithm>

GSENGINE_NAMESPACE_BEGIN

Scene::Scene()
    : nextUid_(1)
{
}

Scene::~Scene()
{
    ClearRenderables();
}

unsigned int Scene::AddRenderable(const std::shared_ptr<Renderer::Renderable>& renderable)
{
    if (!renderable) return 0;
    if (renderable->getUid() == 0) {
        renderable->setUid(nextUid_++);
    }
    renderables_.push_back(renderable);
    uidMap_[renderable->getUid()] = renderable;
    return renderable->getUid();
}

void Scene::RemoveRenderable(const std::shared_ptr<Renderer::Renderable>& renderable)
{
    if (!renderable) return;
    uidMap_.erase(renderable->getUid());
    renderables_.erase(std::remove(renderables_.begin(), renderables_.end(), renderable), renderables_.end());
}

void Scene::ClearRenderables()
{
    renderables_.clear();
    uidMap_.clear();
}

std::shared_ptr<Renderer::Renderable> Scene::GetRenderableByUID(unsigned int uid) const
{
    auto it = uidMap_.find(uid);
    if (it == uidMap_.end()) return nullptr;
    return it->second.lock();
}

GSENGINE_NAMESPACE_END