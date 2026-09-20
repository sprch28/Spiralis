#ifndef ____SP_MATH____
#define ____SP_MATH____
#pragma once
#include "../setup/init.hpp"
namespace sp{

template <typename T, typename U>
SP_FORCEINLINE SP_PURE constexpr T min(T a, U b) { return a < b ? a : b; }

template <typename T, typename U>
SP_FORCEINLINE SP_PURE constexpr T max(T a, U b) { return a > b ? a : b; }

template <typename T>
SP_NODISCARD SP_FORCEINLINE SP_CONST constexpr T abs(T data){
    SP_IF_CONSTEXPR(static_cast<T>(5.25)==5.25) return (data < 0 ? -data : data);
    else{
        T const mask = data >> (sizeof(T) * 8 - 1);
        return (data ^ mask) - mask;
    }
}

}; // namespace sp
#endif // ____SP_MATH____