#include <DailyJob.h>

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

    return std::chrono::minutes(minutesUntil);
}
} /* namespace Detail */

/**
 * TODO: desc
 */
DailyJob::DailyJob(int hour, std::string name, void (*job)(), std::function<void()> jobCallback)
{
    if (!job)
    {
        std::string reason = "DailyJob::DailyJob - job cannot be nullptr!";
        throw std::runtime_error(reason);
    }

    if ((hour < 0) || (hour > 23))
    {
        hour = hour % 24;
        std::cout << "DailyJob::DailyJob - hour is out of bounds; normalizing to " << hour << std::endl;
    }

    m_hour = hour;
    m_name = name;
    m_job = job;
    m_jobCallback = jobCallback;
}

/**
 * TODO: desc
 */
void DailyJob::start()
{
    std::cout << "DailyJob::start - running job \"" << m_name << "\" at every " << m_hour << "th hour..." << std::endl;
    std::thread(
        [this]()
        {
            while (true)
            {
                // TODO: remove
                // m_job();
                // m_jobCallback();

                std::chrono::minutes minutesUntilNextRun = Detail::minutesUntil(m_hour);
                double hoursUntilNextRun = std::chrono::duration<double, std::ratio<60>>(minutesUntilNextRun).count() / 60.0;
                std::cout << "DailyJob::" << m_name << " - sleeping for " << hoursUntilNextRun << " hours before running again..." << std::endl;
                std::this_thread::sleep_for(minutesUntilNextRun);

                std::cout << "DailyJob::" << m_name << " - beginning execution..." << std::endl;
                m_job();
                if (m_jobCallback)
                {
                    std::cout << "DailyJob::" << m_name << " - executing job callback..." << std::endl;
                    m_jobCallback();
                }
            }
        }).detach();
}
