#ifndef ____SP_BIT_MANIP____
#define ____SP_BIT_MANIP____
#pragma once
#include "../setup/init.hpp"
#include "../core/type_traits.hpp"
namespace sp{

template <typename T>
SP_FORCEINLINE SP_CONST T rotl(const T x, int k){ return (x << k) | (x >> ((sizeof(T)*8) - k));}

template <typename T>
SP_FORCEINLINE SP_CONST bool isSet(T n, int k) { return (n & (1ULL << k)) != 0; }

template <typename T>
SP_FORCEINLINE SP_CONST T setBit(T n, int k) { return n | (1ULL << k); }

template <typename T>
SP_FORCEINLINE SP_CONST T clearBit(T n, int k) { return n & ~(1ULL << k); }

template <typename T>
SP_FORCEINLINE SP_CONST T toggleBit(T n, int k) { return n ^ (1ULL << k); }

template <typename T>
SP_FORCEINLINE SP_CONST bool isOdd(T n) { return n & 1; }

template <typename T>
SP_FORCEINLINE SP_CONST T clearFirstSet(T n) { return n & (n-1); }

template <typename T>
SP_FORCEINLINE T isolateFirstSet(T n) { return n | (1ULL << find_first_set(n)); }

template <typename T>
SP_FORCEINLINE SP_CONST T setFirstUnset(T n) { return n | (n+1); }

template <typename T>
SP_FORCEINLINE SP_CONST T isolateFirstUnset(T n) { return ~n & (n+1); }

template <typename T>
SP_FORCEINLINE SP_CONST T propagateRightmostSetBit(T n) { return n | (n - 1); }

template <typename T>
SP_FORCEINLINE T getParity(T n) { return popcount(n)&1; }

template <typename T>
SP_FORCEINLINE bool isPow2(T n) { return popcount(n)==1; }

template <typename T>
SP_FORCEINLINE SP_CONST T modPowerOfTwo(T n, int k) { return n & ((1ULL << k) - 1); }

template <typename T>
SP_FORCEINLINE SP_CONST T createMask(T k) { return (1ULL << k) - 1; }

template <typename T>
SP_FORCEINLINE SP_CONST T clearLowKBits(T n, int k) { return n & ~((1ULL << k) - 1); }

template <typename T, typename U>
SP_FORCEINLINE void xor_swap(T& a, U& b) {
    a ^= b;
    b ^= a;
    a ^= b;
}

template <typename T>
SP_FORCEINLINE SP_CONST int bitwiseAbs(T n) {
    const T mask = n >> ((sizeof(T)*8) - 1);
    return (n + mask) ^ mask;
}

template <typename T, typename U>
SP_FORCEINLINE SP_CONST T bitwiseMin(T x, U y) { return y ^ ((x ^ y) & -(x < y)); }
template <typename T, typename U>
SP_FORCEINLINE SP_CONST T bitwiseMax(T x, U y) { return x ^ ((x ^ y) & -(x < y)); }

template <typename T, typename U>
SP_FORCEINLINE SP_CONST bool haveOppositeSigns(T x, U y) { return (x ^ y) < 0; }

SP_FORCEINLINE SP_CONST char toLower(char c) { return c | ' '; }
SP_FORCEINLINE SP_CONST char toUpper(char c) { return c & '_'; }
SP_FORCEINLINE SP_CONST char toggleCase(char c) { return c ^ ' '; }


template <typename T>
SP_FORCEINLINE T reverseBits(T n) {
    using U = spt::make_unsigned_t<T>;
    U val = (U)(n);
    if constexpr(sizeof(T) == 8){
        val = ((val & 0x00000000FFFFFFFFULL) << 32) | ((val & 0xFFFFFFFF00000000ULL) >> 32);
        val = ((val & 0x0000FFFF0000FFFFULL) << 16) | ((val & 0xFFFF0000FFFF0000ULL) >> 16);
        val = ((val & 0x00FF00FF00FF00FFULL) << 8)  | ((val & 0xFF00FF00FF00FF00ULL) >> 8);
        val = ((val & 0x0F0F0F0F0F0F0F0FULL) << 4)  | ((val & 0xF0F0F0F0F0F0F0F0ULL) >> 4);
        val = ((val & 0x3333333333333333ULL) << 2)  | ((val & 0xCCCCCCCCCCCCCCCCULL) >> 2);
        val = ((val & 0x5555555555555555ULL) << 1)  | ((val & 0xAAAAAAAAAAAAAAAAULL) >> 1);
    } 
    else if constexpr(sizeof(T) == 4){
        val = ((val & 0x0000FFFF) << 16) | ((val & 0xFFFF0000) >> 16);
        val = ((val & 0x00FF00FF) << 8)  | ((val & 0xFF00FF00) >> 8);
        val = ((val & 0x0F0F0F0F) << 4)  | ((val & 0xF0F0F0F0) >> 4);
        val = ((val & 0x33333333) << 2)  | ((val & 0xCCCCCCCC) >> 2);
        val = ((val & 0x55555555) << 1)  | ((val & 0xAAAAAAAA) >> 1);
    } 
    else if constexpr(sizeof(T) == 2){
        val = ((val & 0x00FF) << 8) | ((val & 0xFF00) >> 8);
        val = ((val & 0x0F0F) << 4) | ((val & 0xF0F0) >> 4);
        val = ((val & 0x3333) << 2) | ((val & 0xCCCC) >> 2);
        val = ((val & 0x5555) << 1) | ((val & 0xAAAA) >> 1);
    } 
    else if constexpr(sizeof(T) == 1){
        val = ((val & 0x0F) << 4) | ((val & 0xF0) >> 4);
        val = ((val & 0x33) << 2) | ((val & 0xCC) >> 2);
        val = ((val & 0x55) << 1) | ((val & 0xAA) >> 1);
    }

    return (T)(val);
}
template <typename T>
SP_NODISCARD SP_FORCEINLINE SP_PURE SP_COLD T next_pow2(T min) {
    SP_IF_NOT_EXPECT(min <= 1) return 1;
    T val = min;
    #if defined(__GNUC__) || defined(__clang__)
        constexpr int total_bits = sizeof(T) * 8;

        if constexpr (sizeof(T) <= sizeof(unsigned int)) {
            return (T)1 << (total_bits - __builtin_clz((unsigned int)val));
        } else if constexpr (sizeof(T) <= sizeof(unsigned long)) {
            return (T)(1 << (total_bits - __builtin_clzl((unsigned long)val)));
        } else {
            return (T)1 << (total_bits - __builtin_clzll((ull)val));
        }

    #elif defined(_MSC_VER)
        unsigned long index;
        if constexpr (sizeof(T) > 4) {
            if (_BitScanReverse64(&index, (unsigned __int64)val)) return (T)1 << (index + 1);
        } else {
            if (_BitScanReverse(&index, (unsigned long)val)) return (T)1 << (index + 1);
        }
        return 1;

    #else
        val |= val >> 1;
        val |= val >> 2;
        val |= val >> 4;
        if constexpr(sizeof(T) > 1) val |= val >> 8;
        if constexpr(sizeof(T) > 2) val |= val >> 16;
        if constexpr(sizeof(T) > 4) val |= val >> 32;
        return val + 1;
    #endif
}

    template <typename T>
    SP_FORCEINLINE SP_PURE ull popcount(T val){
    #if ___SP_DETECTED_COMPILER___ == clang
        if constexpr(spt::is_same_v<T, ull>||spt::is_same_v<T, ll>) return __builtin_popcountll(val);
        if constexpr(spt::is_same_v<T, unsigned long>||spt::is_same_v<T, long>) return __builtin_popcountl(val);
        return __builtin_popcount(val);
    #elif ___SP_DETECTED_COMPILER___ == msvc
        if constexpr(spt::is_same_v<T, ull>||spt::is_same_v<T, ll>) return __popcnt64(val);
        if constexpr(spt::is_same_v<T, unsigned long>||spt::is_same_v<T, long>) return __popcnt(val);
        return __popcnt(val);
    #endif
    }

    template <typename T>
    SP_FORCEINLINE SP_PURE T reverse_bits(T val){
    #if ___SP_DETECTED_COMPILER___ == clang
        if constexpr(spt::is_same_v<T, ull>||spt::is_same_v<T, ll>) return __builtin_bswap64(val);
        return __builtin_bswap32(val);
    #elif ___SP_DETECTED_COMPILER___ == msvc
        if constexpr(spt::is_same_v<T, ull>||spt::is_same_v<T, ll>) return _byteswap_uint64(val);
        return _byteswap_uint32(val);
    #endif
    }

    template <typename T>
    SP_FORCEINLINE SP_PURE ull leading_zeros(T val){
    #if ___SP_DETECTED_COMPILER___ == clang
        if constexpr(spt::is_same_v<T,ull>||spt::is_same_v<T,ll>) return __builtin_clzll(val);
        if constexpr(spt::is_same_v<T,unsigned long>||spt::is_same_v<T,long>) return __builtin_clzl(val);
        return __builtin_clz(val);
    #elif ___SP_DETECTED_COMPILER___ == msvc
        if constexpr(spt::is_same_v<T,ull>||spt::is_same_v<T,ll>) return _BitScanReverse(&index, val);
        if constexpr(spt::is_same_v<T,unsigned long>||spt::is_same_v<T,long>) return _BitScanReverse(&index, val);
        return _BitScanReverse(&index, val);
    #endif
    }

    template <typename T>
    SP_FORCEINLINE SP_PURE ull trailing_zeros(T val){
    #if ___SP_DETECTED_COMPILER___ == clang
        if constexpr(spt::is_same_v<T, ull>||spt::is_same_v<T,ll>) return __builtin_ctzll(val);
        if constexpr(spt::is_same_v<T, unsigned long>||spt::is_same_v<T, long>) return __builtin_ctzl(val);
        return __builtin_ctz(val);
    #elif ___SP_DETECTED_COMPILER___ == msvc
        if constexpr(spt::is_same_v<T, ull>||spt::is_same_v<T, ll>) return _BitScanForward(&index, val);
        if constexpr(spt::is_same_v<T, unsigned long>||spt::is_same_v<T, long>) return _BitScanForward(&index, val);
        return _BitScanForward(&index, val);
    #endif
    }

    template <typename T>
    SP_FORCEINLINE SP_PURE ull find_first_set(T val){
    #if ___SP_DETECTED_COMPILER___ == clang
        if constexpr(spt::is_same_v<T, ull>||spt::is_same_v<T, ll>) return __builtin_ffsll(val);
        if constexpr(spt::is_same_v<T, unsigned long>||spt::is_same_v<T, long>) return __builtin_ffsl(val);
        return __builtin_ffs(val);
    #elif ___SP_DETECTED_COMPILER___ == msvc
        if constexpr(spt::is_same_v<T, ull>||spt::is_same_v<T, ll>) return _BitScanForward(&index, val);
        if constexpr(spt::is_same_v<T, unsigned long>||spt::is_same_v<T, long>) return _BitScanForward(&index, val);
        return _BitScanForward(&index, val);
    #endif
    }
};
#endif // ____SP_BIT_MANIP____