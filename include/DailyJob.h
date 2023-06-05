#ifndef __DAILY_JOB_H__
#define __DAILY_JOB_H__

#include <cstdint>
#include <string>
#include <functional>
#include <memory>

class DailyJob
{
public:
    DailyJob(int hour, std::string name, void (*job)(), std::function<void()> jobCallback);

    void start();

private:
    int m_hour;
    std::string m_name;
    void (*m_job)();
    std::function<void()> m_jobCallback;
};

#endif /* __DAILY_JOB_H__ */
