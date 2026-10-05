#ifndef ____SP_TESTS_HBA____
#define ____SP_TESTS_HBA____
#pragma once
#define __SP_BENCHMARK__
#include "../../include/Spiralis/containers/hba.hpp"
#include "../../include/Spiralis/bench/test.hpp"
#include "trivial.hpp"

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

SP_TEST("Compress"){
    T1 a(128, Trivial({50}));
    T10 b(128, Trivial({50}));

    SP_ASSERT_TRUE(a.size()==128,a.size());
    SP_ASSERT_TRUE(b.size()==128,b.size());
    SP_ASSERT_TRUE(a.is_contiguous());
    SP_ASSERT_TRUE(b.is_contiguous());

    for(ull i = 0; i < 128; ++i){
        SP_ASSERT_TRUE(a[i].value==50,a[i].value);
        SP_ASSERT_TRUE(b[i].value==50,b[i].value);
    }

    a.erase(20).erase(60).erase(100);
    b.erase(20).erase(60).erase(100);

    SP_ASSERT_TRUE(a.size()==125,a.size());
    SP_ASSERT_TRUE(b.size()==125,b.size());
    SP_ASSERT_FALSE(a.is_contiguous());
    SP_ASSERT_FALSE(b.is_contiguous());

    a.compress();
    b.compress();

    SP_ASSERT_TRUE(a.size()==125,a.size());
    SP_ASSERT_TRUE(b.size()==125,b.size());
    SP_ASSERT_TRUE(a.is_contiguous());
    SP_ASSERT_TRUE(b.is_contiguous());
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

// erase_unordered
SP_TEST("Erase unordered"){
    NT1 a(65);
    NT10 b(65);
    for(ull i = 0; i < 65; ++i){
        a.at_physical(i).value = i;
        b.at_physical(i).value = i;
    }

    a.erase_unordered(20);
    b.erase_unordered(20);

    SP_ASSERT_TRUE(a.size()==64,a.size()); 
    SP_ASSERT_TRUE(b.size()==64,b.size());

    SP_TEST_EXPECT_TRUE(a.back().value==63,a.back().value);
    SP_TEST_EXPECT_TRUE(b.back().value==63,b.back().value);
    SP_TEST_EXPECT_TRUE(a.at_physical(20).value==64,a.at_physical(20).value);
    SP_TEST_EXPECT_TRUE(b.at_physical(20).value==64,b.at_physical(20).value);
}

SP_TEST("Destructor Execution on Erase"){
    static int destroy_count = 0;
    struct Tracker{
        ~Tracker() { destroy_count++; }
    };

    using TrackHBA = sp::hba<Tracker, 1>;
    {
        TrackHBA container(5);
        destroy_count = 0;
        container.erase(2);
        SP_TEST_EXPECT_TRUE(destroy_count == 1, destroy_count);
    } // Remainder destroyed here (4 items left)
    SP_TEST_EXPECT_TRUE(destroy_count == 5, destroy_count);
}

SP_TEST("Word-Boundary Bitmask Edge Cases"){
    NT1 a(128);
    
    for(ull i = 0; i < 128; ++i){
        a[i].value = static_cast<int>(i);
    }

    a.erase(64); // Logical 64 (originally physical 64) removed
    a.erase(63); // Logical 63 (originally physical 63) removed

    SP_TEST_EXPECT_TRUE(a.idx(63) == 65, a.idx(63));
    SP_TEST_EXPECT_TRUE(a[63].value == 65, a[63].value);
    SP_TEST_EXPECT_TRUE(!a.is_contiguous());
}


// erase_compress
// erase_shift
// erase_range
// erase_if
// erase_range_if

// emplace
SP_TEST("Emplace"){
    NT1 a(65);
    NT10 b(65);
    for(ull i = 0; i < 65; ++i){
        a.at_physical(i).value = i;
        b.at_physical(i).value = i;
    }

    Non_Trivial v(10);
    a.emplace(2,v);
    b.emplace(2,v);

    SP_ASSERT_TRUE(a.size()==66,a.size());
    SP_ASSERT_TRUE(b.size()==66,b.size());

    SP_TEST_EXPECT_TRUE(a.at_physical(2).value==10,a.at_physical(2).value);
    SP_TEST_EXPECT_TRUE(b.at_physical(2).value==10,b.at_physical(2).value);
}

// emplace_back
SP_TEST("Emplace back"){
    NT1 a(65);
    NT10 b(65);
    for(ull i = 0; i < 65; ++i){
        a.at_physical(i).value = i;
        b.at_physical(i).value = i;
    }

    Non_Trivial v(10);
    a.emplace_back(v);
    b.emplace_back(v);

    SP_ASSERT_TRUE(a.size()==66,a.size());
    SP_ASSERT_TRUE(b.size()==66,b.size());

    for(ull i = 0; i < 65; ++i){
        SP_TEST_EXPECT_TRUE(a.at_physical(i).value==i,a.at_physical(i).value);
        SP_TEST_EXPECT_TRUE(b.at_physical(i).value==i,a.at_physical(i).value);
    }

    SP_TEST_EXPECT_TRUE(a.back().value==10,a.back().value);
    SP_TEST_EXPECT_TRUE(b.back().value==10,b.back().value);
}

// emplace_front
SP_TEST("Emplace front"){
    NT1 a(65);
    NT10 b(65);
    for(ull i = 0; i < 65; ++i){
        a.at_physical(i).value = i;
        b.at_physical(i).value = i;
    }

    Non_Trivial v(10);
    a.emplace_front(v);
    b.emplace_front(v);

    SP_ASSERT_TRUE(a.size()==66,a.size());
    SP_ASSERT_TRUE(b.size()==66,b.size());

    for(ull i = 0; i < 65; ++i){
        SP_TEST_EXPECT_TRUE(a.at_physical(i+1).value==i,a.at_physical(i+1).value);
        SP_TEST_EXPECT_TRUE(b.at_physical(i+1).value==i,a.at_physical(i+1).value);
    }

    SP_TEST_EXPECT_TRUE(a.front().value==10,a.front().value);
    SP_TEST_EXPECT_TRUE(b.front().value==10,b.front().value);
}

SP_TEST("Reallocation & Capacity Expansion"){
    NT1 container;
    ull initial_cap = container.capacity();
    
    for(ull i = 0; i < 1000; ++i){
        container.emplace_back(Non_Trivial(static_cast<int>(i)));
    }
    
    SP_TEST_EXPECT_TRUE(container.size() == 1000, container.size());
    SP_TEST_EXPECT_TRUE(container.capacity() > initial_cap, container.capacity());
    
    for (ull i = 0; i < 1000; ++i){
        SP_TEST_EXPECT_TRUE(container[i].value == static_cast<int>(i),container[i].value);
    }
}

// insert
SP_TEST("Insert"){
    NT1 a(65);
    NT10 b(65);
    for(ull i = 0; i < 65; ++i){
        a.at_physical(i).value = i;
        b.at_physical(i).value = i;
    }

    a.insert(2,Non_Trivial(10));
    b.insert(2,Non_Trivial(10));

    SP_ASSERT_TRUE(a.size()==66,a.size());
    SP_ASSERT_TRUE(b.size()==66,b.size());

    SP_TEST_EXPECT_TRUE(a.at_physical(2).value==10,a.at_physical(2).value);
    SP_TEST_EXPECT_TRUE(b.at_physical(2).value==10,b.at_physical(2).value);
}

// push_front
SP_TEST("Push front"){
    NT1 a(65);
    NT10 b(65);
    for(ull i = 0; i < 65; ++i){
        a.at_physical(i).value = i;
        b.at_physical(i).value = i;
    }

    a.push_front(Non_Trivial(10));
    b.push_front(Non_Trivial(10));

    SP_ASSERT_TRUE(a.size()==66,a.size());
    SP_ASSERT_TRUE(b.size()==66,b.size());

    for(ull i = 0; i < 65; ++i){
        SP_TEST_EXPECT_TRUE(a.at_physical(i+1).value==i,a.at_physical(i+1).value);
        SP_TEST_EXPECT_TRUE(b.at_physical(i+1).value==i,a.at_physical(i+1).value);
    }

    SP_TEST_EXPECT_TRUE(a.front().value==10,a.front().value);
    SP_TEST_EXPECT_TRUE(b.front().value==10,b.front().value);
}

// push_back
SP_TEST("Push back"){
    NT1 a(65);
    NT10 b(65);
    for(ull i = 0; i < 65; ++i){
        a.at_physical(i).value = i;
        b.at_physical(i).value = i;
    }

    a.push_back(Non_Trivial(10));
    b.push_back(Non_Trivial(10));

    SP_ASSERT_TRUE(a.size()==66,a.size());
    SP_ASSERT_TRUE(b.size()==66,b.size());

    for(ull i = 0; i < 65; ++i){
        SP_TEST_EXPECT_TRUE(a.at_physical(i).value==i,a.at_physical(i).value);
        SP_TEST_EXPECT_TRUE(b.at_physical(i).value==i,a.at_physical(i).value);
    }

    SP_TEST_EXPECT_TRUE(a.back().value==10,a.back().value);
    SP_TEST_EXPECT_TRUE(b.back().value==10,b.back().value);
}

// pop
SP_TEST("Pop"){
    NT1 a(128);
    NT10 b(128);
    for(ull i = 0; i < 128; ++i){
        a.at_physical(i).value = i;
        b.at_physical(i).value = i;
    }

    int a_elements[3]{};
    int b_elements[3]{};

    a_elements[2] = a.pop(65).value; a_elements[1] = a.pop(64).value; a_elements[0] = a.pop(63).value;
    b_elements[2] = b.pop(65).value; b_elements[1] = b.pop(64).value; b_elements[0] = b.pop(63).value;

    SP_ASSERT_TRUE(a.size()==125,a.size());
    SP_ASSERT_TRUE(b.size()==125,b.size());
    SP_ASSERT_FALSE(a.is_contiguous());
    SP_ASSERT_FALSE(b.is_contiguous());

    for(ull i = 0; i < 63; ++i){
        SP_TEST_EXPECT_TRUE(a.at_physical(i).value==i,a.at_physical(i).value);
        SP_TEST_EXPECT_TRUE(b.at_physical(i).value==i,b.at_physical(i).value);
    }

    SP_TEST_EXPECT_TRUE(a[63].value==66,a[63].value);
    SP_TEST_EXPECT_TRUE(b[63].value==66,b[63].value);
}

// pop_back
SP_TEST("Pop back"){
    NT1 a(64);
    NT10 b(64);
    for(ull i = 0; i < 128; ++i){
        a.at_physical(i).value = i;
        b.at_physical(i).value = i;
    }

    int elem_a = a.pop_back().value;
    int elem_b = b.pop_back().value;

    SP_ASSERT_TRUE(a.size()==63,a.size());
    SP_ASSERT_TRUE(b.size()==63,b.size());

    SP_TEST_EXPECT_TRUE(a.back().value==62,a.back().value);
    SP_TEST_EXPECT_TRUE(b.back().value==62,b.back().value);

    SP_TEST_EXPECT_TRUE(elem_a==63,elem_a);
    SP_TEST_EXPECT_TRUE(elem_b==63,elem_b);
}

// pop_front
SP_TEST("Pop front"){
    NT1 a(64);
    NT10 b(64);
    for(ull i = 0; i < 128; ++i){
        a.at_physical(i).value = i;
        b.at_physical(i).value = i;
    }

    int elem_a = a.pop_front().value;
    int elem_b = b.pop_front().value;

    SP_ASSERT_TRUE(a.size()==63,a.size());
    SP_ASSERT_TRUE(b.size()==63,b.size());

    SP_TEST_EXPECT_TRUE(a.front().value==1,a.front().value);
    SP_TEST_EXPECT_TRUE(b.front().value==1,b.front().value);

    SP_TEST_EXPECT_TRUE(elem_a==0,elem_a);
    SP_TEST_EXPECT_TRUE(elem_b==0,elem_b);
}

// clear
SP_TEST("Clear"){
    NT1 a(128);
    NT10 b(128);

    // No longer contiguous
    a.erase(27);
    b.erase(27);

    a.clear();
    b.clear();

    SP_TEST_EXPECT_TRUE(a.size()==0,a.size());
    SP_TEST_EXPECT_TRUE(b.size()==0,b.size());

    SP_TEST_EXPECT_TRUE(a.capacity()==128,a.capacity());
    SP_TEST_EXPECT_TRUE(b.capacity()==128,b.capacity());

    // Clearing data should set _is_contiguous to true again
    SP_TEST_EXPECT_TRUE(a.is_contiguous());
    SP_TEST_EXPECT_TRUE(b.is_contiguous());
}



} // namespace Spiralis_Test_HBA

#endif