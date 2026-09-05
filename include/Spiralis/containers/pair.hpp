#ifndef ____SP_PAIR____
#define ____SP_PAIR____
#pragma once
#include "../setup/init.hpp"
#include <ostream>

namespace sp{
template <typename First, typename Second>
class pair{
public:
    First first;
    Second second;
    constexpr pair() : first(), second(){}
    constexpr pair(First one, Second two) : first(one), second(two){}
};

template <typename First, typename Second>
constexpr std::ostream& operator<<(std::ostream& os, const sp::pair<First, Second>& p) {
    os << "(" << p.first << ", " << p.second << ")";
    return os;
}
}

#endif