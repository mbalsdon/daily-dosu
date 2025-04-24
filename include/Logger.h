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

    SourceLocation(const char* f, const char* fn, unsigned int const& l)
        : file(f)
        , function(fn)
        , line(l)
    {}
};

/**
 * Simple logger class.
 */
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

    [[nodiscard]] static Logger& getInstance() noexcept
    {
        static Logger instance;
        return instance;
    }

    void setLogLevel(int const& level)
    {
        m_logLevel = intToLevel_(level);
    }

    void setLogLevel(Level const& level)
    {
        m_logLevel = level;
    }

    void setLogColors(bool const& enable)
    {
        m_logAnsiColors = enable;
    }

    template<typename... Args>
    void log(Level const& level, SourceLocation const& location, Args&&... args) const
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

        std::stringstream ss;

        // Add color
        if (m_logAnsiColors)
        {
            ss << "\033[" + levelToColor_(level) << "m";
        }

        // Append prefix
        ss << "[" << levelToString_(level) << "] ";
        if (m_logLevel <= Level::DEBUG)
        {
            ss << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0') << std::setw(3) << nowMs.count() << "] ";
            ss << "[" << threadID << "] ";
            ss << "[" << location.file << ":" << location.line << "] ";
            ss << "[" << location.function << "] ";
        }

        // Append messages
        (ss << ... << std::forward<Args>(args)) << std::endl;

        // Reset color
        if (m_logAnsiColors)
        {
            ss << "\033[0m";
        }

        // Send to stdout (thread-safe)
        std::cout << ss.str() << std::flush;
    }

private:
    Logger() = default;
    Logger(Logger const&) = delete;
    Logger& operator=(Logger const&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    [[nodiscard]] static const char* levelToString_(Level const& level) noexcept
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

    [[nodiscard]] static Level intToLevel_(int const& l) noexcept
    {
        switch (l)
        {
        case 0: return Level::DEBUG;
        case 1: return Level::INFO;
        case 2: return Level::WARNING;
        case 3: return Level::ERROR;
        default: return Level::INFO;
        }
    }

    [[nodiscard]] static std::string levelToColor_(Level const& level) noexcept
    {
        switch (level)
        {
        case Level::DEBUG: return "90";
        case Level::INFO: return "37";
        case Level::WARNING: return "33";
        case Level::ERROR: return "31";
        default: return 0;
        }
    }

    Level m_logLevel = Level::INFO;
    bool m_logAnsiColors = false;
};

#define CURRENT_LOCATION SourceLocation(__FILE__, __func__, __LINE__)
#define LOG_DEBUG(...) Logger::getInstance().log(Logger::Level::DEBUG, CURRENT_LOCATION, __VA_ARGS__)
#define LOG_INFO(...) Logger::getInstance().log(Logger::Level::INFO, CURRENT_LOCATION, __VA_ARGS__)
#define LOG_WARN(...) Logger::getInstance().log(Logger::Level::WARNING, CURRENT_LOCATION, __VA_ARGS__)
#define LOG_ERROR(...) Logger::getInstance().log(Logger::Level::ERROR, CURRENT_LOCATION, __VA_ARGS__)

#define LOG_ERROR_THROW(cond, ...) \
    if (!(cond)) { \
        std::ostringstream errorMessage; \
        errorMessage << "Condition '" << #cond << "' failed. "; \
        Logger::getInstance().log(Logger::Level::ERROR, CURRENT_LOCATION, __VA_ARGS__); \
        throw std::runtime_error(errorMessage.str()); \
    }

#endif /* __LOGGER_H__ */
