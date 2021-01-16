#pragma once
#define FMT_UNICODE 0
#include <chrono>
#include <include/fmt/printf.h>
#include <include/fmt/compile.h>
#include <iostream>
#include <logging/json.h>

namespace k2 {
enum LogLevel {
    VERBOSE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

inline thread_local LogLevel LOG_LEVEL;
const char* const LogLevelNames[] = {
    "NOTSET",
    "VERBOSE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

}

template <>
struct fmt::formatter<k2::LogLevel> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(k2::LogLevel const& level, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), FMT_COMPILE("{}"), k2::LogLevelNames[level]);
    }
};

namespace std {
    inline std::ostream& operator<<(std::ostream& os, const k2::LogLevel& level) {
	return os << k2::LogLevelNames[level];
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

struct Timestamp_ts {
    uint16_t micros;
    uint16_t millis;
    uint8_t secs;
    uint8_t mins;
    uint8_t hours;
    uint16_t days;
};

inline Timestamp_ts toTimestamp_ts(const TimePoint& tp) {
    auto now = k2::usec(tp.time_since_epoch()).count();
    auto [quotient1, micros] = std::ldiv(now, 1000);
    auto [quotient2, millis] = std::ldiv(quotient1, 1000);
    auto [quotient3, secs] = std::ldiv(quotient2, 60);
    auto [quotient4, mins] = std::div((int)quotient3, 60); // the quotient is small enough to fit in an int now
    auto [days, hours] = std::div(quotient4, 24);
    return {(uint16_t)micros, (uint16_t)millis, (uint8_t)secs, (uint8_t)mins, (uint8_t)hours, (uint16_t)days};
}

inline const char* printTime(const TimePoint& tp) {
    auto ts = toTimestamp_ts(tp);
    static thread_local char buffer[24];
    fmt::format_to_n(buffer, sizeof(buffer), "{}", ts);
    return buffer;
}

}  // namespace k2

template <>
struct fmt::formatter<k2::Timestamp_ts> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(k2::Timestamp_ts const& ts, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), FMT_COMPILE("{:04}:{:02}:{:02}:{:02}.{:03}.{:03}"), ts.days, ts.hours, ts.mins, ts.secs, ts.millis, ts.micros);
    }
};

template <>
struct fmt::formatter<k2::Duration> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(k2::Duration const& dur, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", k2::TimePoint{} + dur);
    }
};

namespace std {
void inline to_json(nlohmann::json& j, const k2::Duration& obj) {
    j = nlohmann::json{{"duration", fmt::format("{}", obj)}};
}
}

template <>
struct fmt::formatter<k2::TimePoint> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(k2::TimePoint const& tp, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", k2::toTimestamp_ts(tp));
    }
};

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
#define LOG_FLUSH std::flush

#define DO_FLOG(level, module, fmt_str, ...)                                                            \
    fmt::print(LOG_STREAM, FMT_STRING("{} - FMT {} - ({}) [{}: {}:{}] " fmt_str "\n"),                  \
    "k2::Clock::now()", level, logger._module, __PRETTY_FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__);

#define LFLOG(level, logger, fmt_str, ...)                    \
    if (logger.isEnabledFor(level)) {                         \
        DO_FLOG(level, logger._module, fmt_str, ##__VA_ARGS__); \
        LOG_STREAM << LOG_FLUSH; \
    }

#define LFDEBUG(logger, fmt_str, ...) LFLOG(k2::LogLevel::DEBUG, logger, fmt_str, ##__VA_ARGS__);
#define LFINFO(logger, fmt_str, ...) LFLOG(k2::LogLevel::INFO, logger, fmt_str, __VA_ARGS__);



#define DO_SLOG(level, module, msg)                                               \
    LOG_STREAM << "k2::Clock::now()" << " - STREAM " << level << " - (" << module \
               << ") [" << __PRETTY_FUNCTION__ << ": " << __FILE__ << ":" << __LINE__ << "] " << msg << "\n";

#define LSLOG(level, logger, msg)           \
    if (logger.isEnabledFor(level)) {       \
        DO_SLOG(level, logger._module, msg); \
        LOG_STREAM << LOG_FLUSH; \
    }

#define LSDEBUG(logger, msg) LSLOG(k2::LogLevel::DEBUG, logger, msg);
#define LSINFO(logger, msg) LSLOG(k2::LogLevel::INFO, logger, msg);

namespace k2 {
class Logger {
   public:
    Logger(const char* moduleName) : _module(moduleName) {
    }
    bool isEnabledFor(k2::LogLevel level) {
	if (level > _override) {
           return true;
	}
        return level >= k2::LOG_LEVEL;
    }
    LogLevel _override;
    std::string _module;
};
}  // namespace k2

