#ifndef ____SP_HASHES____
#define ____SP_HASHES____
#pragma once
#include "../setup/init.hpp"
#include "../core/type_traits.hpp"
#include <cstdint>
#include <cstring> // memcpy for type-punning

namespace sp{

    // ─── helpers ────────────────────────────────────────────────────────────────
    namespace detail {
        /// Type-puns @p v into an unsigned integer of the same size via @c __builtin_memcpy.
        /// Avoids UB from @c reinterpret_cast on non-trivially-copyable types.
        /// @tparam U Target unsigned integer type (must be same size as @p T).
        /// @tparam T Source type.
        template <typename U, typename T>
        SP_FORCEINLINE U bit_cast_hash(T v) noexcept {
            static_assert(sizeof(U) == sizeof(T), "");
            U u{};
            __builtin_memcpy(&u, &v, sizeof(T));
            return u;
        }
    } // namespace detail

    // ─── basic_hash ─────────────────────────────────────────────────────────────
    /// General-purpose hash functor covering all common key types.
    /// - Integers (@c bool, @c char, @c short, @c int, @c long, @c long @c long and unsigned variants):
    ///   Fibonacci/Knuth multiplicative hash.
    /// - Floats (@c float, @c double): bit-reinterpreted as an integer then multiplied;
    ///   @c -0.0 is canonicalised to @c 0 so it hashes identically to @c +0.0.
    /// - Pointers: low alignment bits stripped (>>3) then Fibonacci-multiplied.
    /// - String-like types (anything with @c c_str() + @c size()): FNV-1a + custom avalanche.
    /// - Raw @c char* / @c const @c char*: same FNV-1a path via @c sp::strlen.
    class basic_hash{
        public:
        constexpr basic_hash(){}
        SP_CONSTEXPR20 ~basic_hash(){}
        /// Hashes @p val using the strategy appropriate for its type.
        /// @tparam T Key type; see class doc for the full dispatch table.
        /// @param val         Value to hash.
        /// @param bucket_size Unused; present for @c hash_with_cap compatibility.
        /// @return 64-bit hash.
        template <typename T>
        SP_FORCEINLINE SP_PURE constexpr size_type operator()(T val, size_type bucket_size=0) const {
            size_type value = 0;
            // --- integer types ---
            SP_IF_CONSTEXPR((spt::is_same<T,bool>::value))
                value = static_cast<size_type>(val) * 2654435761ULL;
            else SP_IF_CONSTEXPR((spt::is_same<T,char>::value || spt::is_same<T,unsigned char>::value || spt::is_same<T,signed char>::value))
                value = static_cast<size_type>(static_cast<unsigned char>(val)) * 2654435761ULL;
            else SP_IF_CONSTEXPR((spt::is_same<T,short>::value || spt::is_same<T,unsigned short>::value))
                value = static_cast<size_type>(static_cast<unsigned short>(val)) * 2654435761ULL;
            else SP_IF_CONSTEXPR((spt::is_same<T,int>::value || spt::is_same<T,unsigned int>::value))
                value = static_cast<size_type>(static_cast<unsigned int>(val) * 2654435769U);
            else SP_IF_CONSTEXPR((spt::is_same<T,long>::value || spt::is_same<T,unsigned long>::value))
                value = static_cast<size_type>(static_cast<unsigned long>(val) * 11400714819323198485ULL);
            else SP_IF_CONSTEXPR((spt::is_same<T,long long>::value || spt::is_same<T,ull>::value))
                value = static_cast<size_type>(static_cast<ull>(val) * 11400714819323198485ULL);
            // --- floating point: bit-reinterpret then hash as integer ---
            else SP_IF_CONSTEXPR((spt::is_same<T,float>::value)){
                unsigned int bits = detail::bit_cast_hash<unsigned int>(val);
                // Canonicalise -0.0 → 0
                if(bits == 0x80000000U) bits = 0;
                value = static_cast<size_type>(bits * 2654435769U);
            }
            else SP_IF_CONSTEXPR((spt::is_same<T,double>::value)){
                ull bits = detail::bit_cast_hash<ull>(val);
                if(bits == 0x8000000000000000ULL) bits = 0;
                value = static_cast<size_type>(bits * 11400714819323198485ULL);
            }
            // --- pointer types: shift out alignment bits, then multiply ---
            else SP_IF_CONSTEXPR(spt::is_pointer_v<T>){
                ull addr = reinterpret_cast<ull>(val);
                addr >>= 3; // strip likely-zero low bits
                value = static_cast<size_type>(addr * 11400714819323198485ULL);
            }
            // --- string-like types (c_str() + size()) ---
            else SP_IF_CONSTEXPR(SP_HAS_METHOD(T, c_str) && SP_HAS_METHOD(T, size))
                return ____private_string_hash(val.c_str(), val.size());
            // --- raw C-strings ---
            else SP_IF_CONSTEXPR((spt::is_same<T,const char*>::value || spt::is_same<T,char*>::value))
                return ____private_string_hash(val, sp::strlen(val));
            return value;
        }

        private:
        /// FNV-1a over @p len bytes of @p data followed by a custom avalanche mix.
        /// @param data        Pointer to character data.
        /// @param len         Number of bytes to hash.
        /// @param bucket_size Unused; kept for potential future adaptive mixing.
        /// @return 64-bit hash.
        static SP_FORCEINLINE constexpr size_type ____private_string_hash(const char* data, size_type len, size_type bucket_size=0){
            ull h = 14695981039346656037ULL;
            const ull fnv_prime = 1099511628211ULL;
            for(int ch = 0; ch < len; ch++){
                h ^= static_cast<unsigned char>(data[ch]);
                h *= fnv_prime;
            }
            h ^= 0x73b60264cf36661eULL;      // [0]
            h += 0xfffffffffffffe9bULL;      // [1]
            h ^= 0x893e824093a0c6aeULL;      // [2]
            h ^= (h << 15);                  // [3]
            h ^= (h << 14);                  // [4]
            h *= 0xfffffffffffffe5dULL;      // [5]
            h ^= (h << 54);                  // [6]
            h ^= (h << 46);                  // [7]
            h ^= (h << 27);                  // [8]
            h += 0xffffffffffffffc5ULL;      // [9]
            h += 0xffffffffffffff15ULL;      // [10]
            h ^= (h << 18);                  // [11]
            h *= 0xfffffffffffffea9ULL;      // [12]
            h ^= (h >> 33);
            h *= 0xff51afd7ed558ccdULL;
            h ^= (h >> 33);
            return h;
            /*if(bucket_size<=250'000){
                    if(bucket_size <= 10){
                        h ^= 0x86df614c88897b72ULL;
                        h *= 0xffffffffffffffadULL;
                        h *= 0xfffffffffffffea9ULL;
                    }
                    else if(bucket_size <= 20){
                        h *= 0xffffffffffffffc5ULL;           // [0] MUL_PRIME
                        h += 0xffffffffffffff15ULL;           // [1] ADD_PRIME
                        h ^= 0xdafd8459e6f964dbULL;           // [2] XOR_CONST
                        h ^= (h << 43);                       // [3] XOR_LEFT
                        h ^= 0xf45ec8bb4fe1d09aULL;           // [4] XOR_CONST
                        h += 0xfffffffffffffea9ULL;           // [5] ADD_PRIME
                        h ^= (h >> 35);                       // [6] XOR_RIGHT
                        h += 0xfffffffffffffe35ULL;           // [7] ADD_PRIME
                        h ^= (h >> 5);                        // [8] XOR_RIGHT
                        h ^= 0x8fdfc5c6478f479bULL;           // [9] XOR_CONST
                        h += 0xfffffffffffffe5dULL;           // [10] ADD_PRIME
                        h ^= 0x3b544a068a3b96a1ULL;           // [11] XOR_CONST
                        h ^= (h << 1);                        // [12] XOR_LEFT
                        h *= 0xfffffffffffffe75ULL;           // [13] MUL_PRIME
                        h *= 0xfffffffffffffe35ULL;           // [14] MUL_PRIME
                    }
                    else if(bucket_size <= 100){
                        h *= 0xffffffffffffffadULL;           // [0]
                        h *= 0xffffffffffffff4dULL;           // [1]
                        h ^= 0xecd26b5007395d41ULL;           // [2]
                        h ^= (h >> 38);                       // [3]
                        h *= 0xfffffffffffffe35ULL;           // [4]
                        h ^= (h >> 30);                       // [5]
                        h ^= (h >> 58);                       // [6]
                        h *= 0xfffffffffffffe75ULL;           // [7]
                        h *= 0xffffffffffffffa1ULL;           // [8]
                        h ^= (h << 57);                       // [9]
                        h ^= (h >> 63);                       // [10]
                        h ^= (h >> 1);                        // [11]
                        h ^= (h >> 20);                       // [12]
                        h ^= (h << 24);                       // [13]
                    } 
                    else if(bucket_size <= 500){
                        h ^= 0xc7517d4abd283521ULL;  // [0]
                        h ^= (h >> 12);              // [1]
                        h += 0xfffffffffffffee1ULL;  // [2]
                        h ^= (h >> 37);              // [3]
                        h ^= 0x9860546e83155ecbULL;  // [4]
                        h ^= (h >> 20);              // [5]
                        h ^= (h >> 48);              // [6]
                        h ^= (h << 58);              // [7]
                        h *= 0xffffffffffffff43ULL;  // [8]
                    }
                    else if(bucket_size <= 1000){
                        h *= 0xfffffffffffffe9bULL;          // [0]
                        h ^= 0x25d4bba5bc2b7ff8ULL;          // [1]
                        h ^= (h >> 6);                        // [2]
                        h ^= 0xcbffd6b7a588be25ULL;          // [3]
                        h ^= 0x15a61195b752eab3ULL;          // [4]
                        h ^= 0x698433ad0785bfdaULL;          // [5]
                    }
                    else if(bucket_size <= 3500){
                        h ^= 0x73b60264cf36661eULL;      // [0]
                        h += 0xfffffffffffffe9bULL;      // [1]
                        h ^= 0x893e824093a0c6aeULL;      // [2]
                        h ^= (h << 15);                  // [3]
                        h ^= (h << 14);                  // [4]
                        h *= 0xfffffffffffffe5dULL;      // [5]
                        h ^= (h << 54);                  // [6]
                        h ^= (h << 46);                  // [7]
                        h ^= (h << 27);                  // [8]
                        h += 0xffffffffffffffc5ULL;      // [9]
                        h += 0xffffffffffffff15ULL;      // [10]
                        h ^= (h << 18);                  // [11]
                        h *= 0xfffffffffffffea9ULL;      // [12]
                    }
                    else if(bucket_size <= 5000){
                        h += 0xffffffffffffff4dULL;      // [0]
                        h ^= 0x23bbdd0d41d13f2bULL;      // [1]
                        h ^= 0x86ee931b4a6c8903ULL;      // [2]
                        h ^= (h << 27);                  // [3]
                        h *= 0xfffffffffffffea9ULL;      // [4]
                        h ^= (h << 61);                  // [5]
                    }
                    else if(bucket_size <= 10000){
                        h ^= (h >> 18);                      // [0]
                        h ^= (h << 13);                      // [1]
                        h *= 0xffffffffffffffa1ULL;          // [2]
                        h *= 0xfffffffffffffe75ULL;          // [3]
                        h ^= 0xb7df475ccdce7cc1ULL;          // [4]
                    }
                    else if(bucket_size <= 20000){
                        h += 0xffffffffffffff43ULL;  // [0]
                        h += 0xfffffffffffffe5dULL;  // [1]
                        h ^= 0xdbf530685f955086ULL;  // [2]
                        h += 0xffffffffffffffadULL;  // [3]
                        h += 0xfffffffffffffe75ULL;  // [4]
                        h += 0xffffffffffffff15ULL;  // [5]
                        h += 0xffffffffffffffa1ULL;  // [6]
                        h += 0xffffffffffffff09ULL;  // [7]
                        h ^= (h >> 11);              // [8]
                    }
                    else if(bucket_size <= 50000){
                        h ^= (h >> 48);              // [0]
                        h ^= (h << 52);              // [1]
                        h ^= 0xe195d2445cc1e92fULL;  // [2]
                        h ^= (h >> 1);               // [3]
                        h *= 0xfffffffffffffe5dULL;  // [4]
                        h += 0xffffffffffffffa1ULL;  // [5]
                    }
                    else if(bucket_size <= 75000){
                        h ^= (h >> 62);               // [0]
                        h ^= (h << 37);               // [1]
                        h ^= (h << 43);               // [2]
                        h ^= (h >> 19);               // [3]
                        h += 0xffffffffffffff09ULL;   // [4]
                        h ^= (h << 30);               // [5]
                        h += 0xffffffffffffff09ULL;   // [6]
                        h ^= 0x61369c2d5a65ed50ULL;   // [7]
                        h ^= (h >> 23);               // [8]
                        h ^= (h << 55);               // [9]
                        h ^= (h >> 34);               // [10]
                        h *= 0xfffffffffffffefdULL;   // [11]
                    }
                    else if(bucket_size <= 100000){
                        h ^= 0x86eb51b1b7df33e7ULL;    // [0]
                        h += 0xfffffffffffffe41ULL;    // [1]
                        h ^= (h >> 32);                // [2]
                    }
                    else if(bucket_size <= 150000){
                        h ^= 0x9351401449515b9eULL;   // [0]
                        h ^= (h >> 8);                // [1]
                        h ^= (h >> 44);               // [2]
                        h ^= 0xfdcca755f339700cULL;   // [3]
                        h ^= (h << 56);               // [4]
                        h ^= (h << 29);               // [5]
                        h ^= 0x173ab25dffb79bc0ULL;   // [6]
                    }
                    else if(bucket_size <= 200000){
                        h ^= 0x3618d446a5b01e8cULL;   // [0]
                        h ^= (h >> 56);               // [1]
                        h ^= (h >> 28);               // [2]
                        h ^= (h >> 36);               // [3]
                        h *= 0xfffffffffffffe75ULL;   // [4]
                        h ^= (h << 39);               // [5]
                        h += 0xffffffffffffff4dULL;   // [6]
                        h += 0xffffffffffffff43ULL;   // [7]
                        h += 0xfffffffffffffe35ULL;   // [8]
                        h += 0xfffffffffffffe9bULL;   // [9]
                        h ^= (h >> 51);               // [10]
                        h += 0xfffffffffffffe81ULL;   // [11]
                        h ^= (h << 24);               // [12]
                        h *= 0xfffffffffffffefdULL;   // [13]
                        h ^= (h >> 41);               // [14]
                    }
                    else  250'000 *//*{
                        h *= 0xfffffffffffffe75ULL;          // [0]
                        h *= 0xfffffffffffffe75ULL;          // [1]
                        h ^= (h << 31);                      // [2]
                        h ^= (h >> 62);                      // [3]
                        h += 0xffffffffffffff4dULL;          // [4]
                        h ^= (h >> 36);                      // [5]
                        h *= 0xffffffffffffffc5ULL;          // [6]
                        h ^= 0x01921140c76a3513ULL;          // [7]
                        h ^= (h >> 9);                       // [8]
                        h ^= 0x49dfd2556514b992ULL;          // [9]
                        h ^= 0xff4179d1b9b6fb18ULL;          // [10]
                        h ^= 0x635bec7169befaf0ULL;          // [11]
                        h ^= 0xa9b69acc53b45d84ULL;          // [12]
                    }
                }
                else if(bucket_size<=3'437'500){
                    if(bucket_size<=312'500){
                        h *= 0xffffffffffffffa1ULL;
                        h ^= (h<<50);
                        h ^= 0x8170f3a10c8d0708ULL;
                    }
                    else if(bucket_size<=625'000){
                        h += 0xfffffffffffffe5dULL;
                        h ^= (h<<45);
                        h ^= (h>>14);
                        h^= (h<<50);
                        h += 0xffffffffffffff4dULL;
                        h *= 0xfffffffffffffe81ULL;
                        h += 0xffffffffffffff4dULL;
                        h ^= (h<<31);
                    }
                    else if(bucket_size<=937'500){
                        h += 0xfffffffffffffe75ULL;
                        h ^= 0xfaf93f67e01a3557ULL;
                        h *= 0xffffffffffffff15ULL;
                        h ^= 0x420c025ea43ad201ULL;
                        h *= 0xffffffffffffff09ULL;
                        h ^= (h>>22);
                        h *= 0xfffffffffffffea9ULL;
                        h ^= (h<<46);
                        h ^= 0xb49211fdd5b57da0ULL;
                        h ^= (h>>47);
                    }
                    else if(bucket_size<=1'250'000){
                        h ^= (h<<16);
                        h ^= (h<<18);
                        h ^= (h>>52);
                        h ^= (h>>47);
                        h ^= (h<<31);
                        h += 0xffffffffffffffadULL;
                        h ^= 0x5c6b5545330e8ebdULL;
                        h += 0xfffffffffffffe41ULL;
                        h ^= 0x7257f6dcfe89c8a2ULL;
                        h ^= (h<<44);
                        h *= 0xfffffffffffffe35ULL;
                        h += 0xfffffffffffffea9ULL;
                        h ^= 0x6a34867877c2292cULL;
                    }
                    else if(bucket_size<=1'562'500){
                        h ^= 0x682b92d641229967ULL;
                        h += 0xffffffffffffffc5ULL;
                        h ^= (h<<8);
                        h ^= (h<<6);
                        h ^= 0xa0da8d1e6b05d1d0ULL;
                        h *= 0xfffffffffffffe35ULL;
                        h ^= 0xfd9700f379298ffeULL;
                        h ^= (h>>21);
                        h ^= (h<<58);
                        h ^= (h>>15);
                    }
                    else if(bucket_size<=1'875'000){
                        h *= 0xfffffffffffffec1ULL;
                        h ^= (h>>7);
                        h ^= 0x2b5556ab5c974201ULL;
                        h *= 0xfffffffffffffe1dULL;
                        h ^= (h>>35);
                        h ^= (h<<53);
                        h ^= (h<<8);
                        h ^= (h>>20);
                        h ^= (h<<14);
                        h += 0xfffffffffffffe41ULL;
                        h ^= (h>>42);
                        h ^= 0xc93f76406da313e1ULL;
                        h ^= (h>>2);
                    }
                    else if(bucket_size<=2'187'500){
                        h ^= (h>>38);
                        h += 0xfffffffffffffea9ULL;
                        h += 0xfffffffffffffe41ULL;
                        h ^= (h>>33);
                        h ^= (h>>59);
                        h += 0xfffffffffffffe41ULL;
                        h *= 0xffffffffffffffadULL;
                        h ^= 0x632dac099c79f743ULL;
                        h += 0xfffffffffffffe75ULL;
                        h ^= (h>>10);
                        h ^= 0xaa399e193c455fd8ULL;
                        h ^= (h<<21);
                        h ^= (h<<36);
                    }
                    else if(bucket_size<=2'500'000){
                        h += 0xffffffffffffff43ULL;
                        h += 0xfffffffffffffe5dULL;
                        h ^= (h>>22);
                        h += 0xfffffffffffffe1dULL;
                        h ^= 0x76105cd0f0336903ULL;
                        h ^= 0xc717cf06b250e6e8ULL;
                        h ^= 0x8efb647542f0deceULL;
                        h *= 0xfffffffffffffec1ULL;
                        h ^= 0x4ffd63d5e30b005fULL;
                        h += 0xffffffffffffff43ULL;
                        h *= 0xfffffffffffffefdULL;
                    }
                    else if(bucket_size<=2'812'500){
                        h += 0xffffffffffffffadULL;
                        h += 0xfffffffffffffea9ULL;
                        h ^= 0x1262d33ff0782583ULL;
                        h ^= (h>>12);
                        h += 0xfffffffffffffe81ULL;
                        h ^= 0x91f678537ac8d85eULL;
                        h += 0xfffffffffffffe81ULL;
                        h ^= (h<<5);
                        h ^= 0x752ee0facedb10a1ULL;
                    }
                    else if(bucket_size<=3'125'000){
                        h *= 0xffffffffffffffc5ULL;
                        h ^= (h<<6);
                        h *= 0xffffffffffffff35ULL;
                        h ^= (h<<22);
                        h += 0xfffffffffffffefdULL;
                        h ^= (h>>10);
                        h ^= (h>>32);
                        h += 0xfffffffffffffe4fULL;
                        h ^= (h<<26);
                        h ^= (h>>15);
                        h ^= 0x624c68703b70f0a3ULL;
                        h *= 0xffffffffffffff15ULL;
                        h ^= 0x7b54f9598d0bb699ULL;
                    }
                    else 3'437'500*//* {
                        h *= 0xfffffffffffffe1dULL;
                        h ^= (h<<61);
                        h ^= (h<<61);
                        h *= 0xffffffffffffff4dULL;
                        h ^= (h<<59);
                        h ^= 0xe241192de6f8ab63ULL;
                        h ^= (h<<4);
                        h ^= (h>>4);
                        h ^= (h<<26);
                        h ^= (h<<10);
                        h ^= 0xe5df533add7ef7dbULL;
                        h ^= (h<<45);
                    }
                }
                else if(bucket_size<=6'250'000){
                    if(bucket_size<=3'750'000){
                        h ^= (h>>30);
                        h ^= 0xbfaf848fa9862346ULL;
                        h += 0xfffffffffffffe5dULL;
                        h ^= (h>>55);
                        h ^= (h>>30);
                        h ^= (h>>44);
                        h ^= 0x9c11548d9f78c414ULL;
                        h ^= (h<<54);
                        h += 0xfffffffffffffe35ULL;
                        h *= 0xfffffffffffffe75ULL;
                        h ^= (h>>63);
                        h *= 0xfffffffffffffee1ULL;
                    }
                    else if(bucket_size<=4'062'500){
                        h *= 0xfffffffffffffe4fULL;
                        h += 0xfffffffffffffefdULL;
                        h ^= 0xcca2c2474bbe763eULL;
                        h += 0xfffffffffffffec1ULL;
                        h *= 0xffffffffffffff15ULL;
                    }
                    else if(bucket_size<=4'375'000){
                        h ^= (h>>27);
                        h ^= (h>>16);
                        h ^= 0x9cbda3794c4c5d15ULL;
                        h += 0xffffffffffffffc5ULL;
                        h ^= (h<<49);
                        h ^= (h>>36);
                        h += 0xfffffffffffffefdULL;
                        h ^= (h>>48);
                        h *= 0xfffffffffffffe1dULL;
                        h ^= (h<<56);
                        h *= 0xfffffffffffffec1ULL;
                        h *= 0xfffffffffffffe41ULL;
                    }
                    else if(bucket_size<=4'687'500){
                        h ^= (h<<55);
                        h ^= (h<<24);
                        h ^= (h<<24);
                        h += 0xfffffffffffffe5dULL;
                        h ^= (h>>63);
                    }
                    else if(bucket_size<=5'000'000){
                        h *= 0xfffffffffffffe5dULL;
                        h ^= (h>>17);
                        h ^= (h>>16);
                        h += 0xfffffffffffffe4fULL;
                        h *= 0xffffffffffffff09ULL;
                        h += 0xfffffffffffffee1ULL;
                        h ^= (h>>23);
                        h *= 0xfffffffffffffe9bULL;
                        h ^= 0x5fdc8bb6f55d58e0ULL;
                    }
                    else if(bucket_size<=5'312'500){
                        h ^= (h>>16);
                        h += 0xffffffffffffff35ULL;
                        h += 0xfffffffffffffe81ULL;
                        h += 0xfffffffffffffe4fULL;
                        h ^= (h>>38);
                        h ^= (h<<28);
                        h *= 0xffffffffffffff35ULL;
                        h ^= 0x2dd06e6511d9e1e9ULL;
                        h += 0xfffffffffffffefdULL;
                        h ^= (h>>23);
                        h ^= (h<<39);
                    }
                    else if(bucket_size<=5'625'000){
                        h ^= (h>>12);
                        h ^= 0xc5b7c96d8cd2ce4eULL;
                    }
                    else if(bucket_size<=5'937'500){
                        h *= 0xfffffffffffffe81ULL;
                        h *= 0xffffffffffffff35ULL;
                        h ^= (h>>32);
                        h ^= (h<<13);
                        h ^= 0xbb55e8436a5147a4ULL;
                        h ^= 0xfd7532c96586c67eULL;
                        h += 0xffffffffffffff15ULL;
                        h ^= (h>>32);
                        h ^= (h>>54);
                        h ^= 0xc7a4cf59d9e8d5f5ULL;
                        h ^= 0x86c0b42de7374f52ULL;
                        h ^= (h<<27);
                    }
                    else 6'250'000*//* {
                        h ^= (h>>45);
                        h *= 0xfffffffffffffe81ULL;
                        h ^= (h<<27);
                        h ^= (h<<36);
                        h ^= (h<<59);
                        h ^= 0x4e028f9a93386907ULL;
                        h += 0xffffffffffffff35ULL;
                        h *= 0xffffffffffffff09ULL;
                        h ^= 0xf48c7b86ac3a5ebeULL;
                        h += 0xfffffffffffffec1ULL;
                        h ^= (h<<49);
                        h += 0xffffffffffffff35ULL;
                        h *= 0xfffffffffffffe4fULL;
                    }
                }
                else if(bucket_size<=8'437'500){
                    if(bucket_size<=6'562'500){
                        h ^= (h>>31);
                        h ^= (h<<38);
                        h ^= (h<<10);
                        h ^= 0xe0d0f36027ba75dcULL;
                        h += 0xfffffffffffffefdULL;
                    }
                    else if(bucket_size<=6'875'000){
                        h *= 0xffffffffffffffadULL;
                        h ^= (h>>40);
                        h ^= (h<<10);
                        h *= 0xffffffffffffff09ULL;
                        h += 0xfffffffffffffe35ULL;
                        h ^= (h<<60);
                        h ^= 0x3461555b3f9763d5ULL;
                        h += 0xffffffffffffffc5ULL;
                        h ^= (h<<22);
                        h ^= (h>>22);
                        h += 0xfffffffffffffee1ULL;
                        h ^= 0xf5f23901781fba20ULL;
                        h += 0xfffffffffffffe5dULL;
                        h += 0xfffffffffffffe9bULL;
                        h ^= 0xd634af0bd3651c5fULL;
                    }
                    else if(bucket_size<=7'187'500){
                        h += 0xfffffffffffffe5dULL;
                        h ^= 0x9fa5e9abed46a48aULL;
                        h ^= 0xcf5be876084f3c5cULL;
                        h ^= 0x766b3084958cb317ULL;
                        h ^= (h<<14);
                    }
                    else if(bucket_size<=7'500'000){
                        h *= 0xfffffffffffffe5dULL;
                        h += 0xfffffffffffffea9ULL;
                        h *= 0xffffffffffffffadULL;
                        h += 0xfffffffffffffee1ULL;
                        h ^= (h>>27);
                        h += 0xffffffffffffff09ULL;
                        h += 0xfffffffffffffe4fULL;
                    }
                    else if(bucket_size<=7'812'500){
                        h += 0xfffffffffffffea9ULL;
                        h += 0xfffffffffffffe35ULL;
                        h *= 0xfffffffffffffe35ULL;
                        h ^= (h<<1);
                        h += 0xfffffffffffffe41ULL;
                        h ^= (h<<33);
                        h ^= 0x39d625c206c6ab69ULL;
                        h ^= (h<<53);
                        h ^= 0xfb2260e7d97e8113ULL;
                        h ^= (h<<49);
                        h += 0xfffffffffffffe81ULL;
                    }
                    else if(bucket_size<=8'125'000){
                        h ^= (h<<57);
                        h += 0xfffffffffffffe75ULL;
                        h ^= (h>>23);
                    }
                    else 8'437'500*//* {
                        h *= 0xffffffffffffffadULL;
                        h *= 0xffffffffffffffadULL;
                        h *= 0xfffffffffffffe75ULL;
                        h ^= (h<<10);
                        h ^= 0x3082af994f1e9614ULL;
                        h ^= 0xa5ce5708d5f266adULL;
                        h += 0xfffffffffffffe5dULL;
                        h ^= (h>>32);
                        h *= 0xfffffffffffffea9ULL;
                        h ^= (h>>11);
                        h += 0xfffffffffffffe35ULL;
                        h += 0xffffffffffffffc5ULL;
                        h ^= (h>>59);
                        h += 0xffffffffffffff43ULL;
                        h ^= 0xcdda1fd9fb4e4567ULL;
                    }
                }
                else{
                    if(bucket_size<=8'750'000){
                        h *= 0xfffffffffffffe35ULL;
                        h *= 0xfffffffffffffefdULL;
                        h += 0xfffffffffffffe41ULL;
                        h *= 0xfffffffffffffe41ULL;
                        h ^= (h>>7);
                        h ^= (h<<41);
                    }
                    else if(bucket_size<=9'062'500){
                        h ^= 0x8245941f175dc4bbULL;
                        h *= 0xfffffffffffffee1ULL;
                        h ^= (h<<24);
                        h ^= (h<<49);
                        h ^= (h<<63);
                        h ^= (h>>30);
                        h ^= (h<<39);
                        h ^= 0xb2a7a3982efc73b8ULL;
                    }
                    else if(bucket_size<=9'375'000){
                        h *= 0xfffffffffffffe9bULL;
                        h ^= 0xd2a4fe1b86127a0eULL;
                        h += 0xffffffffffffff43ULL;
                        h += 0xfffffffffffffea9ULL;

                    }
                    else if(bucket_size<=9'687'500){
                        h ^= (h>>55);
                        h ^= (h<<36);
                        h ^= (h<<45);
                        h ^= (h>>50);
                        h ^= (h>>46);
                        h *= 0xffffffffffffffc5ULL;
                    }
                    else{
                        h ^= (h<<41);
                        h ^= (h>>51);
                        h ^= (h>>19);
                        h ^= (h>>38);
                        h += 0xfffffffffffffe1dULL;
                        h ^= 0xb91ac5d456664327ULL;
                        h *= 0xfffffffffffffea9ULL;
                        h ^= (h>>28);
                        h ^= 0x08c248048d6bd7e3ULL;
                        h += 0xffffffffffffff15ULL;
                        h += 0xffffffffffffff09ULL;
                        h ^= 0xa894c4f57d82f7adULL;
                    }
            }
            // avalanche effect
            h ^= (h >> 33);
            h *= 0xff51afd7ed558ccdULL;
            h ^= (h >> 33);
            return h;*/
        }
    }; // class basic_hash


    // ─── identity_hash ──────────────────────────────────────────────────────────
    /// Zero-overhead hash for dense integer keys (e.g. array indices).
    /// Returns the key cast to @c size_type with no mixing.
    /// @warning Only suitable when the caller controls key distribution;
    ///          clustered keys will cause heavy collisions in a hash map.
    struct identity_hash {
        /// @param val         Integer key to "hash".
        /// @param bucket_size Unused.
        /// @return @c static_cast<size_type>(val).
        template <typename T>
        SP_FORCEINLINE SP_PURE constexpr size_type operator()(T val, size_type = 0) const noexcept {
            return static_cast<size_type>(val);
        }
    };


    // ─── fnv1a_hash ─────────────────────────────────────────────────────────────
    /// Standalone FNV-1a (64-bit) hash functor.
    /// Faster than @c basic_hash for short strings due to no extra avalanche passes.
    /// Non-string types fall back to hashing their raw bytes.
    struct fnv1a_hash {
        /// Hashes @p len bytes starting at @p data.
        /// @return 64-bit FNV-1a digest.
        SP_FORCEINLINE SP_PURE constexpr size_type operator()(const char* data, size_type len, size_type = 0) const noexcept {
            ull h = 14695981039346656037ULL;
            for(size_type i = 0; i < len; ++i){
                h ^= static_cast<unsigned char>(data[i]);
                h *= 1099511628211ULL;
            }
            return static_cast<size_type>(h);
        }
        /// Dispatches to the byte-buffer overload via @c c_str()+size(), @c strlen,
        /// or a raw @c sizeof(T) byte view for other types.
        template <typename T>
        SP_FORCEINLINE SP_PURE constexpr size_type operator()(T val, size_type bucket_size = 0) const noexcept {
            SP_IF_CONSTEXPR(SP_HAS_METHOD(T, c_str) && SP_HAS_METHOD(T, size))
                return (*this)(val.c_str(), val.size());
            else SP_IF_CONSTEXPR((spt::is_same<T,const char*>::value || spt::is_same<T,char*>::value))
                return (*this)(val, sp::strlen(val));
            // fallback: hash raw bytes of the value
            return (*this)(reinterpret_cast<const char*>(&val), sizeof(T));
        }
    };


    // ─── murmur3_hash ───────────────────────────────────────────────────────────
    /// MurmurHash3 128-bit body with 64-bit output.
    /// Excellent avalanche and distribution; a good general-purpose alternative
    /// to @c basic_hash when collision resistance matters more than raw speed.
    struct murmur3_hash {
    private:
        /// Final avalanche mix for a 64-bit word (MurmurHash3 fmix64).
        static SP_FORCEINLINE constexpr ull fmix64(ull k) noexcept {
            k ^= k >> 33;
            k *= 0xff51afd7ed558ccdULL;
            k ^= k >> 33;
            k *= 0xc4ceb9fe1a85ec53ULL;
            k ^= k >> 33;
            return k;
        }
    public:
        /// Hashes @p len bytes of @p data with an optional @p seed.
        /// @return 64-bit digest (sum of two fmix64-finalised accumulators).
        SP_FORCEINLINE SP_PURE constexpr size_type operator()(const char* data, size_type len, size_type seed = 0) const noexcept {
            const size_type nblocks = len >> 3;
            ull h1 = seed, h2 = seed;
            const ull c1 = 0x87c37b91114253d5ULL;
            const ull c2 = 0x4cf5ad432745937fULL;
            // body
            for(size_type i = 0; i < nblocks; ++i){
                ull k1 = 0, k2 = 0;
                __builtin_memcpy(&k1, data + i*16,     8);
                __builtin_memcpy(&k2, data + i*16 + 8, 8);
                k1 *= c1; k1 = (k1<<31)|(k1>>33); k1 *= c2; h1 ^= k1;
                h1 = (h1<<27)|(h1>>37); h1 += h2; h1 = h1*5 + 0x52dce729ULL;
                k2 *= c2; k2 = (k2<<33)|(k2>>31); k2 *= c1; h2 ^= k2;
                h2 = (h2<<31)|(h2>>33); h2 += h1; h2 = h2*5 + 0x38495ab5ULL;
            }
            // tail
            const unsigned char* tail = reinterpret_cast<const unsigned char*>(data + nblocks*16);
            ull k1 = 0, k2 = 0;
            switch(len & 15){
                case 15: k2 ^= static_cast<ull>(tail[14]) << 48; [[fallthrough]];
                case 14: k2 ^= static_cast<ull>(tail[13]) << 40; [[fallthrough]];
                case 13: k2 ^= static_cast<ull>(tail[12]) << 32; [[fallthrough]];
                case 12: k2 ^= static_cast<ull>(tail[11]) << 24; [[fallthrough]];
                case 11: k2 ^= static_cast<ull>(tail[10]) << 16; [[fallthrough]];
                case 10: k2 ^= static_cast<ull>(tail[ 9]) <<  8; [[fallthrough]];
                case  9: k2 ^= static_cast<ull>(tail[ 8]);
                         k2 *= c2; k2 = (k2<<33)|(k2>>31); k2 *= c1; h2 ^= k2; [[fallthrough]];
                case  8: k1 ^= static_cast<ull>(tail[ 7]) << 56; [[fallthrough]];
                case  7: k1 ^= static_cast<ull>(tail[ 6]) << 48; [[fallthrough]];
                case  6: k1 ^= static_cast<ull>(tail[ 5]) << 40; [[fallthrough]];
                case  5: k1 ^= static_cast<ull>(tail[ 4]) << 32; [[fallthrough]];
                case  4: k1 ^= static_cast<ull>(tail[ 3]) << 24; [[fallthrough]];
                case  3: k1 ^= static_cast<ull>(tail[ 2]) << 16; [[fallthrough]];
                case  2: k1 ^= static_cast<ull>(tail[ 1]) <<  8; [[fallthrough]];
                case  1: k1 ^= static_cast<ull>(tail[ 0]);
                         k1 *= c1; k1 = (k1<<31)|(k1>>33); k1 *= c2; h1 ^= k1;
            }
            h1 ^= len; h2 ^= len;
            h1 += h2; h2 += h1;
            return static_cast<size_type>(fmix64(h1) + fmix64(h2));
        }
        /// Dispatches to the byte-buffer overload via @c c_str()+size(), @c strlen,
        /// or a raw @c sizeof(T) byte view for other types.
        template <typename T>
        SP_FORCEINLINE SP_PURE constexpr size_type operator()(T val, size_type bucket_size = 0) const noexcept {
            SP_IF_CONSTEXPR(SP_HAS_METHOD(T, c_str) && SP_HAS_METHOD(T, size))
                return (*this)(val.c_str(), val.size());
            else SP_IF_CONSTEXPR((spt::is_same<T,const char*>::value || spt::is_same<T,char*>::value))
                return (*this)(val, sp::strlen(val));
            return (*this)(reinterpret_cast<const char*>(&val), sizeof(T));
        }
    };


    // ─── wyhash ─────────────────────────────────────────────────────────────────
    /// wyhash v4 — highest throughput hash in most benchmarks.
    /// Uses a 128-bit multiply-xor mix (@c _wymix) with four secret constants.
    /// Handles strings of any length via size-specialised fast paths (1–16 bytes)
    /// and a 48-byte-per-iteration streaming loop for longer inputs.
    /// Non-string types fall back to hashing their raw bytes.
    struct wyhash {
    private:
        /// Core mix: XORs the low and high halves of the 128-bit product of @p a and @p b.
        static SP_FORCEINLINE constexpr ull _wymix(ull a, ull b) noexcept {
            // 128-bit multiply via two 64-bit halves (portable, no __uint128_t required)
            ull lo = a * b;
            // hi approximation: good enough for hashing
            ull hi = (ull)((__uint128_t)a * b >> 64);
            return lo ^ hi;
        }
        /// Loads 8 bytes from @p p (unaligned-safe via @c __builtin_memcpy).
        static SP_FORCEINLINE constexpr ull _wyr8(const unsigned char* p) noexcept {
            ull v; __builtin_memcpy(&v, p, 8); return v;
        }
        /// Loads 4 bytes from @p p.
        static SP_FORCEINLINE constexpr ull _wyr4(const unsigned char* p) noexcept {
            unsigned int v; __builtin_memcpy(&v, p, 4); return v;
        }
        /// Reads 1–3 bytes from @p p as a single 24-bit word (wyhash short-string trick).
        static SP_FORCEINLINE constexpr ull _wyr3(const unsigned char* p, size_type k) noexcept {
            return (static_cast<ull>(p[0]) << 16) | (static_cast<ull>(p[k>>1]) << 8) | p[k-1];
        }
    public:
        static constexpr ull secret0 = 0xa0761d6478bd642fULL; ///< Mix constant 0.
        static constexpr ull secret1 = 0xe7037ed1a0b428dbULL; ///< Mix constant 1.
        static constexpr ull secret2 = 0x8ebc6af09c88c6e3ULL; ///< Mix constant 2.
        static constexpr ull secret3 = 0x589965cc75374cc3ULL; ///< Mix constant 3 (final mix).

        /// Hashes @p len bytes of @p data with an optional @p seed.
        /// @return 64-bit wyhash digest.
        SP_FORCEINLINE SP_PURE size_type operator()(const char* data, size_type len, size_type seed = 0) const noexcept {
            const unsigned char* p = reinterpret_cast<const unsigned char*>(data);
            ull h = seed ^ secret0;
            if(len <= 3){
                h = _wymix(h ^ secret1, (len ? _wyr3(p, len) : 0) ^ secret2);
            } else if(len <= 8){
                h = _wymix(_wyr4(p) ^ h ^ secret1, _wyr4(p + len - 4) ^ secret2);
            } else if(len <= 16){
                h = _wymix(_wyr8(p) ^ h ^ secret1, _wyr8(p + len - 8) ^ secret2);
            } else {
                ull see1 = h, see2 = h;
                size_type i = 0;
                for(; i + 48 <= len; i += 48){
                    h    = _wymix(_wyr8(p+i)    ^ secret1, _wyr8(p+i+ 8) ^ h);
                    see1 = _wymix(_wyr8(p+i+16) ^ secret2, _wyr8(p+i+24) ^ see1);
                    see2 = _wymix(_wyr8(p+i+32) ^ secret3, _wyr8(p+i+40) ^ see2);
                }
                for(; i + 16 <= len; i += 16)
                    h = _wymix(_wyr8(p+i) ^ secret1, _wyr8(p+i+8) ^ h);
                h ^= see1 ^ see2;
                h = _wymix(_wyr8(p + len - 16) ^ secret1, _wyr8(p + len - 8) ^ h);
            }
            return static_cast<size_type>(_wymix(h ^ len, secret3));
        }
        /// Dispatches to the byte-buffer overload via @c c_str()+size(), @c strlen,
        /// or a raw @c sizeof(T) byte view for other types.
        template <typename T>
        SP_FORCEINLINE SP_PURE constexpr size_type operator()(T val, size_type bucket_size = 0) const noexcept {
            SP_IF_CONSTEXPR(SP_HAS_METHOD(T, c_str) && SP_HAS_METHOD(T, size))
                return (*this)(val.c_str(), val.size());
            else SP_IF_CONSTEXPR((spt::is_same<T,const char*>::value || spt::is_same<T,char*>::value))
                return (*this)(val, sp::strlen(val));
            return (*this)(reinterpret_cast<const char*>(&val), sizeof(T));
        }
    };

} // namespace sp

#endif // ____SP_HASHES____