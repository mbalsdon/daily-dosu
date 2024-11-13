#include "DailyJob.h"
#include "Logger.h"

#include <iostream>
#include <chrono>
#include <thread>

namespace Detail
{
std::chrono::seconds secondsUntil(int hour)
{
    auto now = std::chrono::system_clock::now();
    auto ttNow = std::chrono::system_clock::to_time_t(now);
    auto localTime = *std::localtime(&ttNow);

    int currHour = localTime.tm_hour;
    int currMinute = localTime.tm_min;
    int currSecond = localTime.tm_sec;

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

    int secondsUntil = (minutesUntil * 60) - currSecond;
    if (secondsUntil < 0)
    {
        secondsUntil += 86400; // 24 hours
    }

    return std::chrono::seconds(secondsUntil);
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
                std::chrono::seconds secondsUntilNextRun = Detail::secondsUntil(m_hour);
                double hoursUntilNextRun = static_cast<double>(secondsUntilNextRun.count()) / 3600.;
                LOG_DEBUG(m_name, " sleeping for ", hoursUntilNextRun, " hours before running again...");
                std::this_thread::sleep_for(secondsUntilNextRun);

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

                // FIXME: Time calculation is only precise to seconds, so if the job takes less than 1 second it will run again immediately.
                //        Just sleep here for now to avoid that.
                LOG_DEBUG("Job complete - sleeping for one second...");
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }).detach();
}
