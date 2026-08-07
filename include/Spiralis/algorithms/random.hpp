#ifndef ____SP_RANDOM____
#define ____SP_RANDOM____
#pragma once
#include "../setup/init.hpp"
#include "../numeric/int128.hpp"
#include "../numeric/bit_manip.hpp"
#include <stdlib.h>
#if defined(__arm64__) || defined(__aarch64__)
    #include <sys/random.h>
#endif
namespace sp{

SP_FORCEINLINE bool get_rand_set_bits64(ull* result) {
#if defined(__arm64__) || defined(__aarch64__)
    return getentropy(result, sizeof(ull)) == 0;
#elif defined(__x86_64__) || defined(_M_X64)
    unsigned char success = 0;
    asm volatile(
        "rdrand %0; setc %1"
        : "=r"(*result), "=qm"(success)
        :
        : "cc"
    );
    return success;
#else
    #error "Unsupported architecture! No direct hardware assembly path available for module <Spiral/algorithms/random>."
#endif
}

SP_FORCEINLINE bool get_psrand_set_bits64(ull* result){
#if defined(__APPLE__)
    arc4random_buf(result, sizeof(ull));
    return true;
#elif defined(__x86_64__) || defined(_M_X64)
    unsigned char success = 0;
    asm volatile(
        "rdrand %0; setc %1"
        : "=r"(*result), "=qm"(success)
        :
        : "cc"
    );
    return success;
#else
    #error "Unsupported architecture! No direct hardware assembly path available for module <Spiral/algorithms/random>."
#endif
}

SP_FORCEINLINE ull get_rand64(){
    ull result;
    while(!get_rand_set_bits64(&result)) continue;
    return result;
}

SP_FORCEINLINE ull get_psrand64(){
    ull result;
    while(!get_psrand_set_bits64(&result)) continue;
    return result;
}



template <typename T>
class uniform_distribution{
private:
    T m_min;
    T m_max;
public:
    uniform_distribution(T low, T high) : m_min(low), m_max(high) {}
    template <typename Engine>
    T operator()(Engine& eng){
        ull range = static_cast<ull>(m_max-m_min+1);
        ull x = eng();
        unsigned __int128 m = static_cast<unsigned __int128>(x) * static_cast<unsigned __int128>(range);
        ull l = static_cast<ull>(m);

        if(l < range){
            ull t = -range % range;
            while (l < t) {
                x = eng();
                m = static_cast<unsigned __int128>(x) * static_cast<unsigned __int128>(range);
                l = static_cast<ull>(m);
            }
        }
        return m_min + static_cast<T>(m >> 64);
    }
};

class random_engine{
private:
    ull _state[4];
public:
    using result_type = ull;
    random_engine(ull seed=get_rand64()){
        ull z = seed;
        for (int i = 0; i < 4; ++i) {
            z += 0x9e3779b97f4a7c15;
            ull val = z;
            val = (val ^ (val >> 30)) * 0xbf58476d1ce4e5b9;
            val = (val ^ (val >> 27)) * 0x94d049bb133111eb;
            _state[i] = val ^ (val >> 31);
        }
    }
    static constexpr result_type min() { return 0; }
    static constexpr result_type max() { return (ull)(-1); }
    result_type operator()() {
        const ull result = rotl(_state[1] * 5, 7) * 9;

        const ull t = _state[1] << 17;

        _state[2] ^= _state[0];
        _state[3] ^= _state[1];
        _state[1] ^= _state[2];
        _state[0] ^= _state[3];

        _state[2] ^= t;

        _state[3] = rotl(_state[3], 45);

        return result;
    }
};
};
#endif