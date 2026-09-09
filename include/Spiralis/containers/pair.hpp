#ifndef ____SP_PAIR____
#define ____SP_PAIR____
#pragma once
#include "../setup/init.hpp"
#include <ostream>

namespace sp {

template <typename T1, typename T2>
struct pair {
    T1 first;
    T2 second;

    constexpr pair() = default;

    template <typename U1, typename U2>
    constexpr pair(U1&& one, U2&& two) 
        : first(sp::forward<U1>(one)), second(sp::forward<U2>(two)) {}
};

} // namespace sp

template <typename First, typename Second>
constexpr std::ostream& operator<<(std::ostream& os, const sp::pair<First, Second>& p) {
    os << "(" << p.first << ", " << p.second << ")";
    return os;
}

#endif