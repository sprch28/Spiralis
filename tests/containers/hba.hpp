#ifndef ____SP_TESTS_HBA____
#define ____SP_TESTS_HBA____
#pragma once
#define __SP_BENCHMARK__
#include "../../include/Spiralis/Spiralis.hpp"
#include "trivial.hpp"

namespace Spiralis_Test_Hba {

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

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// ELEMENT ACCESS AND MUTATION TESTS
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

SP_TEST("Subscript Operator and Pointers"){
    NT1 container(5, Non_Trivial(0));
    
    for(ull i = 0; i < container.size(); ++i){
        container[i].value = static_cast<int>(i * 10);
    }

    const NT1& const_ref = container;
    for(ull i = 0; i < const_ref.size(); ++i){
        const int expected = static_cast<int>(i * 10);
        SP_TEST_EXPECT_TRUE(const_ref[i].value == expected, i, const_ref[i].value, expected);
    }

    SP_ASSERT_TRUE(container.data() != nullptr);
    SP_ASSERT_TRUE(container.get_meta() != nullptr);
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// MUTATION OPERATIONS (ERASE, INSERT, COMPRESS)
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

SP_TEST("Single Erase Integrity"){
    T10 vec = {Trivial{10}, Trivial{20}, Trivial{30}, Trivial{40}, Trivial{50}};
    
    vec.erase(2); // Remove value 30
    
    SP_ASSERT_TRUE(vec.size() == 4, vec.size());
    SP_TEST_EXPECT_TRUE(vec[0].value == 10, vec[0].value);
    SP_TEST_EXPECT_TRUE(vec[1].value == 20, vec[1].value);
    SP_TEST_EXPECT_TRUE(vec[2].value == 40, vec[2].value);
    SP_TEST_EXPECT_TRUE(vec[3].value == 50, vec[3].value);
}

SP_TEST("Sequential and Boundary Erases"){
    NT1 vec(100, Non_Trivial(0));
    for(int i = 0; i < 100; ++i){
        vec[i].value = i;
    }

    vec.erase(0); // Erase head
    SP_TEST_EXPECT_TRUE(vec[0].value == 1, vec[0].value);

    vec.erase(vec.size() - 1); // Erase tail
    SP_TEST_EXPECT_TRUE(vec[vec.size() - 1].value == 98, vec.size(), vec[vec.size() - 1].value);

    vec.erase(10); // Originally value 11
    vec.erase(10); // Originally value 12

    SP_ASSERT_TRUE(vec.size() == 96, vec.size());
    SP_TEST_EXPECT_TRUE(vec[9].value == 10, vec[9].value);
    SP_TEST_EXPECT_TRUE(vec[10].value == 13, vec[10].value);
}

SP_TEST("Insert Shifts and Expansion"){
    T1 vec = {Trivial{1}, Trivial{2}, Trivial{4}, Trivial{5}};

    vec.insert(2, Trivial{3});

    SP_ASSERT_TRUE(vec.size() == 5, vec.size());
    for(ull i = 0; i < 5; ++i){
        const int expected = static_cast<int>(i + 1);
        SP_TEST_EXPECT_TRUE(vec[i].value == expected, i, vec[i].value, expected);
    }
}

SP_TEST("Compress Memory and Alignment"){
    NT10 vec(50, Non_Trivial(0));
    for(int i = 0; i < 50; ++i){
        vec[i].value = i;
    }

    // Punch sparse holes
    for(int i = 40; i >= 0; i -= 2){
        vec.erase(i);
    }

    const ull expected_size = vec.size();
    vec.compress();

    SP_ASSERT_TRUE(vec.size() == expected_size, vec.size(), expected_size);
    
    for(ull i = 0; i < vec.size(); ++i){
        SP_TEST_EXPECT_TRUE(vec[i].value >= 0, i, vec[i].value);
        if (i > 0){
            SP_TEST_EXPECT_TRUE(vec[i].value > vec[i - 1].value, i, vec[i].value, vec[i - 1].value);
        }
    }
}

SP_TEST("Erase-insertion"){
    T10 vec = {Trivial({1}), Trivial({2}), Trivial({3}), Trivial({4}), Trivial({5})};
    vec.erase(2); // Erases element 3
    SP_ASSERT_TRUE(vec.size()==4, vec.size());
    SP_TEST_EXPECT_TRUE(vec[0].value==1,vec[0].value);
    SP_TEST_EXPECT_TRUE(vec[1].value==2,vec[1].value);
    SP_TEST_EXPECT_TRUE(vec[2].value==4,vec[2].value);
    SP_TEST_EXPECT_TRUE(vec[3].value==5,vec[3].value);
    vec.insert(1,Trivial({6})); // Second element should be six; shift Trivial({2}) into the hole
    SP_TEST_EXPECT_TRUE(vec[0].value==1,vec[0].value);
    SP_TEST_EXPECT_TRUE(vec[1].value==6,vec[1].value);
    SP_TEST_EXPECT_TRUE(vec[2].value==2,vec[2].value);
    SP_TEST_EXPECT_TRUE(vec[3].value==4,vec[3].value);
    SP_TEST_EXPECT_TRUE(vec[4].value==5,vec[4].value);
    vec.set_contig(true); // set_contig(true) to force the quick access path
    SP_TEST_EXPECT_TRUE(vec[2].value==2,vec[2].value);
}

} // namespace Spiralis_Test_Hba

#endif