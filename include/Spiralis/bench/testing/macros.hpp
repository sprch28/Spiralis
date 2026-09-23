#ifndef ____SP_BENCH_MACROS____
#define ____SP_BENCH_MACROS____
#pragma once

#include "assertions.hpp"
#include "test_registry.hpp"
#include "expect.hpp"


// ============================================================
// ============================================================

#define SP_ASSERT(tf, cond, ...) \
    sp::test::assert(tf, cond, #cond, __FILE__, __LINE__, #__VA_ARGS__, sp::test::pass(__VA_ARGS__))


#define SP_ASSERT_TRUE(cond, ...) \
    SP_ASSERT(true, cond, __VA_ARGS__)


#define SP_ASSERT_FALSE(cond, ...) \
    SP_ASSERT(false, cond, __VA_ARGS__)

#define SP_TEST_EXPECT(tf, cond, ...) \
    sp::test::expect(tf, cond, #cond, __FILE__, __LINE__, #__VA_ARGS__, sp::test::pass(__VA_ARGS__))


#define SP_TEST_EXPECT_TRUE(cond, ...) \
    SP_TEST_EXPECT(true, cond, __VA_ARGS__)


#define SP_TEST_EXPECT_FALSE(cond, ...) \
    SP_TEST_EXPECT(false, cond, __VA_ARGS__)

#define SP_DEBUG(...) \
sp::print(sp::console::FG_YELLOW, "[DEBUG] "); __VA_ARGS__; sp::println(sp::console::RESET_EFFECTS);


// ============================================================
// ============================================================

#define SP_CONCAT_IMPL(a, b) a##b
#define SP_CONCAT(a, b) SP_CONCAT_IMPL(a, b)


// ============================================================
// ============================================================

#define SP_TEST(name) \
    static void SP_CONCAT(Spiralis_Test_Func_, __LINE__)(); \
    static ::sp::test::test_registration \
        SP_CONCAT(Spiralis_Test_Reg_, __LINE__)(name,&SP_CONCAT(Spiralis_Test_Func_, __LINE__)); \
    static void SP_CONCAT(Spiralis_Test_Func_, __LINE__)()

#define SP_RUN_TESTS() sp::test::test_registry.run()

#endif