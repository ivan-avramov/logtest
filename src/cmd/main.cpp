#include <string>

#include "logging/log.h"
k2::Logger logger("LMAIN");

// MAC
// no-log: 2ns/call
// log with disabled level: 3.35ns/call
// log "" to cout: 1321ns/call
// log "" to cout no flush: 838ns/call
// log "" to clog: 6338ns/call
// log "" to cerr: 8101ns/call
// log fmt "" to cout w/ flush: 817ns/call
// log fmt "" to cout w/o flush: 325ns/call
// log fmt "int - int str" to cout w/o flush: 368ns/call
// log fmt "int - int str" to cout w   flush: 849ns/call
// log stream "int - int str" to cout w   flush: 1795/call
// log stream "int - int str" to cout w/o flush: 1228/call

uint64_t dowork(uint64_t a, uint64_t i) {
    //LOG_DEBUG("got" << i << " -- " << a);
    //LSDEBUG(logger, a << " - " << i << " ");
    LFDEBUG(logger, "{} - {} {}", a, i, "");
    LFDEBUG(logger, "no");
    //LLOG_DEBUG("got" << i << a);
    //logger.D([&](auto& os) { os << "got:" << i << "--" << a; });
    //LDEBUG(logger, "got: {} -- {}", i, a);
    return (a + i) / a;
}
uint64_t bench(uint64_t a) {
    uint64_t result = a;
    for (uint64_t i = 0; i < a; ++i) {
        result += dowork(a, i);
    }
    return result;
}

int main(int, char** argv) {
    auto c = std::stoull(argv[1]);
    k2::LOG_LEVEL = (k2::LogLevel)std::stoi(argv[2]);
    LSINFO(logger, "main: " << c);
    LSDEBUG(logger, "deb");
    LSERROR(logger, "err");
    auto st = k2::Clock::now();
    auto result = bench(c);
    auto end = k2::Clock::now();
    auto el = end - st;
    LSINFO(logger, "res= " << result << ", elapsed=" << el << ", percall=" << ((double)k2::nsec(el).count()) / c);
    std::cerr << "res= " << result << ", elapsed=" << el << ", percall=" << ((double)k2::nsec(el).count()) / c << std::endl;
    std::cout << "res= " << result << ", elapsed=" << el << ", percall=" << ((double)k2::nsec(el).count()) / c << std::endl;
    return 0;
}
