#ifndef ____SP_TYPE_TRAITS____
#define ____SP_TYPE_TRAITS____
#pragma once
#include "../setup/init.hpp"
#include <initializer_list>
// FORWARD DECLARATIONS
namespace spt {
    template<typename T, typename U> struct is_same;
    template<typename T> struct is_const;
    template <typename T> struct is_integral;
    template<typename T> struct is_floating_point;
    template<typename T> struct add_rvalue_reference;
    template <typename T> typename add_rvalue_reference<T>::type declval() noexcept;
    template <typename T> struct remove_reference;
}

namespace sp{
    template <template <typename, auto...> typename Alloc, auto... Args>
    struct bind_alloc{
        template <typename T>
        using r = Alloc<T, Args...>;
    };

    template <typename T>
    SP_FORCEINLINE T* addressof(T& value) noexcept{ return reinterpret_cast<T*>(const_cast<char*>(&reinterpret_cast<const volatile char&>(value))); }
    template <typename T> void addressof(const T&&) = delete;


    template <typename T>
    constexpr typename spt::remove_reference<T>::type&& move(T&& arg) noexcept {
        return static_cast<typename spt::remove_reference<T>::type&&>(arg);
    }

    template <class U>
    SP_FORCEINLINE U&& forward(typename spt::remove_reference<U>::type& ref){
        return (U&&)(ref);
    }

    template <typename T, typename U>
    SP_FORCEINLINE void swap(T& one, U& two) {
        T temp = sp::move(one);
        one = static_cast<T>(sp::move(two));
        two = static_cast<U>(sp::move(temp));
    }
}

namespace spt{

// ============================================================
// SECTION 1: CORE BUILDING BLOCKS
// ============================================================

// --- Integral constant & bool wrappers ---
template<typename T, T V>
struct integral_constant{
    static constexpr T value = V;

    using value_type = T;
    using type = integral_constant<T, V>;

    // Conversions
    constexpr operator value_type() const noexcept { return value; }
    constexpr value_type operator()() const noexcept { return value; }
};

template<bool B>
using bool_constant = integral_constant<bool, B>;

using true_type  = bool_constant<true>;
using false_type = bool_constant<false>;


// --- Type identity (prevents deduction, useful for SFINAE anchors) ---
template<typename T>
struct type_identity{
    using type = T;
};

template<typename T>
using type_identity_t = typename type_identity<T>::type;

// --- Void type (SFINAE sink) ---
template<typename...>
using void_t = void;

// --- Conditional ---
template<bool B, typename T, typename F>
struct conditional;

template<typename T, typename F>
struct conditional<true, T, F> { using type = T; };

template<typename T, typename F>
struct conditional<false, T, F> { using type = F; };

template<bool B, typename T, typename F>
using conditional_t = typename conditional<B, T, F>::type;

// --- Enable if ---
template<bool B, typename T = void>
struct enable_if {};

template <typename T>
struct enable_if<true, T> { using type = T; };

template<bool B, typename T = void>
using enable_if_t = typename enable_if<B, T>::type;

// --- Logical combinators ---
template<typename... Bs>
struct conjunction : true_type {};
template <typename B>
struct conjunction<B> : B {};
template<typename B1, typename... Bs>
struct conjunction<B1, Bs...> : conditional_t<B1::value, conjunction<Bs...>, B1> {};

template<typename... Bs>
struct disjunction : false_type {};
template <typename B>
struct disjunction<B> : B {};
template<typename B1, typename... Bs>
struct disjunction<B1, Bs...> : conditional_t<!B1::value, disjunction<Bs...>, B1> {};

template<typename B>
struct negation : bool_constant<!B::value> {};

template<typename... Bs>
inline constexpr bool conjunction_v = conjunction<Bs...>::value;

template<typename... Bs>
inline constexpr bool disjunction_v = disjunction<Bs...>::value;

template<typename B>
inline constexpr bool negation_v = negation<B>::value;

// ============================================================
// SECTION 2: CV / REF QUALIFIERS
// ============================================================

template<typename T> struct remove_const { using type = T; };
template<typename T> struct remove_const<const T> { using type = T; };
template<typename T> struct remove_volatile { using type = T; };
template<typename T> struct remove_volatile<volatile T> { using type = T; };
template<typename T> struct remove_cv   { using type = typename remove_const<typename remove_volatile<T>::type>::type; };
template<typename T> struct add_const   { using type = const T; };
template<typename T> struct add_volatile{ using type = volatile T; };
template<typename T> struct add_cv      { using type = const volatile T; };

template<typename T> using remove_const_t    = typename remove_const<T>::type;
template<typename T> using remove_volatile_t = typename remove_volatile<T>::type;
template<typename T> using remove_cv_t       = typename remove_cv<T>::type;
template<typename T> using add_const_t       = typename add_const<T>::type;
template<typename T> using add_volatile_t    = typename add_volatile<T>::type;
template<typename T> using add_cv_t          = typename add_cv<T>::type;

template<typename T> struct remove_reference { using type = T; };
template<typename T> struct remove_reference<T&> { using type = T; };
template<typename T> struct remove_reference<T&&> { using type = T; };
template<typename T> struct add_lvalue_reference { using type = conditional_t<is_same<T, void>::value, T, T&>; };
template<typename T> struct add_rvalue_reference { using type = conditional_t<is_same<T, void>::value, T, T&&>; };

template<typename T> using remove_reference_t      = typename remove_reference<T>::type;
template<typename T> using add_lvalue_reference_t  = typename add_lvalue_reference<T>::type;
template<typename T> using add_rvalue_reference_t  = typename add_rvalue_reference<T>::type;

template<typename T> struct remove_cvref { using type = typename remove_const<typename remove_volatile<typename remove_reference<T>::type>::type>::type; };
template<typename T> using remove_cvref_t = typename remove_cvref<T>::type;

// ============================================================
// SECTION 3: POINTER / ARRAY / EXTENT
// ============================================================

template<typename T> struct remove_pointer { using type = T; };
template<typename T> struct remove_pointer<T*> { using type = T; };
template<typename T> struct remove_pointer<T**> { using type = T; };
template<typename T> struct add_pointer { using type = T*; };
template<typename T> using remove_pointer_t = typename remove_pointer<T>::type;
template<typename T> using add_pointer_t    = typename add_pointer<T>::type;

template<typename T> struct remove_extent;
template<typename T> struct remove_all_extents;
template<typename T> using remove_extent_t      = typename remove_extent<T>::type;
template<typename T> using remove_all_extents_t = typename remove_all_extents<T>::type;

template<typename T> struct rank : integral_constant<unsigned, 0> {};
template<typename T, size_t N> struct rank<T[N]> : integral_constant<unsigned, rank<T>::value + 1> {};
template<typename T> struct rank<T[]> : integral_constant<unsigned, rank<T>::value + 1> {};
template<typename T, unsigned N = 0> struct extent : integral_constant<unsigned, 0> {};
template<typename T, size_t I, unsigned N> struct extent<T[I], N> : integral_constant<unsigned, N == 0 ? I : extent<T, N - 1>::value> {};
template<typename T, unsigned N> struct extent<T[], N> : integral_constant<unsigned, N == 0 ? 0 : extent<T, N - 1>::value> {};

template<typename T> inline constexpr unsigned rank_v = rank<T>::value;
template<typename T, unsigned N = 0> inline constexpr unsigned extent_v = extent<T, N>::value;

// ============================================================
// SECTION 4: PRIMARY TYPE CATEGORIES
// ============================================================

// Helpers using the remove_cv_t design pattern
template<typename T> struct is_void_h           : false_type {};
template<>           struct is_void_h<void>     : true_type {};
template<typename T> struct is_void : is_void_h<remove_cv_t<T>> {};

template<typename T> struct is_null_pointer_h                  : false_type {};
template<>           struct is_null_pointer_h<decltype(nullptr)> : true_type {};
template<typename T> struct is_null_pointer : is_null_pointer_h<remove_cv_t<T>> {};

template<typename T> struct is_array_h              : false_type {};
template<typename T, size_t N> struct is_array_h<T[N]> : true_type {};
template<typename T> struct is_array_h<T[]>         : true_type {};
template<typename T> struct is_array : is_array_h<remove_cv_t<T>> {};

template<typename T> struct is_pointer_h     : false_type {};
template<typename T> struct is_pointer_h<T*> : true_type {};
template<typename T> struct is_pointer : is_pointer_h<remove_cv_t<T>> {};

template<typename T> struct is_lvalue_reference     : false_type {};
template<typename T> struct is_lvalue_reference<T&> : true_type {};

template<typename T> struct is_rvalue_reference      : false_type {};
template<typename T> struct is_rvalue_reference<T&&> : true_type {};

template<typename T> struct is_member_pointer_h          : false_type {};
template<typename T, typename U> struct is_member_pointer_h<T U::*> : true_type {};
template<typename T> struct is_member_pointer : is_member_pointer_h<remove_cv_t<T>> {};

template<typename T> struct is_function : bool_constant<!is_const<const T>::value && !(is_lvalue_reference<T>::value || is_rvalue_reference<T>::value)> {};

template<typename T> struct is_member_object_pointer_h          : false_type {};
template<typename T, typename U> struct is_member_object_pointer_h<T U::*> : bool_constant<!is_function<T>::value> {}; 
template<typename T> struct is_member_object_pointer : is_member_object_pointer_h<remove_cv_t<T>> {};

template<typename T> 
struct is_member_function_pointer : bool_constant<is_member_pointer<T>::value && !is_member_object_pointer<T>::value> {};

// Compiler Intrinsic Dependent Traits
template<typename T> struct is_enum  : bool_constant<__is_enum(T)> {};
template<typename T> struct is_union : bool_constant<__is_union(T)> {};
template<typename T> struct is_class : bool_constant<__is_class(T)> {};


// --- Variable Templates Helper Shortcuts ---
template<typename T> inline constexpr bool is_void_v                    = is_void<T>::value;
template<typename T> inline constexpr bool is_null_pointer_v            = is_null_pointer<T>::value;
template<typename T> inline constexpr bool is_array_v                   = is_array<T>::value;
template<typename T> inline constexpr bool is_pointer_v                 = is_pointer<T>::value;
template<typename T> inline constexpr bool is_lvalue_reference_v        = is_lvalue_reference<T>::value;
template<typename T> inline constexpr bool is_rvalue_reference_v        = is_rvalue_reference<T>::value;
template<typename T> inline constexpr bool is_member_object_pointer_v   = is_member_object_pointer<T>::value;
template<typename T> inline constexpr bool is_member_function_pointer_v = is_member_function_pointer<T>::value;
template<typename T> inline constexpr bool is_enum_v                    = is_enum<T>::value;
template<typename T> inline constexpr bool is_union_v                   = is_union<T>::value;
template<typename T> inline constexpr bool is_class_v                   = is_class<T>::value;
template<typename T> inline constexpr bool is_function_v                = is_function<T>::value;
// ============================================================
// SECTION 5: COMPOSITE TYPE CATEGORIES
// ============================================================
template<typename T> struct is_reference : bool_constant<!(is_same<T, typename remove_reference<T>::type>::type)> {};
template<typename T> struct is_arithmetic : bool_constant<is_integral<T>::value || is_floating_point<T>::value> {};
template<typename T> struct is_fundamental : bool_constant<is_arithmetic<T>::value || is_void<T>::value> {};
template<typename T> struct is_object : bool_constant<!is_function<T>::value && !is_reference<T>::value> {};
template<typename T> struct is_scalar : bool_constant<is_arithmetic<T>::value || is_enum<T>::value || is_pointer<T>::value || is_member_pointer<T>::value> {};
template<typename T> struct is_compound : bool_constant<!is_fundamental<T>::value> {};
//template<typename T> struct is_member_pointer : bool_constant<is_member_object_pointer<T>::value || is_member_function_pointer<T>::value> {};

template<typename T> inline constexpr bool is_reference_v       = is_reference<T>::value;
template<typename T> inline constexpr bool is_arithmetic_v      = is_arithmetic<T>::value;
template<typename T> inline constexpr bool is_fundamental_v     = is_fundamental<T>::value;
template<typename T> inline constexpr bool is_object_v          = is_object<T>::value;
template<typename T> inline constexpr bool is_scalar_v          = is_scalar<T>::value;
template<typename T> inline constexpr bool is_compound_v        = is_compound<T>::value;
template<typename T> inline constexpr bool is_member_pointer_v  = is_member_pointer<T>::value;

// ============================================================
// SECTION 6: TYPE PROPERTIES
// ============================================================
template<typename T> struct is_const          : false_type {};
template<typename T> struct is_const<const T> : true_type {};

template<typename T> struct is_volatile             : false_type {};
template<typename T> struct is_volatile<volatile T> : true_type {};

template<typename T> struct is_trivial              : bool_constant<__is_trivial(T)> {};
template<typename T> struct is_trivially_copyable   : bool_constant<__is_trivially_copyable(T)> {};
template<typename T> struct is_standard_layout      : bool_constant<__is_standard_layout(T)> {};
template<typename T> struct is_empty                : bool_constant<__is_empty(T)> {};
template<typename T> struct is_polymorphic          : bool_constant<__is_polymorphic(T)> {};
template<typename T> struct is_abstract             : bool_constant<__is_abstract(T)> {};
template<typename T> struct is_final                : bool_constant<__is_final(T)> {};
template<typename T> struct is_aggregate            : bool_constant<__is_aggregate(T)> {};
template<typename T> struct is_signed               : bool_constant<__is_signed(T)> {};
template<typename T> struct is_unsigned             : bool_constant<__is_unsigned(T)> {};

// --- Array Bounds Checking ---
template<typename T> struct is_bounded_array        : bool_constant<is_array_v<T> && extent_v<T, 0> != 0> {};
template<typename T> struct is_unbounded_array      : bool_constant<is_array_v<T> && extent_v<T, 0> == 0> {};

// --- Scoped Enum ---
template<typename T> struct is_scoped_enum          : bool_constant<__is_scoped_enum(T)> {};

// --- Variable Templates Helper Shortcuts (Stripped 'typename') ---
template<typename T> inline constexpr bool is_const_v              = is_const<T>::value;
template<typename T> inline constexpr bool is_volatile_v           = is_volatile<T>::value;
template<typename T> inline constexpr bool is_trivial_v            = is_trivial<T>::value;
template<typename T> inline constexpr bool is_trivially_copyable_v = is_trivially_copyable<T>::value;
template<typename T> inline constexpr bool is_standard_layout_v    = is_standard_layout<T>::value;
template<typename T> inline constexpr bool is_empty_v              = is_empty<T>::value;
template<typename T> inline constexpr bool is_polymorphic_v        = is_polymorphic<T>::value;
template<typename T> inline constexpr bool is_abstract_v           = is_abstract<T>::value;
template<typename T> inline constexpr bool is_final_v              = is_final<T>::value;
template<typename T> inline constexpr bool is_aggregate_v          = is_aggregate<T>::value;
template<typename T> inline constexpr bool is_signed_v             = is_signed<T>::value;
template<typename T> inline constexpr bool is_unsigned_v           = is_unsigned<T>::value;
template<typename T> inline constexpr bool is_bounded_array_v      = is_bounded_array<T>::value;
template<typename T> inline constexpr bool is_unbounded_array_v    = is_unbounded_array<T>::value;
template<typename T> inline constexpr bool is_scoped_enum_v        = is_scoped_enum<T>::value;
// ============================================================
// SECTION 7: CONSTRUCTIBILITY & ACTIONS
// ============================================================

// --- Base Core Intrinsics ---
template<typename T, typename... Args> struct is_constructible : bool_constant<__is_constructible(T, Args...)> {};
template<typename T, typename... Args> struct is_trivially_constructible : bool_constant<__is_trivially_constructible(T, Args...)> {};
template<typename T, typename... Args> struct is_nothrow_constructible : bool_constant<__is_nothrow_constructible(T, Args...)> {};
template<typename T, typename U> struct is_assignable : bool_constant<__is_assignable(T, U)> {};
template<typename T, typename U> struct is_trivially_assignable : bool_constant<__is_trivially_assignable(T, U)> {};
template<typename T, typename U> struct is_nothrow_assignable : bool_constant<__is_nothrow_assignable(T, U)> {};
template<typename T> struct is_destructible : bool_constant<__is_destructible(T)> {};
template<typename T> struct is_trivially_destructible : bool_constant<__is_trivially_destructible(T)> {};
template<typename T> struct is_nothrow_destructible : bool_constant<__is_nothrow_destructible(T)> {};
template<typename T> struct has_virtual_destructor : bool_constant<__has_virtual_destructor(T)> {};

template<typename T> struct is_default_constructible : is_constructible<T> {};
template<typename T> struct is_trivially_default_constructible : is_trivially_constructible<T> {};
template<typename T> struct is_nothrow_default_constructible : is_nothrow_constructible<T> {};

template<typename T> struct is_copy_constructible : is_constructible<T, add_lvalue_reference_t<const T>> {};
template<typename T> struct is_trivially_copy_constructible : is_trivially_constructible<T, add_lvalue_reference_t<const T>> {};
template<typename T> struct is_nothrow_copy_constructible : is_nothrow_constructible<T, add_lvalue_reference_t<const T>> {};
template<typename T> struct is_move_constructible : is_constructible<T, add_rvalue_reference_t<T>> {};
template<typename T> struct is_trivially_move_constructible : is_trivially_constructible<T, add_rvalue_reference_t<T>> {};
template<typename T> struct is_nothrow_move_constructible : is_nothrow_constructible<T, add_rvalue_reference_t<T>> {};

// --- Derived Assignability ---
template<typename T> struct is_copy_assignable : is_assignable<add_lvalue_reference_t<T>, add_lvalue_reference_t<const T>> {};
template<typename T> struct is_trivially_copy_assignable : is_trivially_assignable<add_lvalue_reference_t<T>, add_lvalue_reference_t<const T>> {};
template<typename T> struct is_nothrow_copy_assignable : is_nothrow_assignable<add_lvalue_reference_t<T>, add_lvalue_reference_t<const T>> {};
template<typename T> struct is_move_assignable : is_assignable<add_lvalue_reference_t<T>, add_rvalue_reference_t<T>> {};
template<typename T> struct is_trivially_move_assignable : is_trivially_assignable<add_lvalue_reference_t<T>, add_rvalue_reference_t<T>> {};
template<typename T> struct is_nothrow_move_assignable : is_nothrow_assignable<add_lvalue_reference_t<T>, add_rvalue_reference_t<T>> {};

namespace detail {
    using sp::swap; // Fallback to standard swap if available

    template<typename T, typename U, typename = void> struct is_swappable_with_h : false_type {};
    template<typename T, typename U> struct is_swappable_with_h<T, U, decltype(swap(spt::declval<T>(), spt::declval<U>()))> : true_type {};
    template<typename T, typename U, typename = void> struct is_nothrow_swappable_with_h : false_type {};
    template<typename T, typename U> struct is_nothrow_swappable_with_h<T, U, decltype(swap(spt::declval<T>(), spt::declval<U>()))> 
        : bool_constant<noexcept(swap(spt::declval<T>(), spt::declval<U>()))> {};
}

template<typename T, typename U> struct is_swappable_with : detail::is_swappable_with_h<T, U> {};
template<typename T> struct is_swappable : detail::is_swappable_with_h<add_lvalue_reference_t<T>, add_lvalue_reference_t<T>> {};
template<typename T, typename U> struct is_nothrow_swappable_with : detail::is_nothrow_swappable_with_h<T, U> {};
template<typename T> struct is_nothrow_swappable : detail::is_nothrow_swappable_with_h<add_lvalue_reference_t<T>, add_lvalue_reference_t<T>> {};

template<typename T, typename... Args> inline constexpr bool is_constructible_v                  = is_constructible<T, Args...>::value;
template<typename T>                   inline constexpr bool is_default_constructible_v          = is_default_constructible<T>::value;
template<typename T>                   inline constexpr bool is_copy_constructible_v             = is_copy_constructible<T>::value;
template<typename T>                   inline constexpr bool is_move_constructible_v             = is_move_constructible<T>::value;
template<typename T, typename U>       inline constexpr bool is_assignable_v                     = is_assignable<T, U>::value;
template<typename T>                   inline constexpr bool is_copy_assignable_v                = is_copy_assignable<T>::value;
template<typename T>                   inline constexpr bool is_move_assignable_v                = is_move_assignable<T>::value;
template<typename T, typename U>       inline constexpr bool is_swappable_with_v                 = is_swappable_with<T, U>::value;
template<typename T>                   inline constexpr bool is_swappable_v                      = is_swappable<T>::value;
template<typename T>                   inline constexpr bool is_destructible_v                   = is_destructible<T>::value;
template<typename T, typename... Args> inline constexpr bool is_trivially_constructible_v        = is_trivially_constructible<T, Args...>::value;
template<typename T>                   inline constexpr bool is_trivially_default_constructible_v= is_trivially_default_constructible<T>::value;
template<typename T>                   inline constexpr bool is_trivially_copy_constructible_v   = is_trivially_copy_constructible<T>::value;
template<typename T>                   inline constexpr bool is_trivially_move_constructible_v   = is_trivially_move_constructible<T>::value;
template<typename T, typename U>       inline constexpr bool is_trivially_assignable_v           = is_trivially_assignable<T, U>::value;
template<typename T>                   inline constexpr bool is_trivially_copy_assignable_v      = is_trivially_copy_assignable<T>::value;
template<typename T>                   inline constexpr bool is_trivially_move_assignable_v      = is_trivially_move_assignable<T>::value;
template<typename T>                   inline constexpr bool is_trivially_destructible_v         = is_trivially_destructible<T>::value;
template<typename T, typename... Args> inline constexpr bool is_nothrow_constructible_v          = is_nothrow_constructible<T, Args...>::value;
template<typename T>                   inline constexpr bool is_nothrow_default_constructible_v  = is_nothrow_default_constructible<T>::value;
template<typename T>                   inline constexpr bool is_nothrow_copy_constructible_v     = is_nothrow_copy_constructible<T>::value;
template<typename T>                   inline constexpr bool is_nothrow_move_constructible_v     = is_nothrow_move_constructible<T>::value;
template<typename T, typename U>       inline constexpr bool is_nothrow_assignable_v             = is_nothrow_assignable<T, U>::value;
template<typename T>                   inline constexpr bool is_nothrow_copy_assignable_v        = is_nothrow_copy_assignable<T>::value;
template<typename T>                   inline constexpr bool is_nothrow_move_assignable_v        = is_nothrow_move_assignable<T>::value;
template<typename T, typename U>       inline constexpr bool is_nothrow_swappable_with_v         = is_nothrow_swappable_with<T, U>::value;
template<typename T>                   inline constexpr bool is_nothrow_swappable_v              = is_nothrow_swappable<T>::value;
template<typename T>                   inline constexpr bool is_nothrow_destructible_v           = is_nothrow_destructible<T>::value;
template<typename T>                   inline constexpr bool has_virtual_destructor_v            = has_virtual_destructor<T>::value;
// ============================================================
// SECTION 8: TYPE RELATIONSHIPS
// ============================================================

template<typename T, typename U> struct is_same : false_type {};
template<typename T> struct is_same<T, T> : true_type{};
template<typename Base, typename Derived> struct is_base_of : bool_constant<__is_base_of(Base, Derived)> {};
template<typename From, typename To> struct is_convertible : bool_constant<__is_convertible(From, To)> {};
template<typename From, typename To> struct is_nothrow_convertible : bool_constant<__is_nothrow_convertible(From, To)> {};
template<typename T, typename U> struct is_layout_compatible : bool_constant<__is_layout_compatible(T, U)> {};
namespace detail {
    template<typename B, typename D>
    auto test_virtual_base(int) -> bool_constant<
        __is_base_of(B, D) && !__is_convertible(D*, B*) && !__is_convertible(const volatile D*, const volatile B*)
    >;
    template<typename B, typename D>
    auto test_virtual_base(...) -> false_type;
}

template<typename Base, typename Derived> struct is_virtual_base_of : decltype(detail::test_virtual_base<Base, Derived>(0)) {};

template<typename T, typename U> inline constexpr bool is_same_v              = is_same<T, U>::value;
template<typename Base, typename Derived> inline constexpr bool is_base_of_v  = is_base_of<Base, Derived>::value;
template<typename From, typename To> inline constexpr bool is_convertible_v   = is_convertible<From, To>::value;
template<typename From, typename To> inline constexpr bool is_nothrow_convertible_v = is_nothrow_convertible<From, To>::value;
template<typename T, typename U>     inline constexpr bool is_layout_compatible_v   = is_layout_compatible<T, U>::value;

// ============================================================
// SECTION 9: TYPE TRANSFORMATIONS
// ============================================================

// --- Sign manipulation ---
template<typename T> struct make_signed;
template<typename T> struct make_unsigned;
template<typename T> using make_signed_t   = typename make_signed<T>::type;
template<typename T> using make_unsigned_t = typename make_unsigned<T>::type;

// --- Decay (array/function decay + cv strip) ---
template<typename T> struct decay {
private:
    using U = typename remove_reference<T>::type;
public:
    using type = conditional_t<is_array_v<U>,
                     typename remove_extent<U>::type*,
                     conditional_t<is_function_v<U>,
                         typename add_pointer<U>::type,
                         typename remove_cv<U>::type>>;
};
template<typename T> using decay_t = typename decay<T>::type;

// --- Common type ---
template<typename... Ts> struct common_type;
template<typename... Ts> using common_type_t = typename common_type<Ts...>::type;

// --- Common reference ---
template<typename... Ts> struct common_reference;
template<typename... Ts> using common_reference_t = typename common_reference<Ts...>::type;

// --- Underlying type (for enums) ---
template<typename T> struct underlying_type;
template<typename T> using underlying_type_t = typename underlying_type<T>::type;

// --- Invoke result ---
template<typename F, typename... Args> struct invoke_result;
template<typename F, typename... Args> using invoke_result_t = typename invoke_result<F, Args...>::type;

// --- Result of (deprecated alias kept for compat) ---
template<typename F, typename... Args>
using result_of_t = invoke_result_t<F, Args...>;

// --- Aligned storage / union ---
template<size_t Len, size_t Align> struct aligned_storage;
template<size_t Len, typename... Ts> struct aligned_union;
template<size_t Len, size_t Align> using aligned_storage_t = typename aligned_storage<Len, Align>::type;
template<size_t Len, typename... Ts> using aligned_union_t  = typename aligned_union<Len, Ts...>::type;

// --- Alignment ---
template<typename T> struct alignment_of;
template<typename T> inline constexpr size_t alignment_of_v = alignment_of<T>::value;

// ============================================================
// SECTION 10: PACK / VARIADIC UTILITIES
// ============================================================

// --- Pack size ---
template<typename... Ts>
struct pack_size;

template<typename... Ts>
inline constexpr size_t pack_size_v = pack_size<Ts...>::value;

// --- Nth type in pack ---
template<size_t N, typename... Ts>
struct pack_element;

template<size_t N, typename... Ts>
using pack_element_t = typename pack_element<N, Ts...>::type;

// --- Type index in pack (first match) ---
template<typename T, typename... Ts>
struct pack_index;

template<typename T, typename... Ts>
inline constexpr size_t pack_index_v = pack_index<T, Ts...>::value;

// --- Check if type is in pack ---
template<typename T, typename... Ts>
struct is_one_of;

template<typename T, typename... Ts>
inline constexpr bool is_one_of_v = is_one_of<T, Ts...>::value;

// --- All / any / none over a predicate applied to pack ---
template<template<typename> class Pred, typename... Ts>
struct all_of_pack;

template<template<typename> class Pred, typename... Ts>
struct any_of_pack;

template<template<typename> class Pred, typename... Ts>
struct none_of_pack;

template<template<typename> class Pred, typename... Ts>
inline constexpr bool all_of_pack_v  = all_of_pack<Pred, Ts...>::value;

template<template<typename> class Pred, typename... Ts>
inline constexpr bool any_of_pack_v  = any_of_pack<Pred, Ts...>::value;

template<template<typename> class Pred, typename... Ts>
inline constexpr bool none_of_pack_v = none_of_pack<Pred, Ts...>::value;

// --- Type list ---
template<typename... Ts>
struct type_list {};

// --- Concatenate type lists ---
template<typename ListA, typename ListB>
struct type_list_cat;

template<typename ListA, typename ListB>
using type_list_cat_t = typename type_list_cat<ListA, ListB>::type;

// --- Filter type list by predicate ---
template<template<typename> class Pred, typename List>
struct type_list_filter;

template<template<typename> class Pred, typename List>
using type_list_filter_t = typename type_list_filter<Pred, List>::type;

// --- Transform type list by mapping metafunction ---
template<template<typename> class Map, typename List>
struct type_list_transform;

template<template<typename> class Map, typename List>
using type_list_transform_t = typename type_list_transform<Map, List>::type;

// ============================================================
// SECTION 11: INVOCABILITY & CALLABLE TRAITS
// ============================================================

template<typename F, typename... Args> struct is_invocable;
template<typename R, typename F, typename... Args> struct is_invocable_r;
template<typename F, typename... Args> struct is_nothrow_invocable;
template<typename R, typename F, typename... Args> struct is_nothrow_invocable_r;

template<typename F, typename... Args> inline constexpr bool is_invocable_v          = is_invocable<F, Args...>::value;
template<typename R, typename F, typename... Args> inline constexpr bool is_invocable_r_v = is_invocable_r<R, F, Args...>::value;
template<typename F, typename... Args> inline constexpr bool is_nothrow_invocable_v  = is_nothrow_invocable<F, Args...>::value;
template<typename R, typename F, typename... Args> inline constexpr bool is_nothrow_invocable_r_v = is_nothrow_invocable_r<R, F, Args...>::value;

// --- Decompose callable signature ---
template<typename F>
struct function_traits; // specialisations for: plain fn, fn ptr, member fn ptr, functor/lambda

// Convenience aliases derived from function_traits<F>
template<typename F> using function_return_t     = typename function_traits<F>::return_type;
template<typename F> using function_args_t       = typename function_traits<F>::args_type;   // type_list<...>
template<typename F> inline constexpr size_t function_arity_v = function_traits<F>::arity;

// ============================================================
// SECTION 12: NUMERIC / ARITHMETIC TRAITS
// ============================================================

// ============================================================================
// 1. is_integral
// ============================================================================

template <typename T> struct is_integral_helper : false_type {};

template <> struct is_integral_helper<bool>               : true_type {};
template <> struct is_integral_helper<char>               : true_type {};
template <> struct is_integral_helper<signed char>        : true_type {};
template <> struct is_integral_helper<unsigned char>      : true_type {};
#if __cplusplus >= 202002L
template <> struct is_integral_helper<char8_t>            : true_type {};
#endif
template <> struct is_integral_helper<char16_t>           : true_type {};
template <> struct is_integral_helper<char32_t>           : true_type {};
template <> struct is_integral_helper<wchar_t>            : true_type {};
template <> struct is_integral_helper<short>              : true_type {};
template <> struct is_integral_helper<unsigned short>     : true_type {};
template <> struct is_integral_helper<int>                : true_type {};
template <> struct is_integral_helper<unsigned int>       : true_type {};
template <> struct is_integral_helper<long>               : true_type {};
template <> struct is_integral_helper<unsigned long>      : true_type {};
template <> struct is_integral_helper<long long>          : true_type {};
template <> struct is_integral_helper<unsigned long long> : true_type {};

template <typename T>
struct is_integral : is_integral_helper<remove_cv_t<T>> {};

// ============================================================================
// 2. is_floating_point
// ============================================================================

template <typename T> struct is_floating_point_helper : false_type {};

template <> struct is_floating_point_helper<float>       : true_type {};
template <> struct is_floating_point_helper<double>      : true_type {};
template <> struct is_floating_point_helper<long double> : true_type {};

template <typename T>
struct is_floating_point : is_floating_point_helper<remove_cv_t<T>> {};

// ============================================================================
// 3. is_bool
// ============================================================================

template <typename T> struct is_bool_helper : false_type {};
template <>          struct is_bool_helper<bool> : true_type {};

template <typename T>
struct is_bool : is_bool_helper<remove_cv_t<T>> {};

// ============================================================================
// 4. is_integer (integral but not bool)
// ============================================================================

template <typename T>
struct is_integer 
    : integral_constant<bool, is_integral<T>::value && !is_bool<T>::value> {};

// ============================================================================
// 5. is_character
// ============================================================================

template <typename T> struct is_character_helper : false_type {};

template <> struct is_character_helper<char>      : true_type {};
template <> struct is_character_helper<wchar_t>   : true_type {};
#if __cplusplus >= 202002L
template <> struct is_character_helper<char8_t>   : true_type {};
#endif
template <> struct is_character_helper<char16_t>  : true_type {};
template <> struct is_character_helper<char32_t>  : true_type {};

template <typename T>
struct is_character : is_character_helper<remove_cv_t<T>> {};

// ============================================================================
// 6. is_numeric (is_arithmetic && !is_bool)
// ============================================================================

template <typename T>
struct is_numeric 
    : integral_constant<bool, (is_integral<T>::value || is_floating_point<T>::value) && !is_bool<T>::value> {};

// ============================================================================
// Variable Templates
// ============================================================================

template <typename T> inline constexpr bool is_integral_v       = is_integral<T>::value;
template <typename T> inline constexpr bool is_floating_point_v = is_floating_point<T>::value;
template <typename T> inline constexpr bool is_integer_v        = is_integer<T>::value;
template <typename T> inline constexpr bool is_character_v      = is_character<T>::value;
template <typename T> inline constexpr bool is_bool_v           = is_bool<T>::value;
template <typename T> inline constexpr bool is_numeric_v        = is_numeric<T>::value;

// --- Power-of-two & size helpers (useful for allocators, SIMD, hashes) ---
template<size_t N> struct is_power_of_two;
template<size_t N> inline constexpr bool is_power_of_two_v = is_power_of_two<N>::value;

template<size_t N> struct next_power_of_two;
template<size_t N> inline constexpr size_t next_power_of_two_v = next_power_of_two<N>::value;

template<size_t N> struct log2_floor;
template<size_t N> inline constexpr size_t log2_floor_v = log2_floor<N>::value;

// --- Numeric limits reimplementation for sp types ---
template<typename T>
struct numeric_limits; // provide: min, max, lowest, digits, digits10, is_exact, epsilon, infinity, ...

// ============================================================
// SECTION 13: POINTER / MEMORY TRAITS
// ============================================================

template<typename T> struct is_pointer_to_const;
template<typename T> struct is_pointer_to_function;
template<typename T> struct is_smart_pointer;          // detects sp::unique_ptr, sp::shared_ptr, etc.
template<typename T> struct is_raw_pointer;            // is_pointer and !is_smart_pointer

template<typename T> inline constexpr bool is_pointer_to_const_v    = is_pointer_to_const<T>::value;
template<typename T> inline constexpr bool is_pointer_to_function_v = is_pointer_to_function<T>::value;
template<typename T> inline constexpr bool is_smart_pointer_v       = is_smart_pointer<T>::value;
template<typename T> inline constexpr bool is_raw_pointer_v         = is_raw_pointer<T>::value;

// --- Pointed-at type (strips one level of pointer or smart pointer) ---
template<typename T> struct pointee;
template<typename T> using pointee_t = typename pointee<T>::type;

// --- Byte size at compile time ---
template<typename T>
struct byte_size;

template<typename T>
inline constexpr size_t byte_size_v = byte_size<T>::value;

// ============================================================
// SECTION 14: ALLOCATOR TRAITS
// (complements utils/allocators.hpp)
// ============================================================

template<typename Alloc>
struct allocator_traits_ext; // extends spt::allocator_traits with sp-specific queries

template<typename Alloc> struct has_allocate_at_least;
template<typename Alloc> struct has_reallocate;
template<typename Alloc> struct has_aligned_allocate;
template<typename Alloc> struct allocator_is_stateless;
template<typename Alloc> struct allocator_is_thread_safe;

template<typename Alloc> inline constexpr bool has_allocate_at_least_v   = has_allocate_at_least<Alloc>::value;
template<typename Alloc> inline constexpr bool has_reallocate_v           = has_reallocate<Alloc>::value;
template<typename Alloc> inline constexpr bool has_aligned_allocate_v     = has_aligned_allocate<Alloc>::value;
template<typename Alloc> inline constexpr bool allocator_is_stateless_v   = allocator_is_stateless<Alloc>::value;
template<typename Alloc> inline constexpr bool allocator_is_thread_safe_v = allocator_is_thread_safe<Alloc>::value;

// ============================================================
// SECTION 15: CONTAINER / ITERATOR TRAITS
// (complements containers/*.hpp)
// ============================================================

template<typename T> struct is_container;          // has begin/end/size/value_type
template<typename T> struct is_contiguous_container; // data() returns raw pointer
template<typename T> struct is_associative_container;
template<typename T> struct is_sequence_container;
template<typename T> struct is_hashable_container;

template<typename T> inline constexpr bool is_container_v            = is_container<T>::value;
template<typename T> inline constexpr bool is_contiguous_container_v = is_contiguous_container<T>::value;
template<typename T> inline constexpr bool is_associative_container_v= is_associative_container<T>::value;
template<typename T> inline constexpr bool is_sequence_container_v   = is_sequence_container<T>::value;
template<typename T> inline constexpr bool is_hashable_container_v   = is_hashable_container<T>::value;

// --- Container value type ---
template<typename T> struct container_value_type;
template<typename T> using container_value_type_t = typename container_value_type<T>::type;

// --- Iterator category ---
template<typename Iter> struct iterator_category;
template<typename Iter> using iterator_category_t = typename iterator_category<Iter>::type;

template<typename Iter> struct is_input_iterator;
template<typename Iter> struct is_output_iterator;
template<typename Iter> struct is_forward_iterator;
template<typename Iter> struct is_bidirectional_iterator;
template<typename Iter> struct is_random_access_iterator;
template<typename Iter> struct is_contiguous_iterator;

template<typename Iter> inline constexpr bool is_input_iterator_v         = is_input_iterator<Iter>::value;
template<typename Iter> inline constexpr bool is_forward_iterator_v       = is_forward_iterator<Iter>::value;
template<typename Iter> inline constexpr bool is_bidirectional_iterator_v = is_bidirectional_iterator<Iter>::value;
template<typename Iter> inline constexpr bool is_random_access_iterator_v = is_random_access_iterator<Iter>::value;
template<typename Iter> inline constexpr bool is_contiguous_iterator_v    = is_contiguous_iterator<Iter>::value;

// --- Range detection (for range-based algorithms) ---
template<typename T> struct is_range;
template<typename T> struct is_sized_range;
template<typename T> struct is_contiguous_range;

template<typename T> inline constexpr bool is_range_v            = is_range<T>::value;
template<typename T> inline constexpr bool is_sized_range_v      = is_sized_range<T>::value;
template<typename T> inline constexpr bool is_contiguous_range_v = is_contiguous_range<T>::value;

// ============================================================
// SECTION 16: SIMD TRAITS
// (complements utils/SIMD.hpp)
// ============================================================

template<typename T> struct is_simd_type;          // sp::simd<T, Width> or platform intrinsic wrapper
template<typename T> struct is_simd_mask;
template<typename T> struct simd_element_type;
template<typename T> struct simd_width;
template<typename T> struct simd_byte_size;
template<typename T> struct is_simd_compatible;    // T can be a lane element in any SIMD register

template<typename T> inline constexpr bool   is_simd_type_v       = is_simd_type<T>::value;
template<typename T> inline constexpr bool   is_simd_mask_v       = is_simd_mask<T>::value;
template<typename T> inline constexpr size_t simd_width_v         = simd_width<T>::value;
template<typename T> inline constexpr size_t simd_byte_size_v     = simd_byte_size<T>::value;
template<typename T> inline constexpr bool   is_simd_compatible_v = is_simd_compatible<T>::value;

template<typename T> using simd_element_type_t = typename simd_element_type<T>::type;

// --- Optimal SIMD width for element type T on target platform ---
template<typename T> struct native_simd_width;
template<typename T> inline constexpr size_t native_simd_width_v = native_simd_width<T>::value;

// ============================================================
// SECTION 17: HASH TRAITS
// (complements utils/hashes.hpp)
// ============================================================

template<typename T> struct is_hashable;           // sp::hash<T> is well-formed
template<typename T> struct is_trivially_hashable; // safe to hash by raw byte reinterpretation
template<typename T> struct hash_result_type;      // result type of sp::hash<T>{}(val)

template<typename T> inline constexpr bool is_hashable_v          = is_hashable<T>::value;
template<typename T> inline constexpr bool is_trivially_hashable_v= is_trivially_hashable<T>::value;
template<typename T> using hash_result_type_t = typename hash_result_type<T>::type;

// ============================================================
// SECTION 18: SERIALIZATION / IO TRAITS
// (complements utils/io.hpp, containers/files.hpp)
// ============================================================

template<typename T> struct is_serializable;       // sp serialize protocol
template<typename T> struct is_deserializable;
template<typename T> struct is_trivially_serializable; // memcpy-safe on disk

template<typename T> inline constexpr bool is_serializable_v          = is_serializable<T>::value;
template<typename T> inline constexpr bool is_deserializable_v        = is_deserializable<T>::value;
template<typename T> inline constexpr bool is_trivially_serializable_v= is_trivially_serializable<T>::value;

// ============================================================
// SECTION 19: THREAD / CONCURRENCY TRAITS
// (complements containers/thread.hpp)
// ============================================================

template<typename T> struct is_mutex;
template<typename T> struct is_lockable;
template<typename T> struct is_shared_lockable;
template<typename T> struct is_timed_lockable;
template<typename T> struct is_atomic;
template<typename T> struct is_atomic_lockfree;
template<typename T> struct is_thread_safe;        // user-opt-in tag

template<typename T> inline constexpr bool is_mutex_v          = is_mutex<T>::value;
template<typename T> inline constexpr bool is_lockable_v       = is_lockable<T>::value;
template<typename T> inline constexpr bool is_shared_lockable_v= is_shared_lockable<T>::value;
template<typename T> inline constexpr bool is_timed_lockable_v = is_timed_lockable<T>::value;
template<typename T> inline constexpr bool is_atomic_v         = is_atomic<T>::value;
template<typename T> inline constexpr bool is_atomic_lockfree_v= is_atomic_lockfree<T>::value;
template<typename T> inline constexpr bool is_thread_safe_v    = is_thread_safe<T>::value;

// ============================================================
// SECTION 20: GPU TRAITS
// (complements GPU/ headers, guarded by __SP_USE_GPU__)
// ============================================================

#ifdef __SP_USE_GPU__

template<typename T> struct is_gpu_buffer;         // device-side buffer type
template<typename T> struct is_gpu_kernel;         // callable on device
template<typename T> struct is_gpu_compatible;     // trivially copyable + no host ptrs
template<typename T> struct is_unified_memory;     // accessible from host and device

template<typename T> inline constexpr bool is_gpu_buffer_v       = is_gpu_buffer<T>::value;
template<typename T> inline constexpr bool is_gpu_kernel_v       = is_gpu_kernel<T>::value;
template<typename T> inline constexpr bool is_gpu_compatible_v   = is_gpu_compatible<T>::value;
template<typename T> inline constexpr bool is_unified_memory_v   = is_unified_memory<T>::value;

#endif // __SP_USE_GPU__

// ============================================================
// SECTION 21: NETWORK TRAITS
// (complements containers/network.hpp)
// ============================================================

template<typename T> struct is_network_message;    // serializable over the wire
template<typename T> struct is_socket_type;
template<typename T> struct is_endpoint_type;
template<typename T> struct is_protocol_type;
template<typename T> struct is_fixed_size_message; // known byte count at compile time

template<typename T> inline constexpr bool is_network_message_v  = is_network_message<T>::value;
template<typename T> inline constexpr bool is_socket_type_v      = is_socket_type<T>::value;
template<typename T> inline constexpr bool is_endpoint_type_v    = is_endpoint_type<T>::value;
template<typename T> inline constexpr bool is_protocol_type_v    = is_protocol_type<T>::value;
template<typename T> inline constexpr bool is_fixed_size_message_v = is_fixed_size_message<T>::value;

// ============================================================
// SECTION 22: STRING / CHARACTER TRAITS
// (complements containers/string.hpp)
// ============================================================

template<typename T> struct is_char_type;          // any character type
template<typename T> struct is_string_type;        // sp::string, spt::string, const char*, string_view
template<typename T> struct is_string_view_type;
template<typename T> struct is_c_string;           // T is char* or const char*
template<typename T> struct char_type_of;          // element type of a string type

template<typename T> inline constexpr bool is_char_type_v       = is_char_type<T>::value;
template<typename T> inline constexpr bool is_string_type_v     = is_string_type<T>::value;
template<typename T> inline constexpr bool is_string_view_type_v= is_string_view_type<T>::value;
template<typename T> inline constexpr bool is_c_string_v        = is_c_string<T>::value;
template<typename T> using char_type_of_t = typename char_type_of<T>::type;

// ============================================================
// SECTION 23: TENSOR TRAITS
// (complements containers/tensor.hpp)
// ============================================================

template<typename T> struct is_tensor;
template<typename T> struct tensor_rank;
template<typename T> struct tensor_element_type;
template<typename T> struct tensor_is_contiguous;
template<typename T> struct tensor_is_owning;

template<typename T> inline constexpr bool   is_tensor_v              = is_tensor<T>::value;
template<typename T> inline constexpr size_t tensor_rank_v            = tensor_rank<T>::value;
template<typename T> inline constexpr bool   tensor_is_contiguous_v   = tensor_is_contiguous<T>::value;
template<typename T> inline constexpr bool   tensor_is_owning_v       = tensor_is_owning<T>::value;

template<typename T> using tensor_element_type_t = typename tensor_element_type<T>::type;

// ============================================================
// SECTION 24: PAIR / TUPLE TRAITS
// (complements containers/pair.hpp)
// ============================================================

template<typename T>              struct is_pair;
template<typename T>              struct is_tuple_like;   // has spt::get and tuple_size
template<size_t I, typename T>    struct tuple_element_at;
template<typename T>              struct tuple_size_of;

template<typename T>              inline constexpr bool   is_pair_v       = is_pair<T>::value;
template<typename T>              inline constexpr bool   is_tuple_like_v = is_tuple_like<T>::value;
template<typename T>              inline constexpr size_t tuple_size_of_v = tuple_size_of<T>::value;
template<size_t I, typename T>    using tuple_element_at_t = typename tuple_element_at<I, T>::type;

// ============================================================
// SECTION 25: EXCEPTION TRAITS
// (complements utils/exceptions.hpp)
// ============================================================

template<typename T> struct is_exception;           // derives from sp::exception or spt::exception
template<typename T> struct is_sp_exception;        // sp-native exception hierarchy
template<typename From, typename To> struct is_exception_convertible; // for catch covariance

template<typename T> inline constexpr bool is_exception_v    = is_exception<T>::value;
template<typename T> inline constexpr bool is_sp_exception_v = is_sp_exception<T>::value;

// ============================================================
// SECTION 26: DETECTION IDIOM (library fundamentals v2 style)
// ============================================================

struct nonesuch {
    ~nonesuch() = delete;
    nonesuch(nonesuch const&) = delete;
    void operator=(nonesuch const&) = delete;
};

namespace detail {
    // 1. Primary template: This handles the FAILURE case.
    // If Op<Args...> is invalid, SFINAE kicks us here.
    template<typename Default, typename Void, template<typename...> class Op, typename... Args>
    struct detector {
        using value_t = spt::false_type;
        using type    = Default;
    };

    // 2. Partial specialization: This handles the SUCCESS case.
    // If Op<Args...> is valid, std::void_t makes this a match!
    template<typename Default, template<typename...> class Op, typename... Args>
    struct detector<Default, spt::void_t<Op<Args...>>, Op, Args...> {
        using value_t = spt::true_type;
        using type    = Op<Args...>;
    };
} // namespace detail

template<template<typename...> class Op, typename... Args>
using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

template<template<typename...> class Op, typename... Args>
using detected_t  = typename detail::detector<nonesuch, void, Op, Args...>::type;

template<typename Default, template<typename...> class Op, typename... Args>
using detected_or = detail::detector<Default, void, Op, Args...>;

template<template<typename...> class Op, typename... Args>
inline constexpr bool is_detected_v = is_detected<Op, Args...>::value;

/*template<typename T>
add_rvalue_reference_t<T> declval() noexcept;*/

// --- Common detection expressions ---
template<typename T> using has_size_expr        = decltype(spt::declval<T>().size());
template<typename T> using has_data_expr        = decltype(spt::declval<T>().data());
template<typename T> using has_begin_expr       = decltype(spt::declval<T>().begin());
template<typename T> using has_end_expr         = decltype(spt::declval<T>().end());
template<typename T> using has_reserve_expr     = decltype(spt::declval<T>().reserve(0));
template<typename T> using has_resize_expr      = decltype(spt::declval<T>().resize(0));
template<typename T> using has_push_back_expr   = decltype(spt::declval<T>().push_back(spt::declval<typename T::value_type>()));
template<typename T> using has_emplace_back_expr= decltype(spt::declval<T>().emplace_back());
template<typename T> using has_capacity_expr    = decltype(spt::declval<T>().capacity());
template<typename T> using has_clear_expr       = decltype(spt::declval<T>().clear());
template<typename T> using has_find_expr        = decltype(spt::declval<T>().find(spt::declval<typename T::key_type>()));
//template<typename T> using has_hash_expr        = decltype(sp::hash<T>{}(spt::declval<T>()));
template<typename T> using has_to_string_expr   = decltype(spt::declval<T>().to_string());
template<typename T> using has_clone_expr       = decltype(spt::declval<T>().clone());
//template<typename T> using has_serialize_expr   = decltype(spt::declval<T>().serialize(spt::declval<sp::byte_stream&>()));
//template<typename T> using has_deserialize_expr = decltype(T::deserialize(spt::declval<sp::byte_stream&>()));
template<typename T, typename U>
                     using has_eq_expr          = decltype(spt::declval<T>() == spt::declval<U>());
template<typename T, typename U>
                     using has_lt_expr          = decltype(spt::declval<T>() < spt::declval<U>());

template<typename T> struct has_size            : is_detected<has_size_expr, T> {};
template<typename T> struct has_data            : is_detected<has_data_expr, T> {};
template<typename T> struct has_begin           : is_detected<has_begin_expr, T> {};
template<typename T> struct has_end             : is_detected<has_end_expr, T> {};
template<typename T> struct has_reserve         : is_detected<has_reserve_expr, T> {};
template<typename T> struct has_resize          : is_detected<has_resize_expr, T> {};
template<typename T> struct has_push_back       : is_detected<has_push_back_expr, T> {};
template<typename T> struct has_emplace_back    : is_detected<has_emplace_back_expr, T> {};
template<typename T> struct has_capacity        : is_detected<has_capacity_expr, T> {};
template<typename T> struct has_clear           : is_detected<has_clear_expr, T> {};
template<typename T> struct has_find            : is_detected<has_find_expr, T> {};
//template<typename T> struct has_hash            : is_detected<has_hash_expr, T> {};
template<typename T> struct has_to_string       : is_detected<has_to_string_expr, T> {};
template<typename T> struct has_clone           : is_detected<has_clone_expr, T> {};
//template<typename T> struct has_serialize       : is_detected<has_serialize_expr, T> {};
//template<typename T> struct has_deserialize     : is_detected<has_deserialize_expr, T> {};

template<typename T> inline constexpr bool has_size_v         = has_size<T>::value;
template<typename T> inline constexpr bool has_data_v         = has_data<T>::value;
template<typename T> inline constexpr bool has_begin_v        = has_begin<T>::value;
template<typename T> inline constexpr bool has_end_v          = has_end<T>::value;
template<typename T> inline constexpr bool has_reserve_v      = has_reserve<T>::value;
template<typename T> inline constexpr bool has_resize_v       = has_resize<T>::value;
template<typename T> inline constexpr bool has_push_back_v    = has_push_back<T>::value;
template<typename T> inline constexpr bool has_emplace_back_v = has_emplace_back<T>::value;
template<typename T> inline constexpr bool has_capacity_v     = has_capacity<T>::value;
template<typename T> inline constexpr bool has_clear_v        = has_clear<T>::value;
template<typename T> inline constexpr bool has_find_v         = has_find<T>::value;
//template<typename T> inline constexpr bool has_hash_v         = has_hash<T>::value;
template<typename T> inline constexpr bool has_to_string_v    = has_to_string<T>::value;
template<typename T> inline constexpr bool has_clone_v        = has_clone<T>::value;
//template<typename T> inline constexpr bool has_serialize_v    = has_serialize<T>::value;
//template<typename T> inline constexpr bool has_deserialize_v  = has_deserialize<T>::value;

// ============================================================
// SECTION 27: UTILITY METAFUNCTIONS
// ============================================================

// --- declval (needed before <utility> is available in freestanding) ---

// --- always_false (for static_assert in constexpr if branches) ---
template<typename...>
inline constexpr bool always_false = false;

// --- always_true ---
template<typename...>
inline constexpr bool always_true = true;

// --- copy_cv: apply CV of Src onto Dst ---
template<typename Src, typename Dst>
struct copy_cv;

template<typename Src, typename Dst>
using copy_cv_t = typename copy_cv<Src, Dst>::type;

// --- copy_ref: apply reference category of Src onto Dst ---
template<typename Src, typename Dst>
struct copy_ref;

template<typename Src, typename Dst>
using copy_ref_t = typename copy_ref<Src, Dst>::type;

// --- copy_cvref ---
template<typename Src, typename Dst>
struct copy_cvref;

template<typename Src, typename Dst>
using copy_cvref_t = typename copy_cvref<Src, Dst>::type;

// --- Forward-as: conditional move vs copy ---
template<typename T>
struct forward_as;

// --- Lazy evaluation wrapper (prevents instantiation of costly types) ---
template<template<typename...> class F, typename... Args>
struct lazy;

template<template<typename...> class F, typename... Args>
using lazy_t = typename lazy<F, Args...>::type;

// --- Compile-time integer sequence helpers ---
template<typename T, T... Is>
struct integer_sequence;

template<size_t... Is>
using index_sequence = integer_sequence<size_t, Is...>;

template<typename T, T N>
using make_integer_sequence = /* compiler intrinsic / recursive impl */ integer_sequence<T>;

template<size_t N>
using make_index_sequence = make_integer_sequence<size_t, N>;

template<typename... Ts>
using index_sequence_for = make_index_sequence<sizeof...(Ts)>;

// --- Overloaded (for spt::visit-style lambdas) ---
template<typename... Fs>
struct overloaded;

// ============================================================
// SECTION EXTRA: SPIRAL EXTENSIONS (STL-FREE DETECTION)
// ============================================================

template <typename T, typename = void_t<>>
struct has_getSpiralMessage : false_type {};

template <typename T>
struct has_getSpiralMessage<T, void_t<decltype(spt::declval<T>().__getSpiralMessage())>> : true_type {};

template <typename T, typename = void_t<>>
struct has_getSpiralBinary : false_type {};

template <typename T>
struct has_getSpiralBinary<T, void_t<decltype(spt::declval<T>().__getSpiralBinary())>> : true_type {};

template <typename T, typename = void_t<>>
struct is_hashable_string : false_type {};

template <typename T>
struct is_hashable_string<T, void_t<decltype(spt::declval<T>().c_str())>> : true_type {};

template <class U>
struct is_init_list : spt::false_type {};

template <class U>
struct is_init_list<std::initializer_list<U>> : spt::true_type {};

template <typename T, typename = void>
struct has_contiguous_storage : spt::false_type {};

template <typename T>
struct has_contiguous_storage<T, spt::void_t<
    decltype(spt::declval<T>().data()),
    decltype(spt::declval<T>().size())
>> : spt::true_type {};

template <typename Class, auto FuncPtr, typename Void, typename... Args>
struct has_function : spt::false_type {};

template <typename Class, auto FuncPtr, typename... Args>
struct has_function<
    Class, 
    FuncPtr, 
    spt::void_t<decltype((spt::declval<Class>().*FuncPtr)(spt::declval<Args>()...))>, 
    Args...
> : spt::true_type {};

// Primary template for detection
/*template <typename, template <typename...> class Op, typename... Args>
struct is_detected_impl {
    using value_type = bool;
    static constexpr bool value = false;
};

// Specialization that matches if Op<Args...> is a valid expression
template <template <typename...> class Op, typename... Args>
struct is_detected_impl<void_t<Op<Args...>>, Op, Args...> {
    using value_type = bool;
    static constexpr bool value = true;
};*/

template <typename T>
inline constexpr bool has_getSpiralMessage_v = has_getSpiralMessage<T>::value;

template <typename T>
inline constexpr bool  has_getSpiralBinary_v = has_getSpiralBinary<T>::value;

template <typename T>
inline constexpr bool  is_hashable_string_v = is_hashable_string<T>::value;

template <class U>
inline constexpr bool is_init_list_v = is_init_list<U>::value;

template <typename T>
inline constexpr bool has_contiguous_storage_v = has_contiguous_storage<T>::value;

template <typename Class, auto FuncPtr, typename... Args>
inline constexpr bool has_function_v = has_function<Class, FuncPtr, void, Args...>::value;

/*template <template <typename...> class Op, typename... Args>
inline constexpr bool is_detected_v = is_detected_impl<void, Op, Args...>::value;*/

// Put this in your type_traits.hpp once:
#define SP_DEFINE_METHOD_CHECKER(Method) \
    namespace detail { \
        template <typename T, typename = void> struct has_##Method : false_type {}; \
        template <typename T> struct has_##Method<T, void_t<decltype(&T::Method)>> : true_type {}; \
    }

#define SP_HAS_METHOD(Class, Method) spt::detail::has_##Method<Class>::value

SP_DEFINE_METHOD_CHECKER(with_cap);
SP_DEFINE_METHOD_CHECKER(c_str);
SP_DEFINE_METHOD_CHECKER(data);
SP_DEFINE_METHOD_CHECKER(size);
SP_DEFINE_METHOD_CHECKER(is_aligned);
SP_DEFINE_METHOD_CHECKER(to_string)

template <typename Alloc>
constexpr bool get_allocator_alignment() {
    if constexpr (SP_HAS_METHOD(Alloc, is_aligned)) {
        return Alloc::is_aligned();
    } else {
        return false;
    }
}


// ============================================================
// SECTION 28: CONCEPT-STYLE CONSTRAINTS
// (expressed as constexpr bool + enable_if helpers for pre-C++20 compat
//  and as actual concepts if __cpp_concepts is available)
// ============================================================

#if defined(__cpp_concepts) && __cpp_concepts >= 201907L

template<typename T>
concept Arithmetic = is_arithmetic_v<T>;

template<typename T>
concept Integral = is_integral_v<T>;

template<typename T>
concept FloatingPoint = is_floating_point_v<T>;

template<typename T>
concept Trivial = is_trivial_v<T>;

template<typename T>
concept TriviallyCopyable = is_trivially_copyable_v<T>;

template<typename T>
concept StandardLayout = is_standard_layout_v<T>;

template<typename T>
concept POD = Trivial<T> && StandardLayout<T>;

template<typename T>
concept Hashable = is_hashable_v<T>;

template<typename T>
concept TriviallyHashable = is_trivially_hashable_v<T>;

template<typename T>
concept Container = is_container_v<T>;

template<typename T>
concept ContiguousContainer = is_contiguous_container_v<T>;

template<typename T>
concept Serializable = is_serializable_v<T>;

template<typename T>
concept TriviallySerializable = is_trivially_serializable_v<T>;

template<typename T>
concept StringLike = is_string_type_v<T>;

template<typename T>
concept SIMDType = is_simd_type_v<T>;

template<typename T>
concept SIMDCompatible = is_simd_compatible_v<T>;

//template<typename T>
//concept Tensor = is_tensor_v<T>;

template<typename T>
concept Numeric = is_numeric_v<T>;

template<typename T>
concept Lockable = is_lockable_v<T>;

template<typename T, typename U>
concept SameAs = is_same_v<T, U>;

template<typename Derived, typename Base>
concept DerivedFrom = is_base_of_v<Base, Derived>;

template<typename From, typename To>
concept ConvertibleTo = is_convertible_v<From, To>;

template<typename T>
concept MoveConstructible = is_move_constructible_v<T>;

template<typename T>
concept CopyConstructible = is_copy_constructible_v<T>;

template<typename T>
concept DefaultConstructible = is_default_constructible_v<T>;

template<typename T>
concept Destructible = is_destructible_v<T>;

template<typename T>
concept MoveAssignable = is_move_assignable_v<T>;

template<typename T>
concept CopyAssignable = is_copy_assignable_v<T>;

template<typename T>
concept Swappable = is_swappable_v<T>;

template<typename F, typename... Args>
concept Invocable = is_invocable_v<F, Args...>;

template<typename F, typename R, typename... Args>
concept InvocableR = is_invocable_r_v<R, F, Args...>;

template<typename Iter>
concept InputIterator = is_input_iterator_v<Iter>;

template<typename Iter>
concept ForwardIterator = is_forward_iterator_v<Iter>;

template<typename Iter>
concept BidirectionalIterator = is_bidirectional_iterator_v<Iter>;

template<typename Iter>
concept RandomAccessIterator = is_random_access_iterator_v<Iter>;

template<typename Iter>
concept ContiguousIterator = is_contiguous_iterator_v<Iter>;

template<typename T>
concept Range = is_range_v<T>;

template<typename T>
concept SizedRange = is_sized_range_v<T>;

#ifdef __SP_USE_GPU__
template<typename T>
concept GPUCompatible = is_gpu_compatible_v<T>;
#endif

#endif // __cpp_concepts

}; // namespace sp
#endif // ____SP_TYPE_TRAITS____