#ifndef ____SP_TESTS_HBA____
#define ____SP_TESTS_HBA____
#pragma once
#define __SP_BENCHMARK__
#include "../../include/Spiralis/containers/hba.hpp"
#include "../../include/Spiralis/bench/test.hpp"
#include "trivial.hpp"

/*

Compress
Emplace
Emplace_front
insert(copy)
insert(move)
push_front(copy)
push_front(move)

*/

namespace Spiralis_Test_HBA{

using T1   = sp::hba<Trivial, 1>;
using T10  = sp::hba<Trivial, 10>;
using NT1  = sp::hba<Non_Trivial, 1>;
using NT10 = sp::hba<Non_Trivial, 10>;

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// CONSTRUCTOR TESTS
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

SP_TEST("Default Construction"){
    T1 a;
    T10 b;
    NT1 c;
    NT10 d;

    SP_ASSERT_TRUE(a.size() == 0 && a.capacity() == 0 && a.get_meta_size() == 0, a.size(), a.capacity(), a.get_meta_size());
    SP_ASSERT_TRUE(b.size() == 0 && b.capacity() == 0 && b.get_meta_size() == 0, b.size(), b.capacity(), b.get_meta_size());
    SP_ASSERT_TRUE(c.size() == 0 && c.capacity() == 0 && c.get_meta_size() == 0, c.size(), c.capacity(), c.get_meta_size());
    SP_ASSERT_TRUE(d.size() == 0 && d.capacity() == 0 && d.get_meta_size() == 0, d.size(), d.capacity(), d.get_meta_size());

    SP_TEST_EXPECT_TRUE(a.empty(), a.empty());
    SP_TEST_EXPECT_TRUE(b.is_empty(), b.is_empty());
    SP_TEST_EXPECT_TRUE(c.empty(), c.empty());
    SP_TEST_EXPECT_TRUE(d.is_empty(), d.is_empty());
}

SP_TEST("Sized Default Value Construction"){
    T1 a(20);
    T10 b(20);
    NT1 c(20);
    NT10 d(20);

    SP_ASSERT_TRUE(a.size() == 20 && a.capacity() == 32 && a.get_meta_size() == 1, a.size(), a.capacity(), a.get_meta_size());
    SP_ASSERT_TRUE(b.size() == 20 && b.capacity() == 32 && b.get_meta_size() == 10, b.size(), b.capacity(), b.get_meta_size());
    SP_ASSERT_TRUE(c.size() == 20 && c.capacity() == 32 && c.get_meta_size() == 1, c.size(), c.capacity(), c.get_meta_size());
    SP_ASSERT_TRUE(d.size() == 20 && d.capacity() == 32 && d.get_meta_size() == 10, d.size(), d.capacity(), d.get_meta_size());

    for(ull i = 0; i < 20; ++i){
        SP_TEST_EXPECT_TRUE(a[i].value == 0, i, a[i].value);
        SP_TEST_EXPECT_TRUE(b[i].value == 0, i, b[i].value);
        SP_TEST_EXPECT_TRUE(c[i].value == 0, i, c[i].value);
        SP_TEST_EXPECT_TRUE(d[i].value == 0, i, d[i].value);
    }
}

SP_TEST("Sized Value Explicit Construction"){
    T1 a(200, Trivial{10});
    T10 b(200, Trivial{10});
    NT1 c(200, Non_Trivial(10));
    NT10 d(200, Non_Trivial(10));

    SP_ASSERT_TRUE(a.size() == 200 && a.capacity() == 256 && a.get_meta_size() == 4, a.size(), a.capacity(), a.get_meta_size());
    SP_ASSERT_TRUE(b.size() == 200 && b.capacity() == 256 && b.get_meta_size() == 13, b.size(), b.capacity(), b.get_meta_size());
    SP_ASSERT_TRUE(c.size() == 200 && c.capacity() == 256 && c.get_meta_size() == 4, c.size(), c.capacity(), c.get_meta_size());
    SP_ASSERT_TRUE(d.size() == 200 && d.capacity() == 256 && d.get_meta_size() == 13, d.size(), d.capacity(), d.get_meta_size());

    for(ull i = 0; i < 200; ++i){
        SP_TEST_EXPECT_TRUE(a[i].value == 10, i, a[i].value);
        SP_TEST_EXPECT_TRUE(b[i].value == 10, i, b[i].value);
        SP_TEST_EXPECT_TRUE(c[i].value == 10, i, c[i].value);
        SP_TEST_EXPECT_TRUE(d[i].value == 10, i, d[i].value);
    }
}

SP_TEST("Initializer List Construction"){
    T1 a = {Trivial{1}, Trivial{2}, Trivial{3}, Trivial{4}, Trivial{5}};
    T10 b = {Trivial{1}, Trivial{2}, Trivial{3}, Trivial{4}, Trivial{5}};
    NT1 c = {Non_Trivial(1), Non_Trivial(2), Non_Trivial(3), Non_Trivial(4), Non_Trivial(5)};
    NT10 d = {Non_Trivial(1), Non_Trivial(2), Non_Trivial(3), Non_Trivial(4), Non_Trivial(5)};

    SP_ASSERT_TRUE(a.size() == 5 && a.capacity() == 8 && a.get_meta_size() == 1, a.size(), a.capacity(), a.get_meta_size());
    SP_ASSERT_TRUE(b.size() == 5 && b.capacity() == 8 && b.get_meta_size() == 10, b.size(), b.capacity(), b.get_meta_size());
    SP_ASSERT_TRUE(c.size() == 5 && c.capacity() == 8 && c.get_meta_size() == 1, c.size(), c.capacity(), c.get_meta_size());
    SP_ASSERT_TRUE(d.size() == 5 && d.capacity() == 8 && d.get_meta_size() == 10, d.size(), d.capacity(), d.get_meta_size());

    for(ull i = 0; i < 5; ++i){
        const int expected = static_cast<int>(i + 1);
        SP_TEST_EXPECT_TRUE(a[i].value == expected, i, a[i].value, expected);
        SP_TEST_EXPECT_TRUE(b[i].value == expected, i, b[i].value, expected);
        SP_TEST_EXPECT_TRUE(c[i].value == expected, i, c[i].value, expected);
        SP_TEST_EXPECT_TRUE(d[i].value == expected, i, d[i].value, expected);
    }
}

SP_TEST("Erase"){
    NT1 a(128);
    NT10 b(128);

    SP_ASSERT_TRUE(a.size()==128,a.size());
    SP_ASSERT_TRUE(b.size()==128,b.size());

    for(ull i = 0; i < 128; ++i){
        a[i].value = i;
        b[i].value = i;
    }
    NT1 c(a);
    NT10 d(b);

    a.erase(64);
    b.erase(64);
    /*sp::bitset<64> set(a.get_meta()[1]);
    SP_DEBUG(sp::println(set.to_string()));
    SP_DEBUG(sp::println(a.is_slot_active(64)));*/
    SP_ASSERT_TRUE(a.idx(64)==65,a.idx(64));
    SP_ASSERT_TRUE(b.idx(64)==65,b.idx(64));
    SP_TEST_EXPECT_TRUE(a[64].value==65, a[64].value);
    SP_TEST_EXPECT_TRUE(b[64].value==65, b[64].value);

    c.erase(63);
    d.erase(63);
    SP_ASSERT_TRUE(c.idx(63)==64,c.idx(63));
    SP_ASSERT_TRUE(d.idx(63)==64,d.idx(63));
    SP_TEST_EXPECT_TRUE(c[63].value==64,c[63].value);
    SP_TEST_EXPECT_TRUE(d[63].value==64,d[63].value);

    a.erase(0);
    b.erase(0);
    SP_ASSERT_TRUE(a.idx(0)==1,a.idx(0));
    SP_ASSERT_TRUE(b.idx(0)==1,b.idx(0));
    SP_TEST_EXPECT_TRUE(a[0].value==1,a[0].value);
    SP_TEST_EXPECT_TRUE(b[0].value==1,b[0].value);
}

SP_TEST("Copy Semantics"){
    T1 original(10, Trivial{42});
    original.erase(2);
    original.erase(5);

    T1 copy_constructed(original);
    SP_ASSERT_TRUE(copy_constructed.size() == original.size(), copy_constructed.size(), original.size());
    SP_ASSERT_TRUE(copy_constructed.capacity() == original.capacity(), copy_constructed.capacity(), original.capacity());

    for(ull i = 0; i < copy_constructed.size(); ++i){
        SP_TEST_EXPECT_TRUE(copy_constructed[i].value == 42, i, copy_constructed[i].value);
    }

    copy_constructed[0].value = 99;
    SP_TEST_EXPECT_TRUE(original[0].value == 42, original[0].value);
    SP_TEST_EXPECT_TRUE(copy_constructed[0].value == 99, copy_constructed[0].value);
}

SP_TEST("Move Semantics"){
    NT10 source(15, Non_Trivial(77));
    source.erase(3);

    const ull prev_size = source.size();
    const ull prev_cap  = source.capacity();

    NT10 moved(sp::move(source));

    SP_ASSERT_TRUE(moved.size() == prev_size, moved.size(), prev_size);
    SP_ASSERT_TRUE(moved.capacity() == prev_cap, moved.capacity(), prev_cap);
    SP_TEST_EXPECT_TRUE(source.size() == 0, source.size());
    SP_TEST_EXPECT_TRUE(source.capacity() == 0, source.capacity());
    SP_TEST_EXPECT_TRUE(source.data() == nullptr);
    SP_TEST_EXPECT_TRUE(source.get_meta() == nullptr);

    for(ull i = 0; i < moved.size(); ++i){
        SP_TEST_EXPECT_TRUE(moved[i].value == 77, i, moved[i].value);
    }
}



} // namespace Spiralis_Test_HBA

#endif