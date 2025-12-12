#pragma once

#include "Core.h"
#include "Event.h"
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

GSENGINE_NAMESPACE_BEGIN

class GSENGINE_API EventBus
{
public:
    using Handler = std::function<void(Event &)>;

    EventBus() = default;
    EventBus(const EventBus &) = delete;
    EventBus &operator=(const EventBus &) = delete;
    EventBus(EventBus &&) = default;
    EventBus &operator=(EventBus &&) = default;

    size_t Subscribe(EventType type, int priority, Handler handler);
    void Unsubscribe(EventType type, size_t handlerId);

    void Push(std::unique_ptr<Event> evt);

    template <typename T, typename... Args>
    void Emplace(Args &&...args)
    {
        Push(std::make_unique<T>(std::forward<Args>(args)...));
    }

    void Dispatch();

    void ClearQueue();

private:
    struct HandlerEntry
    {
        int priority;
        size_t id;
        Handler handler;
    };

    std::unordered_map<EventType, std::vector<HandlerEntry>> handlers_;
    std::vector<std::unique_ptr<Event>> queue_;
    size_t nextId_{1};
};

GSENGINE_NAMESPACE_END
