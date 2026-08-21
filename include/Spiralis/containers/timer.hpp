#ifndef ____SP_TIMER____
#define ____SP_TIMER____
#pragma once

#ifndef ____SPIRAL_CPP____
    #include "../setup/init.hpp"
#endif
#include <mach/mach_time.h>

namespace sp {

class HRTimer {
private:
ull _start=0;
ull _stop=0;
bool _is_running=false;
mach_timebase_info_data_t _info;
public:
HRTimer(){
    mach_timebase_info(&_info);
}

SP_FORCEINLINE void start(){
    _is_running = true;
    ull now_ticks = mach_absolute_time();
    _start = now_ticks * _info.numer / _info.denom;
}

SP_FORCEINLINE void stop(){
    ull now_ticks = mach_absolute_time();
    _stop = now_ticks * _info.numer / _info.denom;
    _is_running = false;
}

SP_FORCEINLINE ull getTimeNano(){
    if(_is_running) stop();
    return _stop - _start;
}

SP_FORCEINLINE ull getTimeMicro(){
    if(_is_running) stop();
    return (_stop - _start) / 1000ULL;
}

SP_FORCEINLINE ull getTimeMilli(){
    if(_is_running) stop();
    return (_stop - _start) / 1000000ULL;
}

SP_FORCEINLINE ull getTimeSec(){
    if(_is_running) stop();
    return (_stop - _start) / 1000000000ULL;
}

template <typename T>
static SP_FORCEINLINE void doNotOptimize(T&& value) {
    asm volatile("" : : "r,m"(value) : "memory");
}


};

} // namespace sp

#endif // ____SP_TIMER____