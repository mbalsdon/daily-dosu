#ifndef __DAILY_JOB_H__
#define __DAILY_JOB_H__

#include <string>
#include <functional>

class DailyJob
{
public:
    DailyJob(int hour, std::string name, std::function<void()> job, std::function<void()> jobCallback);

    void start();

private:
    int m_hour;
    std::string m_name;
    std::function<void()> m_job;
    std::function<void()> m_jobCallback;
};

#endif /* __DAILY_JOB_H__ */
