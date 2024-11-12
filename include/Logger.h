#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <sstream>

struct SourceLocation
{
    const char* file;
    const char* function;
    unsigned int line;

    SourceLocation(const char* f, const char* fn, unsigned int l)
        : file(f)
        , function(fn)
        , line(l)
    {}
};

class Logger
{
public:
    enum class Level
    {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };

    static Logger& getInstance()
    {
        static Logger instance;
        return instance;
    }

    void setLogLevel(int level)
    {
        m_logLevel = intToLevel(level);
    }

    template<typename... Args>
    void log(Level level, const SourceLocation& location, Args&&... args)
    {
        if (level < m_logLevel)
        {
            return;
        }
        // Get stuff for log prefix
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        auto threadID = std::this_thread::get_id();

        // Append prefix
        std::stringstream ss;
        ss << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0') << std::setw(3) << nowMs.count() << "] ";
        ss << "[" << levelToString(level) << "] ";
        if (m_logLevel <= Level::DEBUG)
        {
            ss << "[" << threadID << "] ";
            ss << "[" << location.file << ":" << location.line << "] ";
            ss << "[" << location.function << "] ";
        }

        // Append messages
        (ss << ... << std::forward<Args>(args)) << std::endl;

        // Send to stdout (thread-safe)
        std::cout << ss.str();
    }

private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static const char* levelToString(Level level)
    {
        switch (level)
        {
        case Level::DEBUG: return "DEBUG";
        case Level::INFO: return "INFO";
        case Level::WARNING: return "WARNING";
        case Level::ERROR: return "ERROR";
        default: return "UNKNOWN";
        }
    }

    static const Level intToLevel(int l)
    {
        if (l == 0) return Level::DEBUG;
        if (l == 1) return Level::INFO;
        if (l == 2) return Level::WARNING;
        if (l == 3) return Level::ERROR;

        return Level::INFO;
    }

    Level m_logLevel = Level::INFO;
};

#define CURRENT_LOCATION SourceLocation(__FILE__, __func__, __LINE__)
#define LOG_DEBUG(...) Logger::getInstance().log(Logger::Level::DEBUG, CURRENT_LOCATION, __VA_ARGS__)
#define LOG_INFO(...) Logger::getInstance().log(Logger::Level::INFO, CURRENT_LOCATION, __VA_ARGS__)
#define LOG_WARN(...) Logger::getInstance().log(Logger::Level::WARNING, CURRENT_LOCATION, __VA_ARGS__)
#define LOG_ERROR(...) Logger::getInstance().log(Logger::Level::ERROR, CURRENT_LOCATION, __VA_ARGS__)

#endif /* __LOGGER_H__ */
