#ifndef REPLICATED_SPLINTERDB_TIMER_H
#define REPLICATED_SPLINTERDB_TIMER_H

#include <chrono>
#include <iomanip>

namespace replicated_splinterdb {

class Timer {
  public:
    Timer() : duration_ms(0) { reset(); }
    Timer(size_t _duration_ms) : duration_ms(_duration_ms) { reset(); }
    inline bool timeout() { return timeover(); }
    bool timeover() {
        auto cur = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed = cur - start;
        if (duration_ms < static_cast<size_t>(elapsed.count()) * 1000)
            return true;
        return false;
    }
    uint64_t getTimeSec() {
        auto cur = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed = cur - start;
        return (uint64_t)(elapsed.count());
    }
    uint64_t getTimeMs() {
        auto cur = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed = cur - start;
        return (uint64_t)(elapsed.count() * 1000);
    }
    uint64_t getTimeUs() {
        auto cur = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed = cur - start;
        return (uint64_t)(elapsed.count() * 1000000);
    }
    void reset() { start = std::chrono::system_clock::now(); }
    void resetSec(size_t _duration_sec) {
        duration_ms = _duration_sec * 1000;
        reset();
    }
    void resetMs(size_t _duration_ms) {
        duration_ms = _duration_ms;
        reset();
    }

  private:
    std::chrono::time_point<std::chrono::system_clock> start;
    size_t duration_ms;
};

static std::string usToString(uint64_t us) {
    std::stringstream ss;
    if (us < 1000) {
        // us
        ss << std::fixed << std::setprecision(0) << us << " us";
    } else if (us < 1000000) {
        // ms
        double tmp = static_cast<double>(us) / 1000.0;
        ss << std::fixed << std::setprecision(1) << tmp << " ms";
    } else if (us < (uint64_t)600 * 1000000) {
        // second: 1 s -- 600 s (10 mins)
        double tmp = static_cast<double>(us) / 1000000.0;
        ss << std::fixed << std::setprecision(1) << tmp << " s";
    } else {
        // minute
        double tmp = static_cast<double>(us) / 60.0 / 1000000.0;
        ss << std::fixed << std::setprecision(0) << tmp << " m";
    }
    return ss.str();
}

[[maybe_unused]] static void sleep_ms(size_t ms,
                                      const std::string& msg = std::string()) {
    if (!msg.empty()) printf("%s (%zu ms)\n", msg.c_str(), ms);
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

};  // namespace replicated_splinterdb

#endif  // REPLICATED_SPLINTERDB_TIMER_H