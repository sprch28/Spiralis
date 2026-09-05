#ifndef ____SP_ALLOCATORS____
#define ____SP_ALLOCATORS____
#pragma once
#include "../setup/init.hpp"
#include "../core/type_traits.hpp"

#include <iostream>
#include <new>
#include <cstdlib>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <sys/mman.h>
#endif

#if defined(__APPLE__)
    #include <mach/arm/vm_param.h>
#endif


namespace sp{

class _spiral_alloc_traits{
public:
    static constexpr bool can_avoid_construct_on_trivially_copyable = true;
};


template <typename Alloc>
struct allocator_traits : public _spiral_alloc_traits{
    using allocator_type = Alloc;
    using value_type = typename Alloc::value_type;

    using pointer       = typename Alloc::value_type*; 
    using const_pointer = const typename Alloc::value_type*;
    using size_type     = __SP_SIZE_TYPE__;
    using difference_type = ptrdiff_t;

    using propagate_on_container_copy_assignment = spt::false_type;
    using propagate_on_container_move_assignment = spt::false_type;
    using propagate_on_container_swap            = spt::false_type;
    using is_always_equal = spt::bool_constant<spt::is_empty_v<Alloc>>;
    
    template <typename A, typename T, typename... Args>
    using construct_expr = decltype(spt::declval<A>().construct(spt::declval<T*>(), spt::declval<Args>()...));

    template <typename A, typename T>
    using destroy_expr = decltype(spt::declval<A>().destroy(spt::declval<T*>()));

    template <typename T, typename... Args>
    static SP_FORCEINLINE void construct(Alloc& a, T* p, Args&&... args) {
        SP_IF_CONSTEXPR((spt::is_detected_v<construct_expr, Alloc, T, Args...>)) {
            a.construct(p, sp::forward<Args>(args)...);
        }else{
            ::new (static_cast<void*>(p)) T(sp::forward<Args>(args)...);
        }
    }

    template <typename T>
    static SP_FORCEINLINE void destroy(Alloc& a, T* p) {
        SP_IF_CONSTEXPR((spt::is_detected_v<destroy_expr, Alloc, T>)) {
            a.destroy(p);
        }else{
            p->~T();
        }
    }

    SP_NODISCARD SP_FORCEINLINE static pointer allocate(Alloc& a, size_type n){
        return a.allocate(n);
    }

    SP_FORCEINLINE static void deallocate(Alloc& a, pointer p, size_type n) noexcept{
        a.deallocate(p, n);
    }
};


template <typename T>
class allocator : public _spiral_alloc_traits{
public:
    using value_type = T;
    constexpr allocator() noexcept = default;
    template <typename U>
    constexpr allocator(const allocator<U>&) noexcept {}

    constexpr T* allocate(size_type n){
        void* p = std::malloc(n * sizeof(T));
        SP_IF_NOT_EXPECT(!p && n != 0) { std::cout << "ERROR: Could not allocate in sp::allocator\nRequested bytes: " << n << std::endl; throw std::bad_alloc(); }
        return static_cast<T*>(p);
    }

    constexpr void deallocate(T* p, size_type n)noexcept{
        std::free(p);
    }
    static constexpr size_type get_alignment() { return 0; }
    SP_CONSTEXPR20 ~allocator() noexcept = default;
};

template <typename T>
class aligned_allocator : public _spiral_alloc_traits{
public:
    using value_type = T;

    constexpr aligned_allocator() noexcept = default;
    template <typename U>
    constexpr aligned_allocator(const aligned_allocator<U>&) noexcept {}

    constexpr T* allocate(size_type n){
        SP_IF_NOT_EXPECT(n==0) return nullptr;
        ull bytes = (sizeof(T) * n + sp_cache_line_size - 1) & ~(sp_cache_line_size - 1);
        void* p = nullptr;
    #if defined(__APPLE__) || defined(__linux__)
        p = std::aligned_alloc(sp_cache_line_size, bytes);
    #else
        p = _aligned_malloc(bytes, sp_cache_line_size);
    #endif
    SP_IF_NOT_EXPECT(p == nullptr && n != 0) throw std::bad_alloc();
        return static_cast<T*>(p);
    }

    constexpr void deallocate(T* p, size_type n)noexcept{
    #if defined(_MSC_VER) || defined(__MINGW32__)
        _aligned_free(p);
    #else
        std::free(p);
    #endif
    }

    static constexpr size_type get_alignment() { return sp_cache_line_size; }
    SP_CONSTEXPR20 ~aligned_allocator() noexcept = default;
};

template <typename T>
class page_allocator : public _spiral_alloc_traits {
public:
    using value_type = T;
    //using size_type = __SP_SIZE_TYPE__;

    constexpr page_allocator() noexcept = default;
    template <typename U>
    constexpr page_allocator(const page_allocator<U>& other) noexcept {}
    
    static constexpr size_type capacity_for(size_type n) noexcept {
        SP_IF_NOT_EXPECT(n == 0) return 0;
        size_type requested_bytes = n * sizeof(T);
        size_type allocated_bytes = (requested_bytes + 4095) & ~4095; // nearest 4096
        return allocated_bytes / sizeof(T);
    }

    constexpr T* allocate(size_type n) {
        SP_IF_NOT_EXPECT(n == 0) return nullptr;
        size_type bytes = (n * sizeof(T) + 4095) & ~4095; // avoiding truncation loss
        
    #if defined(_WIN32)
        void* ptr = VirtualAlloc(nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    #else
        void* ptr = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        SP_IF_NOT_EXPECT(ptr == MAP_FAILED) ptr = nullptr;
    #endif
        SP_IF_NOT_EXPECT(!ptr) throw std::bad_alloc();
        return static_cast<T*>(ptr);
    }

    constexpr void deallocate(T* p, size_type n) noexcept {
        SP_IF_NOT_EXPECT(!p) return;
        size_type bytes = (n * sizeof(T) + 4095) & ~4095;
    #if defined(_WIN32)
        VirtualFree(p, 0, MEM_RELEASE);
    #else
        munmap(p, bytes);
    #endif
    }

    static constexpr size_type get_alignment() { return 4096; }
    SP_CONSTEXPR20 ~page_allocator() noexcept = default;
};
template <typename T, typename U>
bool operator==(const page_allocator<T>&, const page_allocator<U>&) noexcept { return true; }
template <typename T, typename U>
bool operator!=(const page_allocator<T>&, const page_allocator<U>&) noexcept { return false; }


template <typename T, size_type N>
class unbinded_stack_allocator : public _spiral_alloc_traits{
private:
    alignas(T) char buffer[N * sizeof(T)];
    size_type offset = 0;
public:
    using value_type = T;
    //using size_type = __SP_SIZE_TYPE__;
    
    constexpr unbinded_stack_allocator() = default;
    
    constexpr unbinded_stack_allocator(const unbinded_stack_allocator&) noexcept : offset(0) {}
    constexpr unbinded_stack_allocator(unbinded_stack_allocator&&) noexcept : offset(0) {}
    constexpr unbinded_stack_allocator& operator=(const unbinded_stack_allocator&) noexcept { offset = 0; return *this; }
    constexpr unbinded_stack_allocator& operator=(unbinded_stack_allocator&&) noexcept { offset = 0; return *this; }

    static constexpr size_type capacity_for(size_type n) noexcept { return N; }
    
    SP_FORCEINLINE constexpr T* allocate(size_type n) {
        size_type bytes_requested = n * sizeof(T);
        SP_IF_NOT_EXPECT(offset + bytes_requested > sizeof(buffer)) { throw std::bad_alloc(); }
        void* raw_ptr = (void*)(&buffer[offset]);
        offset += bytes_requested;
        return static_cast<T*>(raw_ptr);
    }
    
    constexpr void deallocate(T* p, size_type n) noexcept{}
    SP_CONSTEXPR20 ~unbinded_stack_allocator() noexcept = default;
};
template <ull N = 1024>
struct binded_stack_allocator {
    template <typename T>
    using r = typename sp::bind_alloc<sp::unbinded_stack_allocator, N>::template r<T>; // r for rebind
};

template <ull N = 1024>
using stack_alloc = binded_stack_allocator<N>;
}; // namespace sp
#endif // ____SP_ALLOCATORS____