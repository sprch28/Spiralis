#ifndef ____SP_PAIR____
#define ____SP_PAIR____
#pragma once
#include "../setup/init.hpp"
#include <ostream>

namespace sp {

/// Minimal heterogeneous pair.
/// @tparam T1 Type of the first element.
/// @tparam T2 Type of the second element.
template <typename T1, typename T2>
struct pair {
    T1 first;  ///< First element.
    T2 second; ///< Second element.

    /// Default-constructs both elements.
    constexpr pair() = default;

    /// Constructs the pair by forwarding @p one into @c first and @p two into @c second.
    /// @tparam U1 Deduced type for @c first; must be convertible to @c T1.
    /// @tparam U2 Deduced type for @c second; must be convertible to @c T2.
    template <typename U1, typename U2>
    constexpr pair(U1&& one, U2&& two) : first(sp::forward<U1>(one)), second(sp::forward<U2>(two)) {}
};

} // namespace sp

/// Writes the pair to @p os as @c (first, second).
/// @return Reference to @p os for chaining.
template <typename First, typename Second>
constexpr std::ostream& operator<<(std::ostream& os, const sp::pair<First, Second>& p) {
    os << "(" << p.first << ", " << p.second << ")";
    return os;
}

#endif