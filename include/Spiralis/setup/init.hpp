#ifndef ____SPIRALIS_INIT____
#define ____SPIRALIS_INIT____
#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #include <apiset.h>
    #include <minwindef.h>
    #include <sysinfoapi.h>
#elif defined(__unix__) || defined(__APPLE__)
    #include <unistd.h>
#endif

// ===========================// ===========================// ===========================// ===========================
// THREE UNDERSCORES: SYSTEM DETECTION
// ---------------------------------------------------------------------------------------------------------------------

// must be little-endian system
static_assert([]() constexpr {
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
    return __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__;
#elif defined(_MSC_VER)
    return true;
#else
    #error "Unable to determine endianness at compile-time"
#endif
}(), "Big-endian is not supported.");

#ifdef __cplusplus
    #if __cplusplus <= 201103L
        #define ___SP_CPP_VER___ 11
    #elif __cplusplus <= 201402L
        #define ___SP_CPP_VER___ 14
    #elif __cplusplus <= 201703L
        #define ___SP_CPP_VER___ 17
    #elif __cplusplus <= 202002L
        #define ___SP_CPP_VER___ 20
    #elif __cplusplus <= 202302L
        #define ___SP_CPP_VER___ 23
    #else
        #define ___SP_CPP_VER___ 26
    #endif
#else
    #error "Modern C++ not detected."
#endif

#if ___SP_CPP_VER___ < 17
    #error "Spiral-cpp currently requires C++17 or higher."
#endif

// Compiler detection
#if defined(__clang__)
    #define ___SP_DETECTED_COMPILER___ clang
#elif defined(__GNUC__)
    #define ___SP_DETECTED_COMPILER___ gcc
#elif defined(_MSC_VER)
    #define ___SP_DETECTED_COMPILER___ msvc
#else
    #define ___SP_DETECTED_COMPILER___ other
#endif

// SIMD detection
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #if defined(__AVX512F__)
        #define ___SP_SIMD_LEVEL___ 5  // AVX-512
    #elif defined(__AVX2__)
        #define ___SP_SIMD_LEVEL___ 4  // AVX2
    #elif defined(__AVX__)
        #define ___SP_SIMD_LEVEL___ 3  // AVX
    #elif defined(__SSE4_1__)
        #define ___SP_SIMD_LEVEL___ 2  // SSE4.1
    #else
        #define ___SP_SIMD_LEVEL___ 1  // SSE/SSE2
    #endif
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(_M_ARM)
    #if defined(__ARM_FEATURE_SME)
        #define ___SP_SIMD_LEVEL___ 12 // ARM SME
    #elif defined(__ARM_FEATURE_SVE)
        #define ___SP_SIMD_LEVEL___ 11 // ARM SVE
    #else
        #define ___SP_SIMD_LEVEL___ 10 // NEON
    #endif
#endif



// ===========================// ===========================// ===========================// ===========================
// TWO UNDERSCORES: COMPILE-TIME CONFIGURATIONS
// ---------------------------------------------------------------------------------------------------------------------
#ifndef __SP_LIKELY__
    #define __SP_LIKELY__ 1
#endif
#ifndef __SP_UNROLL_LOOPS__
    #define __SP_UNROLL_LOOPS__ 0
#endif
#ifndef __SP_DEFAULT_MAP_TRAITS__
    #define __SP_DEFAULT_MAP_TRAITS__ 1, 75, sp::basic_hash, sp::allocator
#endif
#ifndef __SP_DEFAULT_SAFETY_LEVEL__
    #define __SP_DEFAULT_SAFETY_LEVEL__ 1
#endif
#ifndef __SP_SIZE_TYPE__
    #define __SP_SIZE_TYPE__ ull
#endif
#ifndef __SP_IO_BUFFER_SIZE__
    #define __SP_IO_BUFFER_SIZE__ 32768 // 1 << 15
#endif

// Application of configurations
#if __SP_LIKELY__ == 1
    #define _SP_LIKELY_   [[likely]]
    #define _SP_UNLIKELY_ [[unlikely]]
#else
    #define _SP_LIKELY_
    #define _SP_UNLIKELY_
#endif

#if __SP_UNROLL_LOOPS__ == 1
    #define IF_UNROLL(...)   __VA_ARGS__
    #define IF_NO_UNROLL(...)
#else
    #define IF_UNROLL(...)
    #define IF_NO_UNROLL(...) __VA_ARGS__
#endif



// ===========================// ===========================// ===========================// ===========================
// ONE UNDERSCORE: INTERNAL HELPERS
// ---------------------------------------------------------------------------------------------------------------------

// ===========================
// Manual loop unrolling
#ifndef _SP_APPLY_UNROLLED_
#define _SP_APPLY_UNROLLED_(loop_until, operation) \
    do { \
        IF_UNROLL( ull __i = 0; for (; __i + 3 < loop_until; __i += 4) { \
            { ull i = __i; operation; } { ull i = __i + 1; operation; } \
            { ull i = __i + 2; operation; } { ull i = __i + 3; operation; } \
        } while (__i < loop_until) { ull i = __i; operation; __i++; } ) \
        IF_NO_UNROLL( for (ull i = 0; i < loop_until; i++) { operation; } ) \
    } while (0)
#endif

#ifndef _SP_EXPLICIT_UNROLLED_
#define _SP_EXPLICIT_UNROLLED_(loop_var, start_value, loop_until, operation) \
    do { \
        IF_UNROLL( ull __i = start_value; for (; __i + 3 < loop_until; __i += 4) { \
            { ull loop_var = __i; operation; } { ull loop_var = __i + 1; operation; } \
            { ull loop_var = __i + 2; operation; } { ull loop_var = __i + 3; operation; } \
        } while (__i < loop_until) { ull loop_var = __i; operation; __i++; } ) \
        IF_NO_UNROLL( for (ull loop_var = start_value; loop_var < loop_until; loop_var++) { operation; } ) \
    } while (0)
#endif

// Prefetching
#if defined(__GNUC__) || defined(__clang__)
    #define _SP_PREFETCH_(addr, rw, locality) __builtin_prefetch(addr, rw, locality)
#elif defined(_MSC_VER)
    #include <immintrin.h>
    #define _SP_PREFETCH_(addr, rw, locality) _mm_prefetch((const char*)(addr), _MM_HINT_T0)
#else
    #define _SP_PREFETCH_(addr, rw, locality)
#endif

// Alignment
#if defined(__clang__) || defined(__GNUC__)
    #define _SP_ASSUME_ALIGNED_(ptr, alignment) \
        ((decltype(ptr))__builtin_assume_aligned((ptr), (alignment)))
#elif defined(_MSC_VER)
    #define _SP_ASSUME_ALIGNED_(ptr, alignment) \
        (__assume(((uintptr_t)(ptr) & ((alignment) - 1)) == 0), (ptr))
#else
    #define _SP_ASSUME_ALIGNED_(ptr, alignment) (ptr)
#endif

// IO Access
#define _SP_GRANT_IO_ACCESS_ \
    friend class ::sp::IO; \
    template <short __t, template <typename> typename __Alloc, bool __u> friend class string_impl; \
    template <typename __t, typename __U> friend struct spt::has_getSpiralMessage; \
    template <typename __t, typename __U> friend struct spt::has_getSpiralBinary;



// ===========================// ===========================// ===========================// ===========================
// NO UNDERSCORES: COMPILER ATTRIBUTES & UTILITIES
// ---------------------------------------------------------------------------------------------------------------------
#if defined(__clang__)
    #define SP_OPTIMIZE_LOOP _Pragma("clang loop vectorize(enable) interleave(enable)")
    #define SP_UNROLL        _Pragma("clang loop unroll(enable)")
#elif defined(__GNUC__)
    #define SP_OPTIMIZE_LOOP _Pragma("GCC ivdep")
    #define SP_UNROLL        _Pragma("GCC unroll 4")
#elif defined(_MSC_VER)
    #define SP_OPTIMIZE_LOOP _Pragma("loop(ivdep)")
    #define SP_UNROLL
#else
    #define SP_OPTIMIZE_LOOP
    #define SP_UNROLL
#endif

// Core attributes
#define SP_NODISCARD [[nodiscard]]
#if defined(_MSC_VER) && (_MSC_VER >= 1929)
    #define SP_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#elif defined(__has_cpp_attribute) && __has_cpp_attribute(no_unique_address) >= 201803L
    #define SP_NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
    #define SP_NO_UNIQUE_ADDRESS
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define SP_FORCEINLINE      __attribute__((always_inline)) inline
    #define SP_NOINLINE         __attribute__((noinline))
    #define SP_HOT              __attribute__((hot))
    #define SP_COLD             __attribute__((cold))
    #define SP_PURE             __attribute__((pure))
    #define SP_CONST            __attribute__((const))
    #define SP_FLATTEN          __attribute__((flatten))
    #define SP_OPTNONE          __attribute__((optnone))
    #define SP_RETURNS_NONNULL  __attribute__((returns_nonnull))
    #define SP_MALLOC           __attribute__((malloc))
    #define SP_ALLOC_ALIGN(n)   __attribute__((alloc_align(n)))
    #define SP_RESTRICT         __restrict__
    #define SP_LEAF             __attribute__((leaf))
    #define SP_PACK             __attribute__((pack))

    #if __SP_LIKELY__ == 1
        #define SP_EXPECT(expr, cond)   __builtin_expect((expr), (cond))
        #define SP_IF_EXPECT(expr)      if (__builtin_expect((expr), true)) [[likely]]
        #define SP_IF_NOT_EXPECT(expr)  if (__builtin_expect((expr), false)) [[unlikely]]
    #else
        #define SP_EXPECT(expr, cond)   (expr)
        #define SP_IF_EXPECT(expr)      if ((expr))
        #define SP_IF_NOT_EXPECT(expr)  if ((expr))
    #endif

#elif defined(_MSC_VER)
    #include <sal.h>
    #define SP_MALLOC
    #define SP_ALLOC_ALIGN(n)
    #define SP_HOT
    #define SP_COLD
    #define SP_PURE
    #define SP_LEAF
    #define SP_FORCEINLINE      __forceinline
    #define SP_NOINLINE         __declspec(noinline)
    #define SP_CONST            __declspec(noalias)
    #define SP_FLATTEN          inline
    #define SP_OPTNONE          __declspec(noinline)
    #define SP_RETURNS_NONNULL  _Ret_notnull_
    #define SP_RESTRICT         __restrict
    #define SP_EXPECT(expr, cond)   (expr)
    #define SP_PACK

    #if __SP_LIKELY__ == 1
        #define SP_IF_EXPECT(expr)      if ((expr)) [[likely]]
        #define SP_IF_NOT_EXPECT(expr)  if ((expr)) [[unlikely]]
    #else
        #define SP_IF_EXPECT(expr)      if ((expr))
        #define SP_IF_NOT_EXPECT(expr)  if ((expr))
    #endif

#else
    #define SP_FORCEINLINE  inline
    #define SP_FLATTEN      inline
    #define SP_NOINLINE
    #define SP_HOT
    #define SP_COLD
    #define SP_PURE
    #define SP_CONST
    #define SP_OPTNONE
    #define SP_RETURNS_NONNULL
    #define SP_MALLOC
    #define SP_ALLOC_ALIGN(n)
    #define SP_RESTRICT
    #define SP_LEAF
    #define SP_PACK
    #define SP_EXPECT(expr, cond)   (expr)
    #define SP_IF_EXPECT(expr)      if (expr)
    #define SP_IF_NOT_EXPECT(expr)  if (expr)
#endif

// Function signature helpers
#define _SP_FUNC_NI_   SP_NODISCARD SP_FORCEINLINE
#define _SP_FUNC_NIF_  SP_NODISCARD SP_FORCEINLINE SP_FLATTEN
#define _SP_FUNC_NIFH_ SP_NODISCARD SP_FORCEINLINE SP_FLATTEN SP_HOT
#define _SP_FUNC_NP_   SP_NODISCARD SP_PURE
#define _SP_FUNC_NIP_  SP_NODISCARD SP_FORCEINLINE SP_PURE
#define _SP_FUNC_NIFP_ SP_NODISCARD SP_FORCEINLINE SP_FLATTEN SP_PURE
#define _SP_FUNC_FH_   SP_FLATTEN SP_HOT
#define _SP_FUNC_FHP_  SP_FLATTEN SP_HOT SP_PURE
#define _SP_FUNC_FI_   SP_FLATTEN SP_FORCEINLINE
#define _SP_FUNC_HF_   SP_HOT SP_FLATTEN
#define _SP_FUNC_NFO_  SP_NODISCARD SP_FORCEINLINE
#define _SP_FUNC_NFORN_ _SP_FUNC_NFO_ SP_RETURNS_NONNULL
#define _SP_FUNC_NFOP_  _SP_FUNC_NFO_ SP_PURE

#define _SP_TEMP_F_ _SP_SAFETY_TEMPLATE_ SP_FLATTEN
#define _SP_TEMP_N_ _SP_SAFETY_TEMPLATE_ SP_NODISCARD
#define _SP_TEMP_H_ _SP_SAFETY_TEMPLATE_ SP_HOT
#define _SP_TEMP_HF_ _SP_TEMP_H_ SP_FLATTEN
#define _SP_TEMP_NF_ _SP_TEMP_N_ SP_FLATTEN
#define _SP_TEMP_NFO_ _SP_SAFETY_TEMPLATE_ _SP_FUNC_NFO_
#define _SP_TEMP_NP_ _SP_TEMP_N_ SP_PURE
#define _SP_TEMP_NFP_ _SP_TEMP_NF_ SP_PURE
#define _SP_TEMP_NFPFO_ _SP_TEMP_NFP_ SP_FORCEINLINE
#define _SP_TEMP_NPFO_ _SP_TEMP_NP_ SP_FORCEINLINE
#define _SP_TEMP_FO_ _SP_SAFETY_TEMPLATE_ SP_FORCEINLINE
#define _SP_TEMP_NFPI_  _SP_TEMP_NFPFO_



// ===========================// ===========================// ===========================// ===========================
// VARIABLES AND UTILITY FUNCTIONS
// ---------------------------------------------------------------------------------------------------------------------
typedef unsigned long long ull;
typedef long long ll;
typedef __SP_SIZE_TYPE__ size_type;

namespace sp {
    // Forward declarations
    enum Device { CPU, GPU };
    class IO;

    // Page size
    SP_FORCEINLINE ull get_system_page_size() {
    #if defined(_WIN32) || defined(_WIN64)
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        return si.dwPageSize;
    #elif defined(__unix__) || defined(__APPLE__)
        return static_cast<ull>(sysconf(_SC_PAGESIZE));
    #else
        return 4096;
    #endif
    }

    // Cache line size
    #if defined(__s390x__) || defined(__zarch__)
        constexpr ull sp_cache_line_size = 256ULL;
    #elif defined(__ia64__) || defined(_M_IA64) || defined(__powerpc__) || defined(__ppc__)
        constexpr ull sp_cache_line_size = 128ULL;
    #else
        constexpr ull sp_cache_line_size = 64ULL;
    #endif

    static constexpr size_type npos = (size_type)(-1);

    using ptrdiff_t = decltype(static_cast<int*>(nullptr) - static_cast<int*>(nullptr));
} // namespace sp

// -----------------------------------------------------------------------
#endif // ____SPIRALIS_INIT____