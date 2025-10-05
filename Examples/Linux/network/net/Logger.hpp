#pragma once
#include <cstdio>
#include <cstdarg>
#include <string>
#include <chrono>
#include <ctime>
#include <mutex>

namespace dist
{

    enum class LogLevel
    {
        INFO,
        WARN,
        ERROR,
        DEBUG
    };

    inline const char *levelStr(LogLevel l)
    {
        switch (l)
        {
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARN:
            return "WARN";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::DEBUG:
            return "DEBUG";
        }
        return "?";
    }

    class Logger
    {
    public:
        // Plug your project’s logging here if you have one; this is a simple stub.
        static Logger &instance()
        {
            static Logger L;
            return L;
        }

        void log(LogLevel level, const char *fmt, ...)
        {
            std::lock_guard<std::mutex> lk(mu_);
            auto now = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);
            std::tm tm{};
#if defined(_WIN32)
            localtime_s(&tm, &t);
#else
            localtime_r(&t, &tm);
#endif
            char ts[32];
            std::strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm);
            std::fprintf(stderr, "[%s] %-5s ", ts, levelStr(level));

            va_list ap;
            va_start(ap, fmt);
            std::vfprintf(stderr, fmt, ap);
            va_end(ap);

            std::fputc('\n', stderr);
        }

    private:
        std::mutex mu_;
    };

#define LOG_INFO(...) ::dist::Logger::instance().log(::dist::LogLevel::INFO, __VA_ARGS__)
#define LOG_WARN(...) ::dist::Logger::instance().log(::dist::LogLevel::WARN, __VA_ARGS__)
#define LOG_ERROR(...) ::dist::Logger::instance().log(::dist::LogLevel::ERROR, __VA_ARGS__)
#ifdef NDEBUG
#define LOG_DEBUG(...) ((void)0)
#else
#define LOG_DEBUG(...) ::dist::Logger::instance().log(::dist::LogLevel::DEBUG, __VA_ARGS__)
#endif

} // namespace dist
