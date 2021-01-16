#pragma once
#define FMT_UNICODE 0
#include <chrono>
#include <include/fmt/printf.h>
#include <include/fmt/compile.h>
#include <iostream>

namespace k2 {
    enum LogLevel {
        VERBOSE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL
    };

    extern LogLevel LOG_LEVEL;
}

namespace std {
    inline std::ostream& operator<<(std::ostream& os, const k2::LogLevel& level) {
        switch (level) {
            case k2::LogLevel::VERBOSE: return os << "VERBOSE";
            case k2::LogLevel::DEBUG: return os << "DEBUG";
            case k2::LogLevel::INFO: return os << "INFO";
            case k2::LogLevel::WARN: return os << "WARN";
            case k2::LogLevel::ERROR: return os << "ERROR";
            case k2::LogLevel::FATAL: return os << "FATAL";
            default: return os << "UNKNOWN";
        }
    }
}

//
// duration used in a few places to specify timeouts and such
//

using namespace std::chrono_literals;  // so that we can type "1ms"

namespace k2 {

typedef std::chrono::steady_clock Clock;
typedef Clock::duration Duration;
typedef std::chrono::time_point<Clock> TimePoint;

inline std::chrono::nanoseconds nsec(const Duration& dur) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(dur);
}
// timeduration of input timepoint since epoch(machine boot) in nanosec
inline uint64_t nsec_count(const TimePoint& timeP) {
    return nsec(timeP.time_since_epoch()).count();
}
// count of nanosec for steady_clock::now()
inline uint64_t now_nsec_count() {
    return nsec_count(Clock::now());
}
// timeduration of input timepoint since Unix epoch in nanosec
inline uint64_t nsec_count(const std::chrono::time_point<std::chrono::system_clock>& timeP) {
    return nsec(timeP.time_since_epoch()).count();
}
// count of nanosec for system_clock::now()
inline uint64_t sys_now_nsec_count() {
    return nsec_count(std::chrono::system_clock::now());
}
inline std::chrono::microseconds usec(const Duration& dur) {
    return std::chrono::duration_cast<std::chrono::microseconds>(dur);
}
inline std::chrono::milliseconds msec(const Duration& dur) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(dur);
}
inline std::chrono::seconds sec(const Duration& dur) {
    return std::chrono::duration_cast<std::chrono::seconds>(dur);
}

// this clock source should be used when you don't care just how precise you are with timing
// and you want to avoid a lot of calls to system's clock.
// It provides monotonically increasing, thread-local sequence of values and refreshes the
// system clock when asked.
struct CachedSteadyClock {
    typedef Duration duration;
    typedef Duration::rep rep;
    typedef Duration::period period;
    typedef TimePoint time_point;
    static const bool is_steady = true;

    static time_point now(bool refresh = false) noexcept;

   private:
    static thread_local TimePoint _now;
};

// Utility class to keep track of a deadline. Useful for nested requests
template <typename ClockT = Clock>
class Deadline {
   public:
    Deadline(typename ClockT::duration dur) : _deadline(ClockT::now() + dur) {}

    typename ClockT::duration getRemaining() const {
        auto now = ClockT::now();
        if (now >= _deadline) {
            return typename ClockT::duration(0);
        }
        return _deadline - now;
    }

    bool isOver() const {
        return ClockT::now() >= _deadline;
    }

   private:
    typename ClockT::time_point _deadline;
};  // class Deadline

inline const char* printTime(TimePoint tp) {
    // TODO we can use https://en.cppreference.com/w/cpp/chrono/system_clock/to_stream here, but it is a C++20 feature
    static thread_local char buffer[100];
    auto now = k2::usec(tp.time_since_epoch());
    auto microsec = now.count();
    auto millis = microsec / 1000;
    microsec -= millis * 1000;
    auto secs = millis / 1000;
    millis -= secs * 1000;
    auto mins = (secs / 60);
    secs -= (mins * 60);
    auto hours = (mins / 60);
    mins -= (hours * 60);
    auto days = (hours / 24);
    hours -= (days * 24);
    std::snprintf(buffer, sizeof(buffer), "%04ld:%02ld:%02ld:%02ld.%03ld.%03ld", (long)days, (long)hours, (long)mins, (long)secs, (long)millis, (long)microsec);
    return buffer;
}
}  // namespace k2

namespace std {
inline std::ostream& operator<<(std::ostream& os, const k2::Duration& dur) {
    os << k2::printTime(k2::TimePoint{} + dur);
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const k2::TimePoint& tp) {
    os << k2::printTime(tp);
    return os;
}
}  // namespace std

#define LOG_STREAM std::cout
#define LOG_FLUSH "\n"

#define DO_FLOG(level, module, fmt_str, ...)                                                                  \
    fmt::print(LOG_STREAM, FMT_STRING("{} - FMT {} - ({}) [{}: {}:{}] " fmt_str "\n"),                                    \
            "k2::Clock::now()", level, logger._module, __PRETTY_FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__);

#define LFLOG(level, logger, fmt_str, ...)                    \
    if (logger.isEnabledFor(level)) {                         \
        DO_FLOG(level, logger._module, fmt_str, ##__VA_ARGS__); \
    }

#define LFVERBOSE(logger, fmt_str, ...) LFLOG(k2::LogLevel::VERBOSE, logger, fmt_str, __VA_ARGS__);
#define LFDEBUG(logger, fmt_str, ...) LFLOG(k2::LogLevel::DEBUG, logger, fmt_str, ##__VA_ARGS__);
#define LFINFO(logger, fmt_str, ...) LFLOG(k2::LogLevel::INFO, logger, fmt_str, __VA_ARGS__);
#define LFWARN(logger, fmt_str, ...) LFLOG(k2::LogLevel::WARN, logger, fmt_str, __VA_ARGS__);
#define LFERROR(logger, fmt_str, ...) LFLOG(k2::LogLevel::ERROR, logger, fmt_str, __VA_ARGS__);
#define LFFATAL(logger, fmt_str, ...) LFLOG(k2::LogLevel::FATAL, logger, fmt_str, __VA_ARGS__);



#define DO_SLOG(level, module, msg)                                               \
    LOG_STREAM << "k2::Clock::now()" << " - STREAM " << level << " - (" << module \
               << ") [" << __PRETTY_FUNCTION__ << ": " << __FILE__ << ":" << __LINE__ << "] " << msg << LOG_FLUSH;

#define LSLOG(level, logger, msg)           \
    if (logger.isEnabledFor(level)) {       \
        DO_SLOG(level, logger._module, msg); \
    }

#define LSVERBOSE(logger, msg) LSLOG(k2::LogLevel::VERBOSE, logger, msg);
#define LSDEBUG(logger, msg) LSLOG(k2::LogLevel::DEBUG, logger, msg);
#define LSINFO(logger, msg) LSLOG(k2::LogLevel::INFO, logger, msg);
#define LSWARN(logger, msg) LSLOG(k2::LogLevel::WARN, logger, msg);
#define LSERROR(logger, msg) LSLOG(k2::LogLevel::ERROR, logger, msg);
#define LSFATAL(logger, msg) LSLOG(k2::LogLevel::FATAL, logger, msg);

namespace k2 {
class Logger {
   public:
    Logger(const char* moduleName) : _module(moduleName) {
    }
    bool isEnabledFor(k2::LogLevel level) {
        return level >= k2::LOG_LEVEL;
    }
    std::string _module;
};
}  // namespace k2
