#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t numThreads)
{
    m_workers.reserve(numThreads);

    for (size_t i = 0; i < numThreads; ++i)
    {
        m_workers.emplace_back([this] {
            while (true)
            {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(m_queueMutex);

                    m_condition.wait(lock, [this] { return m_stop || !m_tasks.empty(); });

                    if (m_stop && m_tasks.empty())
                    {
                        break;
                    }

                    task = std::move(m_tasks.front());
                    m_tasks.pop();

                    m_activeCount++;
                }

                task();

                {
                    std::unique_lock<std::mutex> lock(m_queueMutex);
                    m_activeCount--;
                    m_condition.notify_all();
                }
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_stop = true;
    }

    m_condition.notify_all();

    for (auto &worker : m_workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
}

void ThreadPool::enqueue(std::function<void()> task)
{
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_tasks.emplace(std::move(task));
    }

    m_condition.notify_one();
}

void ThreadPool::waitAll()
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    m_condition.wait(lock, [this] { return m_tasks.empty() && m_activeCount == 0; });
}
