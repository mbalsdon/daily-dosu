#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <stdexcept>
#include <cstddef>

class ThreadPool
{
public:
    explicit ThreadPool(
        std::size_t numThreads = std::thread::hardware_concurrency())
        : m_stop(false)
    {
        if (numThreads == 0)
        {
            numThreads = 1;
        }

        m_workers.reserve(numThreads);
        for (std::size_t i = 0; i < numThreads; ++i)
        {
            m_workers.emplace_back(
            [this]
            {
                workerThread_();
            });
        }
    }

    ~ThreadPool()
    {
        shutdown();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /**
     * Submit task to thread pool.
     */
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        using ReturnType = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<ReturnType> result = task->get_future();

        {
            std::unique_lock<std::mutex> lock(m_queueMtx);

            if (m_stop)
            {
                throw std::runtime_error("Cannot submit task to stopped ThreadPool");
            }

            m_tasks.emplace([task]()
            {
                (*task)();
            });
        }

        m_condition.notify_one();
        return result;
    }

    /**
     * Stop all threads in pool.
     */
    void shutdown()
    {
        {
            std::unique_lock<std::mutex> lock(m_queueMtx);
            if (m_stop)
            {
                return;
            }
            m_stop = true;
        }

        m_condition.notify_all();

        for (std::thread& worker : m_workers)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }
    }

    /**
     * Get number of working threads.
     */
    [[nodiscard]] std::size_t getThreadCount() const noexcept { return m_workers.size(); }

private:
    void workerThread_()
    {
        while (true)
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(m_queueMtx);

                m_condition.wait(lock,
                [this]
                {
                    return m_stop || !m_tasks.empty();
                });

                if (m_stop && m_tasks.empty())
                {
                    return;
                }

                task = std::move(m_tasks.front());
                m_tasks.pop();
            }

            task();
        }
    }

    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_queueMtx;
    std::condition_variable m_condition;
    bool m_stop;
};

#endif /* __THREAD_POOL_H__ */
