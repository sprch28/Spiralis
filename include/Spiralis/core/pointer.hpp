#ifndef ____SP_POINTER____
#define ____SP_POINTER____
#pragma once

#include "../setup/init.hpp"
#include "../core/type_traits.hpp"
#include "../core/allocators.hpp"

namespace sp {

template <typename T, template <typename> typename Alloc = sp::allocator>
class ptr{
private:

    T* ptr_;
    SP_NO_UNIQUE_ADDRESS Alloc<T> alloc_;
    constexpr ptr(T* data, Alloc<T>&& alloc) noexcept : ptr_(data), alloc_(sp::move(alloc)){}
    constexpr ptr(T* data) noexcept : ptr_(data), alloc_(){}

    template <typename U, template <typename> typename A, typename... Args>
    friend constexpr ptr<U, A> make_ptr(A<U>, Args&&...);

    template<typename U, template<typename> typename A, typename... Args>
    friend constexpr ptr<U, A> make_ptr(Args&&... args);

public:

    constexpr ptr(ptr&& other) noexcept : ptr_(other.ptr_), alloc_(sp::move(other.alloc_)) { other.ptr_ = nullptr; }
    SP_CONSTEXPR20 ~ptr() noexcept{
        if(ptr_){
            allocator_traits<Alloc<T>>::destroy(alloc_, ptr_);
            allocator_traits<Alloc<T>>::deallocate(alloc_, ptr_, 1);
        }
    }
    ptr(const ptr&) = delete;
    ptr& operator=(const ptr&) = delete;

    constexpr T* get() noexcept { return ptr_; }
    constexpr const T* get() const noexcept { return ptr_; }

    constexpr Alloc<T>& get_allocator() noexcept { return alloc_; }
    constexpr const Alloc<T>& get_allocator() const noexcept { return alloc_; }

    constexpr T* operator->() noexcept { return ptr_; }
    constexpr const T* operator->() const noexcept { return ptr_; }

    constexpr T& operator*() noexcept { return *ptr_; }
    constexpr const T& operator*() const noexcept { return *ptr_; }

    constexpr explicit operator bool() const noexcept { return ptr_ != nullptr; }

    constexpr T* release() noexcept { T* temp = ptr_; ptr_ = nullptr; return temp; }

    constexpr void reset() noexcept{
        if(ptr_){
            sp::allocator_traits<Alloc<T>>::destroy(alloc_, ptr_);
            sp::allocator_traits<Alloc<T>>::deallocate(alloc_, ptr_, 1);
            ptr_ = nullptr;
        }
    }

    constexpr void swap(ptr& other) noexcept{
        sp::swap(ptr_, other.ptr_);
        sp::swap(alloc_, other.alloc_);
    }

    constexpr bool operator==(decltype(nullptr)) const noexcept { return ptr_ == nullptr; }
    constexpr bool operator!=(decltype(nullptr)) const noexcept { return ptr_ != nullptr; }

    constexpr ptr& operator=(ptr&& other) noexcept{
        if(this != &other){
            reset();
            ptr_ = sp::move(other.ptr_);
            alloc_ = sp::move(other.alloc_);
            other.ptr_ = nullptr;
        }
        return *this;
    }

};

template <typename T, template<typename> typename Alloc = sp::allocator, typename... Args>
constexpr ptr<T, Alloc> make_ptr(Args&&... args){
    Alloc<T> alloc;
    T* data = sp::allocator_traits<Alloc<T>>::allocate(alloc, 1);
    sp::allocator_traits<Alloc<T>>::construct(alloc, data, sp::forward<Args>(args)...);
    return ptr<T, Alloc>(data, alloc);
}

template <typename T, template<typename> typename Alloc = sp::allocator, typename... Args>
constexpr ptr<T, Alloc> make_ptr(Alloc<T> alloc, Args&&... args){
    T* data = sp::allocator_traits<Alloc<T>>::allocate(alloc, 1);
    sp::allocator_traits<Alloc<T>>::construct(alloc, data, sp::forward<Args>(args)...);
    return ptr<T, Alloc>(data, sp::move(alloc));
}


// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-



} // namespace sp

#endif // ____SP_POINTER____