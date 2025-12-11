#include "EventBus.h"
#include <algorithm>

RENDERER_NAMESPACE_BEGIN

size_t EventBus::Subscribe(EventType type, int priority, Handler handler) {
    if (!handler) return 0;
    size_t id = nextId_++;
    auto& vec = handlers_[type];
    vec.push_back({priority, id, std::move(handler)});
    std::sort(vec.begin(), vec.end(), [](const HandlerEntry& a, const HandlerEntry& b) {
        return a.priority > b.priority; // 高优先级先处理
    });
    return id;
}

void EventBus::Unsubscribe(EventType type, size_t handlerId) {
    auto it = handlers_.find(type);
    if (it == handlers_.end()) return;
    auto& vec = it->second;
    vec.erase(std::remove_if(vec.begin(), vec.end(), [handlerId](const HandlerEntry& e) {
        return e.id == handlerId;
    }), vec.end());
}

void EventBus::Push(std::unique_ptr<Event> evt) {
    if (!evt) return;
    queue_.push_back(std::move(evt));
}

void EventBus::Dispatch() {
    for (auto& evtPtr : queue_) {
        if (!evtPtr) continue;
        auto hit = handlers_.find(evtPtr->type);
        if (hit == handlers_.end()) continue;
        for (auto& entry : hit->second) {
            if (evtPtr->handled) break;
            if (entry.handler) {
                entry.handler(*evtPtr);
            }
        }
    }
    queue_.clear();
}

void EventBus::ClearQueue() {
    queue_.clear();
}

RENDERER_NAMESPACE_END

