#pragma once

#include <chrono>
#include <format>

using namespace std::chrono;
using namespace std::string_literals;

template<typename Time = nanoseconds, typename Clock = steady_clock>
class Timer
{
    bool _isRunning = false;
    Clock::time_point _startTime;
    Clock::time_point _endTime;

public:
    Timer()
    {
        //	Set local to US for messaging and number formatting
        std::locale::global(std::locale("en_US.UTF-8"));
    }

    ~Timer() = default;

    //	No copying
    Timer(const Timer &) = delete;
    Timer &operator=(const Timer &) = delete;
    //	Moving OK
    Timer(Timer &&) noexcept = default;
    Timer &operator=(Timer &&) = default;

    void start() { _startTime = steady_clock::now(); }
    void finish() { _endTime = steady_clock::now(); }

    Time duration() const
    {
        //  Initialize with end value
        auto temp{_endTime};

        //	Calls during timing run get temporary end date
        if (_isRunning) {
            //	When running, use current time
            temp = Clock::now();
        }

        return duration_cast<Time>(temp - _startTime);
    }
    //	Compute records per second
    double countPerSec(uint64_t rcds) const
    {
        // floating-point duration: no duration_cast needed
        const auto secs = round<microseconds>(duration()).count() / 1'000'000.0;

        return double(rcds) / (secs == 0 ? 1.0 : secs);
    }

    std::string recordsPerSec(const size_t cnt,
                              const std::string &label,
                              const std::string &item = "records") const
    {
        std::string ss;

        //  Apply formatting based on time
        ss = std::format("{}: {:L} {}/sec: {:L}. Elapsed: {}",
                         label,
                         cnt,
                         item,
                         static_cast<uint64_t>(countPerSec(cnt)),
                         elapsedTime());

        return ss;
    }

    std::string elapsedTime() const
    {
        //  Call internal function to get percision
        const auto dur = duration();

        //  C+20 feature to convert duration to timestamp
        const auto hms = std::chrono::hh_mm_ss(dur);
        std::string ss;

        //  Apply formatting based on time
        if (hms.hours().count() > 0) {
            //  Convert to milliseconds
            const auto secs = round<milliseconds>(hms.seconds() + hms.subseconds()).count()
                              / 1000.0;

            ss = std::format("{}:{:02}:{:06.3f}", hms.hours().count(), hms.minutes().count(), secs);
        } else if (hms.minutes().count() > 0) {
            //  Convert to milliseconds
            const auto secs = round<milliseconds>(hms.seconds() + hms.subseconds()).count()
                              / 1000.0;
            ss = std::format("{}:{:06.3f}", hms.minutes().count(), secs);
        } else if (hms.seconds().count() > 0) {
            //  Convert to milliseconds
            const auto secs = round<milliseconds>(hms.seconds() + hms.subseconds()).count()
                              / 1000.0;
            ss = std::format("{:.3f}", secs);
        } else if (hms.subseconds() > 1ms) {
            //  Convert to milliseconds
            const auto ticks = round<milliseconds>(hms.subseconds()).count();
            ss = std::format("{:L}ms", ticks);
        } else if (hms.subseconds() > 1us) {
            //  Convert to microseconds
            const auto ticks = round<microseconds>(hms.subseconds()).count();
            ss = std::format("{:L}\xB5s", ticks);
        } else
            ss = std::format("{:L}ns", hms.subseconds().count());

        return ss;
    }
};
