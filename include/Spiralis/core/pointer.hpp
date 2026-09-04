#ifndef ____SP_POINTER____
#define ____SP_POINTER____
#pragma once
#include "../setup/init.hpp"
#include "../core/type_traits.hpp"
#include "../containers/pair.hpp"
namespace sp{

template <typename T, template <typename> typename Allocator = sp::allocator>
class ptr {
private:
    T* data;
    SP_NO_UNIQUE_ADDRESS Allocator<T> alloc;
    constexpr void cleanup(){
        if(data){
            SP_IF_CONSTEXPR(!spt::is_trivially_copyable_v<T>) sp::allocator_traits<Allocator<T>>::destroy(alloc, data);
            sp::allocator_traits<Allocator<T>>::deallocate(alloc, data, 1);
            data = nullptr;
        }
    }

public:
    constexpr ptr() noexcept : data(nullptr), alloc() {}
    constexpr ptr(sp::nullptr_t) noexcept : data(nullptr), alloc() {}

    constexpr explicit ptr(T* val) noexcept : data(val), alloc() {}

    template <typename... Args>
    constexpr ptr(Args&&... args) : alloc() {
        data = sp::allocator_traits<Allocator<T>>::allocate(alloc, 1);
        sp::allocator_traits<Allocator<T>>::construct(alloc, data, sp::forward<Args>(args)...);
    }

    SP_CONSTEXPR20 ~ptr() { cleanup(); }

    ptr(const ptr&) = delete;
    ptr& operator=(const ptr&) = delete;

    constexpr ptr(ptr&& other) noexcept : data(other.data), alloc(sp::move(other.alloc)) { other.data = nullptr; }

    constexpr ptr& operator=(ptr&& other) noexcept {
        if(this != &other){
            cleanup();
            data = other.data;
            alloc = sp::move(other.alloc);
            other.data = nullptr;
        }
        return *this;
    }

    constexpr ptr& operator=(sp::nullptr_t) noexcept{
        reset();
        return *this;
    }

    constexpr T* get() const noexcept { return data; }
    constexpr T& operator*() const noexcept { return *data; }
    constexpr T* operator->() const noexcept { return data; }
    constexpr explicit operator bool() const noexcept { return data != nullptr; }

    constexpr T* release() noexcept{
        T* temp = data;
        data = nullptr;
        return temp;
    }

    constexpr void reset(T* p = nullptr) noexcept{
        if(data != p){
            cleanup();
            data = p;
        }
    }

    constexpr void swap(ptr& other) noexcept{
        using sp::swap;
        swap(data, other.data);
        swap(alloc, other.alloc);
    }
};

template <typename T, template <typename> typename Allocator>
constexpr void swap(ptr<T, Allocator>& lhs, ptr<T, Allocator>& rhs) noexcept {
    lhs.swap(rhs);
}

template <typename T, template <typename> typename Allocator>
constexpr bool operator==(const ptr<T, Allocator>& p, sp::nullptr_t) noexcept {
    return p.get() == nullptr;
}

template <typename T, template <typename> typename Allocator>
constexpr bool operator==(sp::nullptr_t, const ptr<T, Allocator>& p) noexcept {
    return p.get() == nullptr;
}

template <typename T, template <typename> typename Allocator>
constexpr bool operator!=(const ptr<T, Allocator>& p, sp::nullptr_t) noexcept {
    return p.get() != nullptr;
}

template <typename T, template <typename> typename Allocator>
constexpr bool operator!=(sp::nullptr_t, const ptr<T, Allocator>& p) noexcept {
    return p.get() != nullptr;
}

template <typename T, template <typename> typename Allocator = sp::allocator, typename... Args>
constexpr ptr<T, Allocator> make_ptr(Args&&... args) {
    return ptr<T, Allocator>(sp::forward<Args>(args)...);
}

};
#endif