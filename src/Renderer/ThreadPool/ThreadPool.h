#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

/**
 * @brief 简单的线程池实现
 *
 * 功能：
 * - 创建固定数量的工作线程
 * - 提交任务到任务队列
 * - 等待所有任务完成
 *
 * 使用示例：
 *   ThreadPool pool(8);  // 创建8个工作线程
 *
 *   // 提交任务
 *   pool.enqueue([]{
 *       // 你的任务代码
 *   });
 *
 *   // 等待所有任务完成
 *   pool.waitAll();
 */
class ThreadPool
{
public:
    /**
     * @brief 构造线程池
     * @param numThreads 工作线程数量
     */
    ThreadPool(size_t numThreads);

    /**
     * @brief 析构，等待所有任务完成并停止线程
     */
    ~ThreadPool();

    /**
     * @brief 提交一个任务到线程池
     * @param task 要执行的任务（无参数无返回值的函数）
     */
    void enqueue(std::function<void()> task);

    /**
     * @brief 等待所有已提交的任务完成
     * 注意：此函数会阻塞，直到任务队列为空且所有线程都空闲
     */
    void waitAll();

    /**
     * @brief 获取工作线程数量
     */
    size_t getThreadCount() const
    {
        return m_workers.size();
    }

private:
    std::vector<std::thread> m_workers;

    std::queue<std::function<void()>> m_tasks;

    std::mutex m_queueMutex;

    std::condition_variable m_condition;

    std::atomic<int> m_activeCount{0};

    std::atomic<bool> m_stop{false};
};
