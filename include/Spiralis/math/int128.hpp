#ifndef ____SP_INT128____
#define ____SP_INT128____
#pragma once
#include "../setup/init.hpp"

namespace sp {

struct int128; // Forward declaration

struct uint128 {
public:
    using native_type = unsigned __int128;
    using param_type = const uint128&;

    native_type _val;

    // Declared inside the class block to fix the out-of-line definition error
    uint128(const int128& other);

    // Constructors (Exactly one single-argument constructor prevents all conversion ambiguities)
    SP_FORCEINLINE constexpr uint128() : _val(0) {}
    SP_FORCEINLINE constexpr uint128(native_type val) : _val(val) {}
    SP_FORCEINLINE constexpr uint128(ull high, ull low) : _val(((native_type)high << 64) | low) {}
    SP_FORCEINLINE constexpr uint128(const uint128& other) : _val(other._val) {}

    // Clean modern accessors
    SP_FORCEINLINE constexpr ull low() const { return static_cast<ull>(_val); }
    SP_FORCEINLINE constexpr ull high() const { return static_cast<ull>(_val >> 64); }

    // Bitwise Operators
    _SP_FUNC_NIP_ constexpr uint128 operator&(param_type other) const { return uint128(_val & other._val); }
    _SP_FUNC_NIP_ constexpr uint128 operator|(param_type other) const { return uint128(_val | other._val); }
    _SP_FUNC_NIP_ constexpr uint128 operator^(param_type other) const { return uint128(_val ^ other._val); }
    _SP_FUNC_NIP_ constexpr uint128 operator<<(unsigned int shift) const { return uint128(_val << (shift & 127)); }
    _SP_FUNC_NIP_ constexpr uint128 operator>>(unsigned int shift) const { return uint128(_val >> (shift & 127)); }
    _SP_FUNC_NIP_ constexpr uint128 operator~() const { return uint128(~_val); }

    SP_FORCEINLINE constexpr uint128& operator&=(param_type other) { _val &= other._val; return *this; }
    SP_FORCEINLINE constexpr uint128& operator|=(param_type other) { _val |= other._val; return *this; }
    SP_FORCEINLINE constexpr uint128& operator^=(param_type other) { _val ^= other._val; return *this; }
    SP_FORCEINLINE constexpr uint128& operator<<=(unsigned int shift) { _val <<= (shift & 127); return *this; }
    SP_FORCEINLINE constexpr uint128& operator>>=(unsigned int shift) { _val >>= (shift & 127); return *this; }

    // Arithmetic Operators
    SP_FORCEINLINE constexpr uint128& operator+=(param_type other) { _val += other._val; return *this; }
    SP_FORCEINLINE constexpr uint128& operator-=(param_type other) { _val -= other._val; return *this; }
    SP_FORCEINLINE constexpr uint128& operator*=(param_type other) { _val *= other._val; return *this; }
    SP_FORCEINLINE constexpr uint128& operator/=(param_type other) { _val /= other._val; return *this; }
    SP_FORCEINLINE constexpr uint128& operator%=(param_type other) { _val %= other._val; return *this; }

    SP_FORCEINLINE constexpr uint128& operator++() { ++_val; return *this; }
    SP_FORCEINLINE constexpr uint128 operator++(int) { uint128 res(*this); ++_val; return res; }
    SP_FORCEINLINE constexpr uint128& operator--() { --_val; return *this; }
    SP_FORCEINLINE constexpr uint128 operator--(int) { uint128 res(*this); --_val; return res; }

    _SP_FUNC_NIP_ constexpr uint128 operator+(param_type other) const { return uint128(_val + other._val); }
    _SP_FUNC_NIP_ constexpr uint128 operator-(param_type other) const { return uint128(_val - other._val); }
    _SP_FUNC_NIP_ constexpr uint128 operator*(param_type other) const { return uint128(_val * other._val); }
    _SP_FUNC_NIP_ constexpr uint128 operator/(param_type other) const { return uint128(_val / other._val); }
    _SP_FUNC_NIP_ constexpr uint128 operator%(param_type other) const { return uint128(_val % other._val); }

    // Comparison Operators
    _SP_FUNC_NIP_ constexpr bool operator==(param_type other) const { return _val == other._val; }
    _SP_FUNC_NIP_ constexpr bool operator!=(param_type other) const { return _val != other._val; }
    _SP_FUNC_NIP_ constexpr bool operator>=(param_type other) const { return _val >= other._val; }
    _SP_FUNC_NIP_ constexpr bool operator<=(param_type other) const { return _val <= other._val; }
    _SP_FUNC_NIP_ constexpr bool operator>(param_type other) const { return _val > other._val; }
    _SP_FUNC_NIP_ constexpr bool operator<(param_type other) const { return _val < other._val; }
    _SP_FUNC_NIP_ constexpr bool operator!() const { return !_val; }
    _SP_FUNC_NIP_ explicit operator bool() const { return _val != 0; }
    explicit operator int() const { return static_cast<int>(_val); }
    explicit operator char() const { return static_cast<char>(_val); }
    explicit operator unsigned long long() const { return _val; }
};

struct int128 {
public:
    using native_type = signed __int128;
    using param_type = const int128&;

    native_type _val;

    // Constructors
    SP_FORCEINLINE constexpr int128() : _val(0) {}
    SP_FORCEINLINE constexpr int128(native_type val) : _val(val) {}
    SP_FORCEINLINE constexpr int128(ull high, ull low) : _val(((native_type)high << 64) | low) {}
    SP_FORCEINLINE constexpr int128(const int128& other) : _val(other._val) {}
    SP_FORCEINLINE constexpr int128(const uint128& other) : _val((native_type)other._val) {}

    // Clean modern accessors
    SP_FORCEINLINE constexpr ull low() const { return static_cast<ull>(_val); }
    SP_FORCEINLINE constexpr long long high() const { return static_cast<long long>(_val >> 64); }

    // Bitwise Operators
    _SP_FUNC_NIP_ constexpr int128 operator&(param_type other) const { return int128(_val & other._val); }
    _SP_FUNC_NIP_ constexpr int128 operator|(param_type other) const { return int128(_val | other._val); }
    _SP_FUNC_NIP_ constexpr int128 operator^(param_type other) const { return int128(_val ^ other._val); }
    _SP_FUNC_NIP_ constexpr int128 operator<<(unsigned int shift) const { return int128(_val << (shift & 127)); }
    _SP_FUNC_NIP_ constexpr int128 operator>>(unsigned int shift) const { return int128(_val >> (shift & 127)); }
    _SP_FUNC_NIP_ constexpr int128 operator~() const { return int128(~_val); }

    SP_FORCEINLINE constexpr int128& operator&=(param_type other) { _val &= other._val; return *this; }
    SP_FORCEINLINE constexpr int128& operator|=(param_type other) { _val |= other._val; return *this; }
    SP_FORCEINLINE constexpr int128& operator^=(param_type other) { _val ^= other._val; return *this; }
    SP_FORCEINLINE constexpr int128& operator<<=(unsigned int shift) { _val <<= (shift & 127); return *this; }
    SP_FORCEINLINE constexpr int128& operator>>=(unsigned int shift) { _val >>= (shift & 127); return *this; }

    // Arithmetic Operators
    SP_FORCEINLINE constexpr int128& operator+=(param_type other) { _val += other._val; return *this; }
    SP_FORCEINLINE constexpr int128& operator-=(param_type other) { _val -= other._val; return *this; }
    SP_FORCEINLINE constexpr int128& operator*=(param_type other) { _val *= other._val; return *this; }
    SP_FORCEINLINE constexpr int128& operator/=(param_type other) { _val /= other._val; return *this; }
    SP_FORCEINLINE constexpr int128& operator%=(param_type other) { _val %= other._val; return *this; }

    SP_FORCEINLINE constexpr int128& operator++() { ++_val; return *this; }
    SP_FORCEINLINE constexpr int128 operator++(int) { int128 res(*this); ++_val; return res; }
    SP_FORCEINLINE constexpr int128& operator--() { --_val; return *this; }
    SP_FORCEINLINE constexpr int128 operator--(int) { int128 res(*this); --_val; return res; }

    _SP_FUNC_NIP_ constexpr int128 operator-() const { return int128(-_val); }
    _SP_FUNC_NIP_ constexpr int128 operator+(param_type other) const { return int128(_val + other._val); }
    _SP_FUNC_NIP_ constexpr int128 operator-(param_type other) const { return int128(_val - other._val); }
    _SP_FUNC_NIP_ constexpr int128 operator*(param_type other) const { return int128(_val * other._val); }
    _SP_FUNC_NIP_ constexpr int128 operator/(param_type other) const { return int128(_val / other._val); }
    _SP_FUNC_NIP_ constexpr int128 operator%(param_type other) const { return int128(_val % other._val); }

    // Comparison Operators
    _SP_FUNC_NIP_ constexpr bool operator==(param_type other) const { return _val == other._val; }
    _SP_FUNC_NIP_ constexpr bool operator!=(param_type other) const { return _val != other._val; }
    _SP_FUNC_NIP_ constexpr bool operator>=(param_type other) const { return _val >= other._val; }
    _SP_FUNC_NIP_ constexpr bool operator<=(param_type other) const { return _val <= other._val; }
    _SP_FUNC_NIP_ constexpr bool operator>(param_type other) const { return _val > other._val; }
    _SP_FUNC_NIP_ constexpr bool operator<(param_type other) const { return _val < other._val; }
    _SP_FUNC_NIP_ constexpr bool operator!() const { return !_val; }
    _SP_FUNC_NIP_ explicit operator bool() const { return _val != 0; }
    explicit operator int() const { return static_cast<int>(_val); }
    explicit operator char() const { return static_cast<char>(_val); }
    explicit operator unsigned long long() const { return _val; }
};

inline uint128::uint128(const int128& other) : _val((unsigned __int128)other._val) {}

inline uint128 make128(const char* str) {
    uint128 result(0);
    bool negative = false;
    if(*str == '-'){
        negative = true;
        str++;
    }else if(*str == '+'){
        str++;
    }
    while(*str){
        if(*str >= '0' && *str <= '9')result._val = (result._val * 10) + (*str - '0');
        str++;
    }
    return negative ? uint128(static_cast<uint128::native_type>(-static_cast<int128::native_type>(result._val))) : result;
}

constexpr uint128 UINT128_MAX((unsigned __int128)-1);
constexpr uint128 UINT128_MIN((unsigned __int128)0);
constexpr int128 INT128_MAX((signed __int128)((unsigned __int128)~0 >> 1));
constexpr int128 INT128_MIN(-INT128_MAX - 1);

}; // namespace sp
#endif // ____SP_INT128____