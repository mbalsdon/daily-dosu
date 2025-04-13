#include "DailyJob.h"
#include "Logger.h"
#include "Util.h"

#include <iostream>
#include <thread>
#include <ctime>
#include <utility>

namespace
{
/**
 * Convert hour to 0-23 if it is out-of-bounds.
 */
[[nodiscard]] int normalizeHour(int const& hour) noexcept
{
    if (hour < 0 || hour > 23)
    {
        int normalizedHour = (24 + (hour % 24)) % 24;
        LOG_WARN("Hour is out of bounds, normalizing to ", normalizedHour);
        return normalizedHour;
    }
    else
    {
        return hour;
    }
}
} /* namespace */

/**
 * DailyJob constructor.
 */
DailyJob::DailyJob(int const& hour, std::string const& name, std::function<void()> const& job, std::function<void()> const& jobCallback)
    : m_hour(normalizeHour(hour))
    , m_name(name)
    , m_job(job)
    , m_jobCallback(jobCallback)
{
    LOG_ERROR_THROW(
        m_job,
        "Job function for ", name, " must exist!"
    );
}

/**
 * DailyJob destructor.
 */
DailyJob::~DailyJob()
{
    stop();
}

/**
 * Start the job scheduler.
 */
void DailyJob::start()
{
    std::lock_guard<std::mutex> lock(m_jobMtx);

    if (m_bRunning)
    {
        return;
    }

    LOG_INFO("Running job ", m_name, " at every ", m_hour, "th hour");
    m_bRunning = true;
    m_jobThread = std::make_unique<std::thread>(&DailyJob::runJobLoop_, this);
}

/**
 * Stop the job scheduler.
 */
void DailyJob::stop()
{
    {
        std::lock_guard<std::mutex> lock(m_jobMtx);
        if (!m_bRunning)
        {
            return;
        }
        LOG_INFO("Halting job ", m_name);
        m_bRunning = false;
    }

    m_jobCV.notify_one();

    if (m_jobThread && m_jobThread->joinable())
    {
        m_jobThread->join();
        m_jobThread.reset();
    }
}

/**
 * Run job at scheduled time, looping until told to stop.
 */
void DailyJob::runJobLoop_()
{
    LOG_DEBUG("Running job loop");
    while (m_bRunning)
    {
        // Sleep until next run
        auto nextRun = calculateNextRun_();
        LOG_DEBUG(m_name, " sleeping for ", static_cast<double>(std::chrono::duration_cast<std::chrono::seconds>(nextRun - std::chrono::system_clock::now()).count()) / 3600., " hours");
        {
            std::unique_lock<std::mutex> lock(m_jobMtx);
            if (m_jobCV.wait_until(lock, nextRun, [this] { return !m_bRunning; }))
            {
                break;
            }
        }

        // Run job and callback
        try
        {
            LOG_INFO(m_name, " beginning execution");
            auto startTime = std::chrono::steady_clock::now();

            m_job();

            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::minutes>(endTime - startTime);
            LOG_INFO(m_name, " finished execution! Time elapsed: ", duration.count(), " minutes.");

            if (m_jobCallback)
            {
                LOG_DEBUG("Executing callback for ", m_name);
                m_jobCallback();
            }
        }
        catch(std::exception const& e)
        {
            LOG_ERROR("Caught error in job ", m_name, ": ", e.what());
        }
        catch (...)
        {
            LOG_ERROR("Unknown error in job ", m_name);
        }
    }
}

/**
 * WARNING: Does not account for system time changes during sleep (e.g. DST).
 *
 * Calculate time until next run.
 */
[[nodiscard]] std::chrono::system_clock::time_point DailyJob::calculateNextRun_() const noexcept
{
    auto now = std::chrono::system_clock::now();
    auto currTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime;

    #ifdef _WIN32
        localtime_s(&localTime, &currTime);
    #else
        localtime_r(&currTime, &localTime);
    #endif

    std::tm scheduledTime = localTime;
    scheduledTime.tm_hour = m_hour;
    scheduledTime.tm_min = 0;
    scheduledTime.tm_sec = 0;

    auto scheduledRun = std::chrono::system_clock::from_time_t(std::mktime(&scheduledTime));

    if (now >= scheduledRun)
    {
        scheduledTime.tm_mday += 1;
        scheduledRun = std::chrono::system_clock::from_time_t(std::mktime(&scheduledTime));
    }

    return scheduledRun;
}
