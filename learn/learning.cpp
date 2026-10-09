// I made this shit in hopes of showing you the current stuff this library can do and get you familiar with it
// Do your best not to claude any of it

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// There are currently special modes that can be toggled using double-underscore macros found starting on line 87 of init.hpp.
// Defining them either using -D in the build command or using #define before including the library lets you control them.

// Enable/disable branch hinting (likely/unlikely if statements)
// 1 by default
// 1 = enabled, 0 = disabled
#define __SP_LIKELY__ 1

// Enable/disable manual loop unrolling
// Manual loop unrolling is good to enable when compiling with -O1 or lower; harmful with higher optimizations
// 0 by default
#define __SP_UNROLL_LOOPS__ 1

// sp::hash_map has a lot of template parameters:
// key, value, safety level, % reallocation threshold (as an integer out of 100), hash function class, allocator class
// __SP_DEFAULT_MAP_TRAITS__ is used by default when not all values are entered by the user.
// It can be customized like this:
#define __SP_DEFAULT_MAP_TRAITS__ 1, 90, sp::wyhash, sp::aligned_allocator
// Now creating sp::hmap<sp::string, int> anywhere in the file defaults to those settings
// Using sp::hash_map still keeps the normal defaults, while def_map uses these customized ones.

// Some containers such as sp::array have a safety level of 1 (safe) and 0 (unsafe).
// The level chosen by default can be controlled using:
#define __SP_DEFAULT_SAFETY_LEVEL__ 1 // Safe mode by default

// size_type is used everywhere, and you can decide what it is globally at compile time.
// by default, it's decltype(sizeof(0)) which is usually something like unsigned long
// I'm noticing errors when making it below 64-bit, so I'd be cautious about using this one
#define __SP_SIZE_TYPE__ unsigned long

// The I/O module uses a char buffer to minimize system calls. 
// By default, this buffer can hold 32768 chars (1 << 15).
#define __SP_IO_BUFFER_SIZE__ (1 << 16)

// The hba is an array where you can puncture holes.
// When calling size-modifying functions, we need to decide what happens when reallocation is needed.
// We can either compress while reallocating, or copy the holes.
// This dictates which choice we make by default.
// true by default
#define __SP_HBA_DEFAULT_COMPRESS__ true

// The hba uses a layer hierarchy. Each layer can address 64^L elements at once.
// By default, it's 3 layers. This is a good amount for most applications.
#define __SP_HBA_NUM_LAYERS__ 1 // If we wanted to have small amounts of data

// Spiralis also provides a name alias for unsigned long long and long long
// It can optionally put these aliases in the global namespace.
// It does by default.
#define __SP_GLOBAL_NAMESPACE_ULL__ 0 // This would keep ull and ll out of global namespace

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Spiralis also has optionally-included extensions. That currently includes:
// ml: tokenizers, tensor
// benchmark: correctness testing and soon benchmarking

#define __SP_ML__ // Tell Spiralis to pull in the ML library + tensor class
#define __SP_BENCHMARK__ // Tell Spiralis to pull in the testing/benchmark framework

// Now that our macros are configured and defined how we want them, we can include Spiralis.
// It also works just fine without defining any of that shit but it will just use the default configurations I chose
#include "Spiralis/include/Spiralis/Spiralis.hpp" // Adjust include path if needed

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


int main(){
    //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
                                    // CONTAINERS
    // There are several important containers implemented so far. You've worked on one of them already.
    // ------------------
    // array
    // ------------------
    // array has safety level 1 and safety level 0. These dictate whether it performs reallocation checks automatically.
    // safety level 0 means you must reserve yourself; UB is possible
    // safety level 1 ensures vector-level safety and reallocation checks
    // Level 0 is much faster, but you don't need to choose one or the other.
    {
    sp::array<int, 1, sp::allocator> arr = {1, 2, 3, 4, 5}; // array of ints at safety level of 1, using sp::allocator

    // Even though it defaults to level 1, you can call unsafe functions:
    arr.reserve(9); // ensure sufficient capacity
    arr.push_back<0>(5); // Performs no reallocation check because we know there's enough room.
    arr.push_back<0>(10);
    arr.push_front<0>(60);
    arr.print(); // For convenience: just uses std::cout
    // Now that it has documentation im sure i dont need to explain all the functions
    }

    // The array also has alias names:
    // sp::vector is a safe array with sp::allocator
    // sp::uvector is unsafe array with sp::allocator
    // theres a bunch more; they're at the bottom of array.hpp. It's about safety levels and allocators.
    // I use sp::vector mostly just because it's the default safety and allocator and has same name as std::vector

    
    // ------------------
    // hash_map
    // ------------------
    // I fw hash maps heavy lowk
    // As mentioned above it has lots of params
    {
    sp::hmap<sp::string, int> i = {
        {"Hello",1},
        {"World",2}
    };// Because of the above macro, it will default to:
    // sp::hash_map<sp::string, int, 1, 90, sp::wyhash, sp::aligned_allocator>

    i.print_stats(); // also uses cout right now
    // This one has a ton of functions and should work similar to unordered_map + more features
    }


    // ------------------
    // string
    // ------------------
    // I know damn well you're familiar with this one. 
    {
    sp::string str = "Hello World";
    // There are cool ways to use it though (python-like syntax):
    auto arr = str.split<sp::vector<sp::string>>(' ');
    // arr contains: ["Hello","World"]
    sp::string rejoined = "_"_sp.join(arr);
    // Rejoined contains: "Hello_World"

    // As seen above, any text followed by '_sp' creates a string by default. This is pretty cool imo
    sp::println("Hello"_sp*5); // I lowk noticed some error here when doing this; It will be fixed soon
    }
    

    // ------------------
    // hba (prototype)
    // ------------------
    // To the user, it behaves identical to sp::array (but without the safety level)
    // However, you can puncture holes.
    // int, 2 layers, compress on realloc by default, aligned allocator
    {
    sp::hba<int, 2, true, sp::aligned_allocator> arr = {1, 2, 3, 4, 5};

    arr.erase(1).erase(1); // Erases elements '2' and '3'
    arr.print(); // [1, 4, 5]
    // This looks normal, but the data layout under the hood is:
    // [1, x, x, 4, 5], where x represents a hole.
    // Grabbing the logical index works perfectly because get_idx maps logical to physical.
    // It's a bit slower than a normal operator[] (obviously), but we can fix this.
    arr.compress().print(); // Still prints the same stuff
    // However, now the data is:
    // [1, 4, 5] instead of [1, x, x, 4, 5].
    // operator[] now returns the physical idx directly (fast path)
    }

    // ------------------
    // bitset
    // ------------------
    // This one's simple but useful for debugging.
    sp::size_type i = 39172918;
    // Bitset of 'sizeof(size_type)*8 number of bits'
    sp::bitset<sizeof(size_type)*8> set(i);
    sp::println(set.to_string());

    // ------------------
    // tensor
    // ------------------

    // ------------------
    // tokenizer (this ones cool)
    // ------------------

    // When I wake up I'll also cover allocators, math functions, I/O, SIMD, Multithreading, and the testing framework

    //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    return 0;
}