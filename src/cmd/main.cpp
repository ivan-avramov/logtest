#include <string>
#include <pthread.h>

#include <logging/log.h>
#include <logging/json.h>

inline thread_local k2::Logger logger("LMAIN");

struct Data {
    std::string str1;
    std::string str2;
    uint64_t int1;
    uint64_t int2;
    inline friend std::ostream& operator<<(std::ostream& os, const Data& d) {
        return os << "{\"str1\": " << d.str1 << ", \"str2\": " << d.str2 << ", \"int1\": " << d.int1 << ", \"int2\": " << d.int2 << "}";
    }
    inline friend void to_json(nlohmann::json& j, const Data& d) {
        j = nlohmann::json{
            {"str1", d.str1},
            {"str2", d.str2},
            {"int1", d.int1},
            {"int2", d.int2}
        };
    }
};

template <>
struct fmt::formatter<Data> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(Data const& d, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), FMT_COMPILE("{{\"str1\":{}, \"str2\":{}, \"int1\":{}, \"int2\":{}}}"),
            d.str1, d.str2, d.int1, d.int2);
    }
};

inline void pin(int cpu) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if (0 != pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset)) {
        throw std::runtime_error("Unable to set affinity");
    }
}

uint64_t dowork(uint64_t a, uint64_t i, Data& d) {
    //LSDEBUG(logger, "obj=" << d);
    LFDEBUG(logger, "obj={}", d);
    return (a + i) / a;
}

uint64_t bench(uint64_t a, std::string str) {
    uint64_t result = a;
    Data d;
    d.str1 = str + "ab";
    d.str2 = str + "cd";
    d.int1 = 18446744073709551615ull - a;
    d.int2 = 18446744073709551615ull - a + 1;
    for (uint64_t i = 0; i < a; ++i) {
        result += dowork(a, i, d);
    }
    return result;
}

int main(int, char** argv) {
    pin(2);
    auto c = std::stoull(argv[1]);
    k2::LOG_LEVEL = (k2::LogLevel)std::stoi(argv[2]);
    logger._override = (k2::LogLevel)std::stoi(argv[2]);
    LSINFO(logger, "main: " << c);
    auto st = k2::Clock::now();
    auto result = bench(c, std::string(30, argv[1][0]));
    auto end = k2::Clock::now();
    auto el = end - st;
    std::cerr << "res= " << result << ", elapsed=" << el << ", percall=" << ((double)k2::nsec(el).count()) / c << std::endl;
    std::cout << "res= " << result << ", elapsed=" << el << ", percall=" << ((double)k2::nsec(el).count()) / c << std::endl;
    return 0;
}
