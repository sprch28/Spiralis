#ifndef ____SP_TESTS_ARRAY____
#define ____SP_TESTS_ARRAY____
#pragma once
#define __SP_BENCHMARK__
#include "../../include/Spiralis/Spiralis.hpp"

#include "trivial.hpp"
namespace Spiralis_Test_Array{

using T1 = sp::array<Trivial, 1>;
using T0 = sp::array<Trivial, 0>;
using NT1 = sp::array<Non_Trivial, 1>;
using NT0 = sp::array<Non_Trivial, 0>;

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// These tests should provide a fairly comprehensive 
// review of every constructor, function, etc. to ensure
// correctness and catch any bugs that occur when modifying
// the array class definition.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// Assertions are used in the constructors.
// Because they're used in other tests, constructors must be correct before proceeding.

SP_TEST("Empty Construction") { sp::array<Trivial, 1> i; sp::array<Trivial, 0> j; sp::array<Non_Trivial, 1> k; sp::array<Non_Trivial, 0> l; }
SP_TEST("Size Construction"){
    T1 i(20);
    T0 j(20);
    NT1 k(20);
    NT0 l(20);
    SP_ASSERT_TRUE(i.size() == 20 && j.size() == 20 && k.size() == 20 && l.size() == 20,i.size(),j.size(),k.size(),l.size());
    for(ull _i = 0; _i < 20; ++_i){
        SP_ASSERT_TRUE(i[_i].value == 0 && j[_i].value == 0 && k[_i].value == 0 && l[_i].value == 0,i[_i].value,j[_i].value,k[_i].value,l[_i].value);
    }
}
SP_TEST("Count + value construction"){
    T1 i(20, Trivial({15}));
    T0 j(20, Trivial({15}));
    NT1 k(20, Non_Trivial(15));
    NT0 l(20, Non_Trivial(15));
    SP_ASSERT_TRUE(i.size() == 20 && j.size() == 20 && k.size() == 20 && l.size() == 20,i.size(),j.size(),k.size(),l.size());
    for(ull _i = 0; _i < 20; ++_i){
        SP_ASSERT_TRUE(i[_i].value == 15 && j[_i].value == 15 && k[_i].value == 15 && l[_i].value == 15,i[_i].value,j[_i].value,k[_i].value,l[_i].value);
    }
}
SP_TEST("Data ptr + size construction"){
    sp::allocator<Trivial> alloc;
    Trivial* data = sp::allocator_traits<sp::allocator<Trivial>>::allocate(alloc, 20);
    for(ull i = 0; i < 20; ++i) sp::allocator_traits<sp::allocator<Trivial>>::construct(alloc, data+i, Trivial({15}));
    
    sp::allocator<Non_Trivial> alloc2;
    Non_Trivial* data2 = sp::allocator_traits<sp::allocator<Non_Trivial>>::allocate(alloc2, 20);
    for(ull i = 0; i < 20; ++i) sp::allocator_traits<sp::allocator<Non_Trivial>>::construct(alloc2, data2+i, Non_Trivial(15));

    T1 i(data, 20);
    T0 j(data, 20);
    NT1 k(data2, 20);
    NT0 l(data2, 20);

    SP_ASSERT_TRUE(i.size() == 20 && j.size() == 20 && k.size() == 20 && l.size() == 20,i.size(),j.size(),k.size(),l.size());
    for(ull _i = 0; _i < 20; ++_i){
        SP_ASSERT_TRUE(i[_i].value == 15 && j[_i].value == 15 && k[_i].value == 15 && l[_i].value == 15,i[_i].value,j[_i].value,k[_i].value,l[_i].value);
    }
}

}

#endif