#ifndef ____SP_SIMD____
#define ____SP_SIMD____
#pragma once

#include "../setup/init.hpp"
#include "../parallel/thread.hpp"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define ___SP_FOUND_SIMD___ 1

#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(_M_ARM)
    #define ___SP_FOUND_SIMD___ 1
    #include <arm_neon.h>

    // Scalable Vector Extension (SVE)
    #if defined(__ARM_FEATURE_SVE)
        #include <arm_sve.h>
    #endif

    // Scalable Matrix Extension (SME)
    #if defined(__ARM_FEATURE_SME)
        #include <arm_sve.h> // SME often requires SVE types
        #include <arm_sme.h>
    #endif

#else
    #define ___SP_FOUND_SIMD___ 0
#endif

namespace sp{
class simd{
private:

#if ___SP_FOUND_SIMD___ == 1
    template<typename T> struct TypeTraits;
#endif



// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// ================================================ ARM NEON ================================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================
#if ___SP_SIMD_LEVEL___ == 10

// =================================================== // ===================================================
#define _SP_DEF_OP_(name, intrinsic, return1, return2, return3, V1, V2, V3, is_struct, n_args) \
    static SP_FORCEINLINE return1 name##512(_SP_ARGS_##n_args(V1)) { \
        return (return1){ {intrinsic(_SP_PASS_##is_struct##_##n_args##_(0)), intrinsic(_SP_PASS_##is_struct##_##n_args##_(1)), intrinsic(_SP_PASS_##is_struct##_##n_args##_(2)), intrinsic(_SP_PASS_##is_struct##_##n_args##_(3))} }; \
    } \
    static SP_FORCEINLINE return2 name##256(_SP_ARGS_##n_args(V2)) { \
        return (return2){ {intrinsic(_SP_PASS_##is_struct##_##n_args##_(0)), intrinsic(_SP_PASS_##is_struct##_##n_args##_(1))} }; \
    } \
    static SP_FORCEINLINE return3 name##128(_SP_ARGS_##n_args(V3)) { \
        return intrinsic(_SP_PASS_##is_struct##_##n_args); \
    }

#define _SP_DEF_SELECT_OP_(intrinsic, return1, return2, return3) \
    static SP_FORCEINLINE return1 select512(CmpVec mask, return1 a, return1 b) { \
        return (return1){ { \
            intrinsic(mask.val[0], a.val[0], b.val[0]), intrinsic(mask.val[1], a.val[1], b.val[1]), \
            intrinsic(mask.val[2], a.val[2], b.val[2]), intrinsic(mask.val[3], a.val[3], b.val[3]) \
        }}; \
    } \
    static SP_FORCEINLINE return2 select256(CmpVec2 mask, return2 a, return2 b) { \
        return (return2){ { intrinsic(mask.val[0], a.val[0], b.val[0]), intrinsic(mask.val[1], a.val[1], b.val[1]) } }; \
    } \
    static SP_FORCEINLINE return3 select128(CmpVec3 mask, return3 a, return3 b) { \
        return intrinsic(mask, a, b); \
    }

#define _SP_ARGS_1(T) T a
#define _SP_ARGS_2(T) T a, T b
#define _SP_ARGS_3(T) T a, T b, T c

#define _SP_PASS_T_1 a
#define _SP_PASS_T_2 a, b
#define _SP_PASS_T_3 a, b, c

#define _SP_PASS_F_1 a
#define _SP_PASS_F_2 a, b
#define _SP_PASS_F_3 a, b, c

#define _SP_PASS_T_3_(num) a.val[num], b.val[num], c.val[num]
#define _SP_PASS_T_2_(num) a.val[num], b.val[num]
#define _SP_PASS_T_1_(num) a.val[num]

#define _SP_PASS_F_3_(num) a, b, c
#define _SP_PASS_F_2_(num) a, b
#define _SP_PASS_F_1_(num) a

// =================================================== // ===================================================

#define _SP_DEF_LOADS_(intrinsic512, intrinsic256, intrinsic128, prim_type) \
    static SP_FORCEINLINE Vec load512(const prim_type* p){ return intrinsic512(p); } \
    static SP_FORCEINLINE Vec2 load256(const prim_type* p) { return intrinsic256(p); } \
    static SP_FORCEINLINE Vec3 load128(const prim_type* p) { return intrinsic128(p); }

// =================================================== // ===================================================

#define _SP_DEF_STORES_(intrinsic512, intrinsic256, intrinsic128, prim_type) \
    static SP_FORCEINLINE void store512(prim_type* p, Vec v) { intrinsic512(p, v); } \
    static SP_FORCEINLINE void store256(prim_type* p, Vec2 v) { intrinsic256(p, v); } \
    static SP_FORCEINLINE void store128(prim_type* p, Vec3 v) { intrinsic128(p, v); }

#define _SP_DEF_CMP_STORES_(intrinsic512, intrinsic256, intrinsic128, dtype) \
    static SP_FORCEINLINE void cmpstore512(dtype* p, CmpVec v) { intrinsic512(p, v); } \
    static SP_FORCEINLINE void cmpstore256(dtype* p, CmpVec2 v) { intrinsic256(p, v); } \
    static SP_FORCEINLINE void cmpstore128(dtype* p, CmpVec3 v) { intrinsic128(p, v); }

// =================================================== // ===================================================

#define _SP_DEFINE_SIMD_STRUCT_TRAITS_(_Vec, bits, x, dtype) \
    using Vec = _Vec##x4_t; using Vec2 = _Vec##x2_t; using Vec3 = _Vec##_t; \
    using CmpVec = uint##bits##x##x4_t; using CmpVec2 = uint##bits##x##x2_t; using CmpVec3 = uint##bits##x##_t; \
    static constexpr long step512 = (512 / (sizeof(dtype)*8)); \
    static constexpr long step256 = (256 / (sizeof(dtype)*8)); \
    static constexpr long step128 = (128 / (sizeof(dtype)*8));

// =================================================== // ===================================================

#define _SP_DEFINE_BASE_OPS_(dtype, sim_add, sim_sub, sim_eq, sim_gt, sim_lt, sim_ge, sim_le, sim_fill) \
    _SP_DEF_OP_(add, sim_add, Vec, Vec2, Vec3, Vec, Vec2, Vec3, T, 2) _SP_DEF_OP_(sub, sim_sub, Vec, Vec2, Vec3, Vec, Vec2, Vec3, T, 2) \
    _SP_DEF_OP_(eq, sim_eq, CmpVec, CmpVec2, CmpVec3, Vec, Vec2, Vec3, T, 2) _SP_DEF_OP_(gt, sim_gt, CmpVec, CmpVec2, CmpVec3, Vec, Vec2, Vec3, T, 2) \
    _SP_DEF_OP_(lt, sim_lt, CmpVec, CmpVec2, CmpVec3, Vec, Vec2, Vec3, T, 2) _SP_DEF_OP_(ge, sim_ge, CmpVec, CmpVec2, CmpVec3, Vec, Vec2, Vec3, T, 2) \
    _SP_DEF_OP_(le, sim_le, CmpVec, CmpVec2, CmpVec3, Vec, Vec2, Vec3, T, 2) _SP_DEF_OP_(fill, sim_fill, Vec, Vec2, Vec3, dtype, dtype, dtype, F, 1)

// =================================================== // ===================================================

#define _SP_DEFINE_BASE_STRUCT_TRAITS_(_Vec, bits, x, dtype, intrinsic_shortcut, intrinsic_shortcut2, mulSupport, divSupport, fmaSupport) \
    _SP_DEFINE_SIMD_STRUCT_TRAITS_(_Vec, bits, x, dtype) \
    static constexpr bool supportsMul = mulSupport; \
    static constexpr bool supportsDiv = divSupport; \
    static constexpr bool supportsFma = fmaSupport; \
    _SP_DEF_LOADS_(vld1q_##intrinsic_shortcut##_x4, vld1q_##intrinsic_shortcut##_x2, vld1q_##intrinsic_shortcut, dtype) \
    _SP_DEF_STORES_(vst1q_##intrinsic_shortcut##_x4, vst1q_##intrinsic_shortcut##_x2, vst1q_##intrinsic_shortcut, dtype) \
    _SP_DEF_CMP_STORES_(vst1q_##intrinsic_shortcut2##_x4, vst1q_##intrinsic_shortcut2##_x2, vst1q_##intrinsic_shortcut2, _SP_DEF_ON_SIZE_##bits##_) \
    _SP_DEFINE_BASE_OPS_(dtype, vaddq_##intrinsic_shortcut, vsubq_##intrinsic_shortcut, \
        vceqq_##intrinsic_shortcut, vcgtq_##intrinsic_shortcut, vcltq_##intrinsic_shortcut, \
        vcgeq_##intrinsic_shortcut, vcleq_##intrinsic_shortcut, vdupq_n_##intrinsic_shortcut \
    ) _SP_DEF_SELECT_OP_(vbslq_##intrinsic_shortcut, Vec, Vec2, Vec3)

#define _SP_DEF_ON_SIZE_64_ ull 
#define _SP_DEF_ON_SIZE_32_ unsigned int
#define _SP_DEF_ON_SIZE_8_ unsigned char
// =================================================== // ===================================================

#define _SP_DEFINE_FLOAT_OPS_(_Vec, bits, x, dtype, intrinsic_shortcut, intrinsic_shortcut2) \
    _SP_DEFINE_BASE_STRUCT_TRAITS_(_Vec, bits, x, dtype, intrinsic_shortcut, intrinsic_shortcut2, true, true, true) \
    _SP_DEF_OP_(mul, vmulq_##intrinsic_shortcut, Vec, Vec2, Vec3, Vec, Vec2, Vec3, T, 2) \
    _SP_DEF_OP_(div, vdivq_##intrinsic_shortcut, Vec, Vec2, Vec3, Vec, Vec2, Vec3, T, 2) \
    _SP_DEF_OP_(fma, vfmaq_##intrinsic_shortcut, Vec, Vec2, Vec3, Vec, Vec2, Vec3, T, 3) 

#define _SP_DEFINE_INT_OPS_(_Vec, bits, x, dtype, intrinsic_shortcut, intrinsic_shortcut2) \
    _SP_DEFINE_BASE_STRUCT_TRAITS_(_Vec, bits, x, dtype, intrinsic_shortcut, intrinsic_shortcut2, true, false, false) \
    _SP_DEF_OP_(mul, vmulq_##intrinsic_shortcut, Vec, Vec2, Vec3, Vec, Vec2, Vec3, T, 2) 
     
#define _SP_DEFINE_LONG_OPS_(_Vec, bits, x, dtype, intrinsic_shortcut, intrinsic_shortcut2) \
    _SP_DEFINE_BASE_STRUCT_TRAITS_(_Vec, bits, x, dtype, intrinsic_shortcut, intrinsic_shortcut2, false, false, false) 

// =================================================== // ===================================================

#define _SP_MAKE_INT_STRUCT_(dtype, _Vec, bits, x, intrinsic_shortcut, intrinsic_shortcut2) \
    template<> struct TypeTraits<dtype> { _SP_DEFINE_INT_OPS_(_Vec, bits, x, dtype, intrinsic_shortcut, intrinsic_shortcut2) };

#define _SP_MAKE_FLOAT_STRUCT_(dtype, _Vec, bits, x, intrinsic_shortcut, intrinsic_shortcut2) \
    template<> struct TypeTraits<dtype> { _SP_DEFINE_FLOAT_OPS_(_Vec, bits, x, dtype, intrinsic_shortcut, intrinsic_shortcut2) };

#define _SP_MAKE_LONG_STRUCT_(dtype, _Vec, bits, x, intrinsic_shortcut, intrinsic_shortcut2) \
    template<> struct TypeTraits<dtype> { _SP_DEFINE_LONG_OPS_(_Vec, bits, x, dtype, intrinsic_shortcut, intrinsic_shortcut2) };

#define _SP_MAKE_STRUCT_8_(prim_type, method, shortcut, letter) _SP_MAKE_##method##_STRUCT_(prim_type, shortcut##8x16, 8, x16, letter##8, u##8);
#define _SP_MAKE_STRUCT_32_(prim_type, method, shortcut, letter) _SP_MAKE_##method##_STRUCT_(prim_type, shortcut##32x4, 32, x4, letter##32, u##32)
#define _SP_MAKE_STRUCT_64_(prim_type, method, shortcut, letter) _SP_MAKE_##method##_STRUCT_(prim_type, shortcut##64x2, 64, x2, letter##64, u##64)


// =================================================== // ===================================================

_SP_MAKE_STRUCT_32_(float, FLOAT, float, f);
_SP_MAKE_STRUCT_64_(double, FLOAT, float, f);
_SP_MAKE_STRUCT_32_(int, INT, int, s);
_SP_MAKE_STRUCT_32_(unsigned int, INT, uint, u);
_SP_MAKE_STRUCT_64_(long long, LONG, int, s);
_SP_MAKE_STRUCT_64_(ull, LONG, uint, u);
_SP_MAKE_STRUCT_8_(unsigned char, INT, uint, u);

// =================================================== // ===================================================

#define _SP_SIMD_LOOP_BODY_(store, op, load) \
    for(; i <= n - Tr::step512; i += Tr::step512) Tr::store##512(&result[i], Tr::op##512(Tr::load##512(&arr1[i]), Tr::load##512(&arr2[i]))); \
    for(; i <= n - Tr::step256; i += Tr::step256) Tr::store##256(&result[i], Tr::op##256(Tr::load##256(&arr1[i]), Tr::load##256(&arr2[i]))); \
    for(; i <= n - Tr::step128; i += Tr::step128) Tr::store##128(&result[i], Tr::op##128(Tr::load##128(&arr1[i]), Tr::load##128(&arr2[i]))); 

#define _SP_SIMD_LOOP_SIZE_BODY_(mask_op, sel_t, sel_f, number) \
    for(; i <= n - Tr::step##number; i += Tr::step##number){ \
        auto _vec1 = Tr::load##number(&arr1[i]); auto _vec2 = Tr::load##number(&arr2[i]); \
        Tr::store##number(&result[i], Tr::select##number(Tr::mask_op##number(_vec1, _vec2), sel_t, sel_f)); \
    }

#define _SP_LOOP_SIMD_2_(op) _SP_SIMD_LOOP_BODY_(store, op, load)
#define _SP_LOOP_CMP_SIMD_2_(op) _SP_SIMD_LOOP_BODY_(cmpstore, op, load)

#define _SP_NOLOAD_LOOP_SIZE_BODY_(op, number) \
for(; i <= n - Tr::step##number; i += Tr::step##number) { \
    Tr::store##number(&result[i], Tr::op##number(num)); \
}

#define _SP_LOOP_NOLOAD_SIMD_1_(op) \
_SP_NOLOAD_LOOP_SIZE_BODY_(op, 512) _SP_NOLOAD_LOOP_SIZE_BODY_(op, 256) _SP_NOLOAD_LOOP_SIZE_BODY_(op, 128)

#define _SP_LOOP_SIMD_3_(op) \
    for(; i <= n - Tr::step512; i += Tr::step512) Tr::store512(&result[i], Tr::op##512(Tr::load512(&arr1[i]), Tr::load512(&arr2[i]), Tr::load512(&arr3[i]))); \
    for(; i <= n - Tr::step256; i += Tr::step256) Tr::store256(&result[i], Tr::op##256(Tr::load256(&arr1[i]), Tr::load256(&arr2[i]), Tr::load256(&arr3[i]))); \
    for(; i <= n - Tr::step128; i += Tr::step128) Tr::store128(&result[i], Tr::op##128(Tr::load128(&arr1[i]), Tr::load128(&arr2[i]), Tr::load128(&arr3[i]))); 

#define _SP_LOOP_SEL_SIMD_3_(mask_op) \
    _SP_SIMD_LOOP_SIZE_BODY_(mask_op, _vec1, _vec2, 512) \
    _SP_SIMD_LOOP_SIZE_BODY_(mask_op, _vec1, _vec2, 256) \
    _SP_SIMD_LOOP_SIZE_BODY_(mask_op, _vec1, _vec2, 128)

#define _SP_LOOP_SEL_EXPL_SIMD_3_(mask_op) \
    _SP_SIMD_LOOP_SIZE_BODY_(mask_op, Tr::load512(&true_arr[i]), Tr::load512(&false_arr[i]), 512) \
    _SP_SIMD_LOOP_SIZE_BODY_(mask_op, Tr::load256(&true_arr[i]), Tr::load256(&false_arr[i]), 256) \
    _SP_SIMD_LOOP_SIZE_BODY_(mask_op, Tr::load128(&true_arr[i]), Tr::load128(&false_arr[i]), 128)

#endif // ___SP_SIMD_LEVEL___ == 10


// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// ============================================== GENERIC OPS ===============================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================


#if ___SP_FOUND_SIMD___ == 1
    #define _SP_IF_FOUND(...) __VA_ARGS__
#else
    #define _SP_IF_FOUND(...)
#endif

#define _SP_STRIP(...) __VA_ARGS__

#define _SP_BACKEND_TEMPLATE(name, suffix, params, simd_loop, scalar_op, constexpr_check) \
template <typename T, typename... Extra> \
static SP_FORCEINLINE void name##suffix(_SP_STRIP params, long long n) { \
    long long i = 0; \
    _SP_IF_FOUND( \
        using Tr = TypeTraits<T>; \
        if constexpr(constexpr_check) simd_loop; \
    ) \
    for(; i < n; i++) { scalar_op; } \
}

#define _SP_DEF_BACKEND_GENERIC_1_(name, constexpr_check) \
    _SP_BACKEND_TEMPLATE(name, _generic, (const T& num, T* SP_RESTRICT result), _SP_LOOP_NOLOAD_SIMD_1_(name), result[i] = num, constexpr_check)

#define _SP_DEF_BACKEND_GENERIC_2_(name, op, constexpr_check) \
    _SP_BACKEND_TEMPLATE(name, _generic, (T* SP_RESTRICT arr1, T* SP_RESTRICT arr2, T* SP_RESTRICT result), \
    _SP_LOOP_SIMD_2_(name), \
    result[i] = arr1[i] op arr2[i], constexpr_check)

#define _SP_DEF_BACKEND_GENERIC_3_(name, constexpr_check, ...) \
    _SP_BACKEND_TEMPLATE(name, _generic, (T* SP_RESTRICT arr1, T* SP_RESTRICT arr2, T* SP_RESTRICT arr3, T* SP_RESTRICT result), \
    _SP_LOOP_SIMD_3_(name), \
    __VA_ARGS__, constexpr_check)

#define _SP_DEF_BACKEND_VEC_CMP_2_(name, op, t, f, constexpr_check) \
    template <typename T, typename U> \
    static SP_FORCEINLINE void name##_vec_generic(T* SP_RESTRICT arr1, T* SP_RESTRICT arr2, U* SP_RESTRICT result, long long n) { \
        long long i = 0; _SP_IF_FOUND(using Tr = TypeTraits<T>; if constexpr(constexpr_check) { _SP_LOOP_CMP_SIMD_2_(name); }) \
        for(; i < n; i++) result[i] = (arr1[i] op arr2[i]) ? t : f; \
    }

#define _SP_DEF_BACKEND_VEC_SEL_2_(name, op, constexpr_check) \
    _SP_BACKEND_TEMPLATE(name, _vec_select_generic, (T* SP_RESTRICT arr1, T* SP_RESTRICT arr2, T* SP_RESTRICT result), \
    _SP_LOOP_SEL_SIMD_3_(name), \
    result[i] = (arr1[i] op arr2[i]) ? arr1[i] : arr2[i], constexpr_check) \

#define _SP_DEF_BACKEND_VEC_SEL_EXPL_2_(name, op, constexpr_check) \
    _SP_BACKEND_TEMPLATE(name, _vec_explicit_generic, (T* SP_RESTRICT arr1, T* SP_RESTRICT arr2, T* SP_RESTRICT true_arr, T* SP_RESTRICT false_arr, T* SP_RESTRICT result), \
    _SP_LOOP_SEL_EXPL_SIMD_3_(name), \
    result[i] = (arr1[i] op arr2[i]) ? true_arr[i] : false_arr[i], constexpr_check)

// =================================================== // ===================================================

_SP_DEF_BACKEND_GENERIC_1_(fill, true);
_SP_DEF_BACKEND_GENERIC_2_(add, +, true);
_SP_DEF_BACKEND_GENERIC_2_(sub, -, true);
_SP_DEF_BACKEND_GENERIC_2_(mul, *, Tr::supportsMul);
_SP_DEF_BACKEND_GENERIC_2_(div, /, Tr::supportsDiv);
_SP_DEF_BACKEND_GENERIC_3_(fma, Tr::supportsFma, result[i] = (arr3[i] + (arr1[i] * arr2[i])));

#define _SP_DEF_BACKEND_ALL_CMP_VARIANTS_(name, op) \
_SP_DEF_BACKEND_VEC_CMP_2_(name, op, -1, 0, true) _SP_DEF_BACKEND_VEC_SEL_2_(name, op, true) _SP_DEF_BACKEND_VEC_SEL_EXPL_2_(name, op, true)

_SP_DEF_BACKEND_ALL_CMP_VARIANTS_(eq, ==);
_SP_DEF_BACKEND_ALL_CMP_VARIANTS_(gt, >);
_SP_DEF_BACKEND_ALL_CMP_VARIANTS_(lt, <);
_SP_DEF_BACKEND_ALL_CMP_VARIANTS_(gte, >=);
_SP_DEF_BACKEND_ALL_CMP_VARIANTS_(lte, <=);

// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// ========================================= GENERIC PUBLIC MACROS ==========================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================

#define _SP_MAKE_GENERIC_SIMD_1_(op) \
template <typename T> SP_FLATTEN SP_FORCEINLINE static void op(const T& num, T* SP_RESTRICT result, long long n) { op##_generic(num, result, n); }

#define _SP_MAKE_GENERIC_SIMD_2_(op) \
template <typename T> SP_FLATTEN SP_FORCEINLINE static void op(T* SP_RESTRICT arr1, T* SP_RESTRICT arr2, T* SP_RESTRICT result, long long n) { op##_generic(arr1, arr2, result, n); }

#define _SP_MAKE_GENERIC_SIMD_3_(op) \
template <typename T> SP_FLATTEN SP_FORCEINLINE static void op(T* SP_RESTRICT arr1, T* SP_RESTRICT arr2, T* SP_RESTRICT arr3, T* SP_RESTRICT result, long long n) { op##_generic(arr1, arr2, arr3, result, n); }

#define _SP_MAKE_CMP_SIMD_2_(op) \
template <typename T, typename U> SP_FLATTEN SP_FORCEINLINE static void op##_vec(T* SP_RESTRICT arr1, T* SP_RESTRICT arr2, U* SP_RESTRICT result, long long n){ op##_vec_generic(arr1, arr2, result, n); }

#define _SP_MAKE_CMP_SEL_SIMD_2_(op) \
template <typename T> SP_FLATTEN SP_FORCEINLINE static void op##_vec_select(T* SP_RESTRICT arr1, T* SP_RESTRICT arr2, T* SP_RESTRICT result, long long n) { op##_vec_select_generic(arr1, arr2, result, n); }

#define _SP_MAKE_CMP_SEL_EXPL_SIMD_2_(op) \
template <typename T> SP_FLATTEN SP_FORCEINLINE static void op##_vec_explicit(T* SP_RESTRICT arr1, T* SP_RESTRICT arr2, T* SP_RESTRICT arr3, T* SP_RESTRICT arr4, T* SP_RESTRICT result, long long n) { op##_vec_explicit_generic(arr1, arr2, arr3, arr4, result, n); }

#define _SP_GENERIC_SIMD_POOL_BODY_(op, params) \
    SP_IF_NOT_EXPECT(n < (long)min_elements) { \
        sp::simd::op(_SP_DEF_POOL_OP_INPUT_##params result, n); \
        return; \
    } \
    const long multiplier = 8; \
    const long numTasks = (long)p.size() * multiplier; \
    \
    /* 1. Calculate how many elements of type T fit into a 64-byte cache line */ \
    const long elements_per_cache_line = 64 / sizeof(T) > 0 ? 64 / sizeof(T) : 1; \
    \
    /* 2. Round the raw chunk size UP to the nearest multiple of elements_per_cache_line */ \
    const long raw_chunk = (n + numTasks - 1) / numTasks; \
    const long chunkSize = (raw_chunk + elements_per_cache_line - 1) & ~(elements_per_cache_line - 1); \
    \
    long pos = 0; \
    while (pos + chunkSize < n) { \
        p.enqueue([=]() { sp::simd::op(_SP_DEF_POOL_OP_POS_INPUT_##params result + pos, chunkSize); }); \
        pos += chunkSize; \
    } \
    long remaining_sz = n - pos; \
    if (remaining_sz > 0) { sp::simd::op(_SP_DEF_POOL_OP_POS_INPUT_##params result + pos, remaining_sz); } \
    p.wait_all();


#define _SP_MAKE_GENERIC_SIMD_POOL_(op, params) \
template <typename T> SP_FLATTEN SP_FORCEINLINE static void op(_SP_DEF_POOL_FUNC_HEADER_PARAMS_##params T* SP_RESTRICT result, long long n, thread_pool& p, ull min_elements = 10'000) { _SP_GENERIC_SIMD_POOL_BODY_(op, params) }

#define _SP_MAKE_CMP_SIMD_POOL_(op, params) \
template <typename T, typename U> SP_FLATTEN SP_FORCEINLINE static void op(_SP_DEF_POOL_FUNC_HEADER_PARAMS_##params U* SP_RESTRICT result, long long n, thread_pool& p, ull min_elements = 10'000) { _SP_GENERIC_SIMD_POOL_BODY_(op, params) }

#define _SP_MAKE_CMP_SEL_SIMD_POOL_(op, params) \
template <typename T> SP_FLATTEN SP_FORCEINLINE static void op(_SP_DEF_POOL_FUNC_HEADER_PARAMS_##params T* SP_RESTRICT result, long long n, thread_pool& p, ull min_elements = 10'000) { _SP_GENERIC_SIMD_POOL_BODY_(op, params) }

#define _SP_MAKE_CMP_SEL_EXPL_SIMD_POOL_(op, params) \
template <typename T> SP_FLATTEN SP_FORCEINLINE static void op(_SP_DEF_POOL_FUNC_HEADER_PARAMS_##params T* SP_RESTRICT result, long long n, thread_pool& p, ull min_elements = 10'000) { _SP_GENERIC_SIMD_POOL_BODY_(op, params) }

#define _SP_DEF_POOL_FUNC_HEADER_PARAMS_2 T* SP_RESTRICT arr1, T* SP_RESTRICT arr2, 
#define _SP_DEF_POOL_FUNC_HEADER_PARAMS_3 _SP_DEF_POOL_FUNC_HEADER_PARAMS_2 T* SP_RESTRICT arr3, 
#define _SP_DEF_POOL_FUNC_HEADER_PARAMS_TF T* SP_RESTRICT arr1, T* SP_RESTRICT arr2, T* SP_RESTRICT true_arr, T* SP_RESTRICT false_arr, 

#define _SP_DEF_POOL_OP_INPUT_2 arr1, arr2,
#define _SP_DEF_POOL_OP_INPUT_3 arr1, arr2, arr3,
#define _SP_DEF_POOL_OP_INPUT_TF arr1, arr2, true_arr, false_arr, 

#define _SP_DEF_POOL_OP_POS_INPUT_2 arr1 + pos, arr2 + pos,
#define _SP_DEF_POOL_OP_POS_INPUT_3 arr1 + pos, arr2 + pos, arr3 + pos,
#define _SP_DEF_POOL_OP_POS_INPUT_TF arr1 + pos, arr2 + pos, true_arr + pos, false_arr + pos, 


// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// ============================================== PUBLIC OPS ================================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================

#define _SP_MAKE_ALL_CMP_SIMD_(op) \
_SP_MAKE_CMP_SEL_EXPL_SIMD_2_(op) _SP_MAKE_CMP_SEL_EXPL_SIMD_POOL_(op##_vec_explicit, TF) _SP_MAKE_CMP_SEL_SIMD_2_(op) \
_SP_MAKE_CMP_SEL_SIMD_POOL_(op, 2) _SP_MAKE_CMP_SIMD_2_(op) _SP_MAKE_CMP_SIMD_POOL_(op, 2)

public:

_SP_MAKE_ALL_CMP_SIMD_(eq);
_SP_MAKE_ALL_CMP_SIMD_(gt);
_SP_MAKE_ALL_CMP_SIMD_(lt);
_SP_MAKE_ALL_CMP_SIMD_(gte);
_SP_MAKE_ALL_CMP_SIMD_(lte);

_SP_MAKE_GENERIC_SIMD_1_(fill);

_SP_MAKE_GENERIC_SIMD_2_(add);
_SP_MAKE_GENERIC_SIMD_2_(sub);
_SP_MAKE_GENERIC_SIMD_2_(mul);
_SP_MAKE_GENERIC_SIMD_2_(div);
_SP_MAKE_GENERIC_SIMD_3_(fma);

_SP_MAKE_GENERIC_SIMD_POOL_(add, 2);
_SP_MAKE_GENERIC_SIMD_POOL_(sub, 2);
_SP_MAKE_GENERIC_SIMD_POOL_(mul, 2);
_SP_MAKE_GENERIC_SIMD_POOL_(div, 2);
_SP_MAKE_GENERIC_SIMD_POOL_(fma, 3);


}; // class simd
}; // namespace sp

// =================================================== 
// 1. Cleanup of macros specific to the ARM NEON implementation
// ===================================================
#undef _SP_DEF_OP_
#undef _SP_DEF_SELECT_OP_
#undef _SP_ARGS_1
#undef _SP_ARGS_2
#undef _SP_ARGS_3
#undef _SP_PASS_1
#undef _SP_PASS_2
#undef _SP_PASS_3
#undef _SP_PASS_1_
#undef _SP_PASS_2_
#undef _SP_PASS_3_
#undef _SP_DEF_LOADS_
#undef _SP_DEF_STORES_
#undef _SP_DEF_CMP_STORES_
#undef _SP_DEFINE_SIMD_STRUCT_TRAITS_
#undef _SP_DEFINE_BASE_OPS_
#undef _SP_DEFINE_BASE_STRUCT_TRAITS_
#undef _SP_DEF_ON_SIZE_64_
#undef _SP_DEF_ON_SIZE_32_
#undef _SP_DEF_ON_SIZE_8_
#undef _SP_DEFINE_FLOAT_OPS_
#undef _SP_DEFINE_INT_OPS_
#undef _SP_DEFINE_LONG_OPS_
#undef _SP_MAKE_INT_STRUCT_
#undef _SP_MAKE_FLOAT_STRUCT_
#undef _SP_MAKE_LONG_STRUCT_
#undef _SP_MAKE_STRUCT_8_
#undef _SP_MAKE_STRUCT_32_
#undef _SP_MAKE_STRUCT_64_
#undef _SP_SIMD_LOOP_BODY_
#undef _SP_SIMD_LOOP_SIZE_BODY_
#undef _SP_LOOP_SIMD_2_
#undef _SP_LOOP_CMP_SIMD_2_
#undef _SP_LOOP_SIMD_3_
#undef _SP_LOOP_SEL_SIMD_3_
#undef _SP_LOOP_SEL_EXPL_SIMD_3_
#if ___SP_SIMD_LEVEL___ == 10

#endif // ___SP_SIMD_LEVEL___ == 10

// =================================================== 
// 2. Cleanup of Generic Backend and Public Interface macros
// These were defined outside the level check and must be undef'd here
// ===================================================
#undef _SP_IF_FOUND
#undef _SP_STRIP
#undef _SP_BACKEND_TEMPLATE
#undef _SP_DEF_BACKEND_GENERIC_2_
#undef _SP_DEF_BACKEND_GENERIC_3_
#undef _SP_DEF_BACKEND_VEC_CMP_2_
#undef _SP_DEF_BACKEND_VEC_SEL_2_
#undef _SP_DEF_BACKEND_VEC_SEL_EXPL_2_
#undef _SP_DEF_BACKEND_ALL_CMP_VARIANTS_
#undef _SP_MAKE_GENERIC_SIMD_2_
#undef _SP_MAKE_GENERIC_SIMD_3_
#undef _SP_MAKE_CMP_SIMD_2_
#undef _SP_MAKE_CMP_SEL_SIMD_2_
#undef _SP_MAKE_CMP_SEL_EXPL_SIMD_2_
#undef _SP_GENERIC_SIMD_POOL_BODY_
#undef _SP_MAKE_GENERIC_SIMD_POOL_
#undef _SP_MAKE_CMP_SIMD_POOL_
#undef _SP_MAKE_CMP_SEL_SIMD_POOL_
#undef _SP_MAKE_CMP_SEL_EXPL_SIMD_POOL_
#undef _SP_DEF_POOL_FUNC_HEADER_PARAMS_2
#undef _SP_DEF_POOL_FUNC_HEADER_PARAMS_3
#undef _SP_DEF_POOL_FUNC_HEADER_PARAMS_TF
#undef _SP_DEF_POOL_OP_INPUT_2
#undef _SP_DEF_POOL_OP_INPUT_3
#undef _SP_DEF_POOL_OP_INPUT_TF
#undef _SP_DEF_POOL_OP_POS_INPUT_2
#undef _SP_DEF_POOL_OP_POS_INPUT_3
#undef _SP_DEF_POOL_OP_POS_INPUT_TF
#undef _SP_MAKE_ALL_CMP_SIMD_


#endif // ____SP_SIMD____