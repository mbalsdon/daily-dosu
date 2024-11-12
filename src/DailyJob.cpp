#include "DailyJob.h"
#include "Logger.h"

#include <iostream>
#include <chrono>
#include <thread>

namespace Detail
{
std::chrono::minutes minutesUntil(int hour)
{
    auto now = std::chrono::system_clock::now();
    auto ttNow = std::chrono::system_clock::to_time_t(now);
    auto localTime = *std::localtime(&ttNow);

    int currHour = localTime.tm_hour;
    int currMinute = localTime.tm_min;

    int hoursUntil = hour - currHour;
    if (hoursUntil < 0)
    {
        hoursUntil += 24;
    }

    int minutesUntil = (hoursUntil * 60) - currMinute;
    if (minutesUntil < 0)
    {
        minutesUntil += 1440; // 24 hours
    }

    return std::chrono::minutes(minutesUntil);
}
} /* namespace Detail */

/**
 * DailyJob constructor.
 */
DailyJob::DailyJob(int hour, std::string name, std::function<void()> job, std::function<void()> jobCallback)
    : m_name(name)
    , m_jobCallback(jobCallback)
{
    LOG_DEBUG("Creating DailyJob instance for ", name, ", running at hour ", hour);

    if (!job)
    {
        std::string reason = "DailyJob::DailyJob - job cannot be nullptr!";
        throw std::invalid_argument(reason);
    }

    if ((hour < 0) || (hour > 23))
    {
        hour = hour % 24;
        LOG_WARN("Hour is out of bounds; normalizing to ", hour, "...");
    }

    m_hour = hour;
    m_job = job;
}

/**
 * Starts the job scheduler.
 * Runs in a detached thread every day at the specified hour (system time).
 * If a callback is specified, it will run after each job completion.
 */
void DailyJob::start()
{
    LOG_INFO("Running job \"", m_name, "\" at every ", m_hour, "th hour...");
    std::thread(
        [this]()
        {
            while (true)
            {
                // Sleep until next scheduled run
                std::chrono::minutes minutesUntilNextRun = Detail::minutesUntil(m_hour);
                double hoursUntilNextRun = std::chrono::duration<double, std::ratio<60>>(minutesUntilNextRun).count() / 60.;
                LOG_DEBUG(m_name, " sleeping for ", hoursUntilNextRun, " hours before running again...");
                std::this_thread::sleep_for(minutesUntilNextRun);

                // Run job, reporting time taken (not high precision)
                LOG_INFO(m_name, " beginning execution...");
                auto startTime = std::chrono::system_clock::now();
                m_job();
                auto endTime = std::chrono::system_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::minutes>(endTime - startTime);
                LOG_INFO(m_name, " finished execution! Time elapsed: ", duration.count(), " minutes.");

                // Execute callback if it exists
                if (m_jobCallback)
                {
                    LOG_DEBUG("Executing callback for ", m_name, "...");
                    m_jobCallback();
                }
            }
        }).detach();
}
