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
    {
    sp::size_type i = 39172918;
    // Bitset of 'sizeof(size_type)*8' number of bits
    sp::bitset<sizeof(size_type)*8> set(i);
    sp::println(set.to_string());
    }

    // ------------------
    // tensor
    // ------------------
    // Still a work in progress, but so far it has lots of math ops.
    // Similar to an array, but size is fixed at creation.
    {
    sp::tensor<int, sp::aligned_allocator> t(3,3,3); // 3D tensor of shape 3x3x3 (size 27 in total)
    // Data layout is contiguous, and dimensions are simulated by calculating shapes and strides
    t.print_flat(); // Auto filled with zeros

    // Using namespace spml gives tensor creation functions
    for(auto& u : spml::arange(10)) sp::println(u);
    auto u = spml::full(5, 3, 3); // 3x3 matrix of element 5
    u.print_flat();

    auto u2 = spml::linspace<double, false>(0, 60, 8); // from range 0-60, non-inclusive, within 8 steps
    u2.print_flat();

    auto u3 = spml::arange<double>(0.0,2.0,0.1); // start, stop, step
    u3.print_flat();

    // There are a shit ton of operations already so far. functions leading with c_ create a copy.
    // Functions without c_ at the start modify their calling tensors.
    u.c_exp().print_flat(); // copied version with sin function applied elementwise
    u.print_flat(); // Will still be the same

    u2.cosh().print_flat(); // modifying cosh applied elementwise
    u2.print_flat(); // will be permanently modified
    }

    // This shits already got prob close to 100 functions if not more. It has lots of math functions and activation functions.
    // The modulo sign is used for matmul, or you can just call matmul() directly.

    // ------------------
    // tokenizer (this ones cool)
    // ------------------
    // Assuming you have some dataset file in the same directory:
    // (setup using IO, which will be covered later):
    {
    sp::print("Enter dataset file name: ",sp::flush);
    sp::io.flush();
    sp::string filename = sp::io.getLine();
    sp::println(filename.size());
    sp::println(filename);
    sp::file f(filename.c_str()); sp::IO scanner(f);
    sp::string dataset; 
    while(scanner.getLine<false>(dataset)){}

    // And now the tokenizer:
        // max vocab size, greedy_subword<min char length, max char length, integral data type for tokens>
    sp::tokenizer<40'000ULL, sp_pol::greedy_subword_tokenizer<2ULL, 6ULL, uint32_t>> tok;
    tok.build_mapping_debug(dataset); // build_mapping() builds without printing completion progress info
    sp::vector<uint32_t> tokens = tok.tokenize("Hello World!");
    sp::println(tokens.size()); // How many tokens?
    sp::string r = tok.reconstructed_string(tokens); // Turning tokens array back into a string
    sp::println(r);
    // Can also save the tokenizer to a file and load it later using tok.to_file() / tok.from_file()
    }

    // ------------------
    // Now one of the biggest parts: I/O
    // ------------------
    // I'm most proud of this API because it reads in a very modern way.
    {
    sp::file f("sample.txt",sp::file_mode::write); // Possible modes are read, write, rw, append. You can write sp::write or sp::file_mode::write. Both work.
    sp::IO scanner(f); // Create a new I/O module, constructed to be attached to the file
    scanner.println("Hello World"); // Writes to file and flushes (println calls flush automatically)
    sp::vector<int> vec = {1, 2, 3, 4, 5};
    // Write the size in binary, then the vector using its serialization protocol
    scanner.write(vec.size(),vec).flush(); // Binary serialization automatically provided with many types

    // We can also redirect inputs/outputs separately.
    // This writes whatever the user enters in the console into the file. 
    // Input is reading stdin, while output is directed to a file directory.
    scanner.println(scanner.input_to_console().getLine());
    scanner.output_to_console().println("Input recorded."); // Now both are pointing to stdin/stdout

    f.change_mode(sp::read).rewind(); // Change permissions and go to start
    // to_file(), to_console(), to_cerr() change both input AND output streams
    sp::println(scanner.to_file(f).getLine().to_upper()); // HELLO WORLD
    sp::size_type reconstructed_size = scanner.read<sp::size_type>().first; // sp::pair<size_type, bool>
    // rebuild vec: {1, 2, 3, 4, 5}
    sp::vector<int> reconstructed(reconstructed_size);
    scanner.read(reconstructed);
    reconstructed.print(); // {1, 2, 3, 4, 5}
    }

    // ------------------
    // SIMD
    // ------------------
    // The simd file is easily the biggest pain in the ass, which you'll see right away if you click on it.
    // There's so much macro soup. Worth it? tbh idk, but tensor automatically uses it where supported under the hood to accelerate its elementwise ops
    {
    auto one = spml::full<double>(3.0, 10);
    auto two = spml::full<double>(5.0, 10);
    sp::tensor<double> res(10);
    sp::simd::add(one.data(),two.data(),res.data(),res.size());
    res.print(); // Filled with 8.0
    }

    // simd also has sub, mul, div, fma, and a lot of selection functions.
    // You can pass a sp::thread_pool (see below) object as the final parameter to any function, and it will split the work if the size is big enough.
    // I'm considering a refactor of SIMD to be more practical but idk yet

    // ------------------
    // thread
    // ------------------
    // Very simple thread class that wraps pthread.
    sp::thread t;
    t.run([](){sp::println("Hi");});
    t.detach();
    t.join();
    // Can also pass functions when constructing.

    // ------------------
    // thread_pool
    // ------------------
    // No constructor params defaults to max system threads
    // Or you can specify number of threads
    sp::thread_pool p(4); // Use 4 threads
    // p.size() returns 4
    thread_local int sum = 0;
    for(sp::ull i = 0; i < 50; ++i) p.enqueue([](){
        sum += 5; // Add to the thread local counter
    });
    p.wait_all();
    for(sp::ull i = 0; i < p.size(); ++i) p.enqueue([](){
        sp::println(sum);
        sleep(2);
    });
    p.wait_all();

    //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    return 0;
}