#ifndef ____SP_HIGHRESTIMER____
#define ____SP_HIGHRESTIMER____
#pragma once

#include "../setup/init.hpp"
#include <mach/mach_time.h>

namespace sp {

class HRTimer {
private:
    ull _start_ticks = 0;
    ull _stop_ticks = 0;
    bool _is_running = false;
    mach_timebase_info_data_t _info;

    SP_FORCEINLINE ull elapsed_ticks() const {
        ull end = _is_running ? mach_absolute_time() : _stop_ticks;
        return end - _start_ticks;
    }

public:
    HRTimer() {
        mach_timebase_info(&_info);
    }

    SP_FORCEINLINE void start() {
        _start_ticks = mach_absolute_time();
        _is_running = true;
    }

    SP_FORCEINLINE void stop() {
        _stop_ticks = mach_absolute_time();
        _is_running = false;
    }

    SP_FORCEINLINE ull getTimeNano() const {
        return (elapsed_ticks() * _info.numer) / _info.denom;
    }

    SP_FORCEINLINE ull getTimeMicro() const {
        return getTimeNano() / 1000ULL;
    }

    SP_FORCEINLINE ull getTimeMilli() const {
        return getTimeNano() / 1000000ULL;
    }

    SP_FORCEINLINE ull getTimeSec() const {
        return getTimeNano() / 1000000000ULL;
    }

    template <typename T>
    static SP_FORCEINLINE void doNotOptimize(T&& value) {
        asm volatile("" : : "r,m"(value) : "memory");
    }
};

} // namespace sp

#endif // ____SP_HIGHRESTIMER____