#ifndef __DAILY_JOB_H__
#define __DAILY_JOB_H__

#include <string>
#include <functional>
#include <chrono>
#include <atomic>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

/**
 * Simple daily job scheduler.
 */
class DailyJob
{
public:
    DailyJob(int const& hour, std::string const& name, std::function<void()> const& job, std::function<void()> const& jobCallback);
    ~DailyJob();

    void start();
    void stop();

private:
    void runJobLoop_();
    [[nodiscard]] std::chrono::system_clock::time_point calculateNextRun_() const noexcept;

    int m_hour;
    std::string m_name;
    const std::function<void()> m_job;
    const std::function<void()> m_jobCallback;

    std::atomic<bool> m_bRunning{false};
    std::unique_ptr<std::thread> m_jobThread;
    std::mutex m_jobMtx;
    std::condition_variable m_jobCV;
};

#endif /* __DAILY_JOB_H__ */
