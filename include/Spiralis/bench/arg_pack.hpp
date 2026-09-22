#ifndef ____SP_BENCH_ARG_PACK____
#define ____SP_BENCH_ARG_PACK____
#pragma once
#include "../setup/init.hpp"
#include "../containers/string.hpp"
#include "../io/IO.hpp"
#include "../core/exceptions.hpp"

namespace sp {
namespace test {

template <typename... Args>
struct arg_pack;


template <>
struct arg_pack<>{
    template <typename Func>
    constexpr void apply(Func&&) const {}
};


template <typename Head, typename... Tail>
struct arg_pack<Head, Tail...>{
    Head head;
    arg_pack<Tail...> tail;

    constexpr arg_pack(Head h, Tail... t) : head(sp::forward<Head>(h)), tail(sp::forward<Tail>(t)...) {}

    template <typename Func>
    constexpr void apply(Func&& fn) const{
        fn(head);
        tail.apply(sp::forward<Func>(fn));
    }
};

SP_FORCEINLINE constexpr arg_pack<> pass() {
    return {};
}


template <typename... Args>
SP_FORCEINLINE arg_pack<Args&&...> pass(Args&&... args) {
    return arg_pack<Args&&...>(
        sp::forward<Args>(args)...
    );
}

}}

#endif