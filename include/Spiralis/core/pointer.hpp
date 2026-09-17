#ifndef ____SP_POINTER____
#define ____SP_POINTER____
#pragma once

#include "../setup/init.hpp"
#include "../core/type_traits.hpp"
#include "../core/allocators.hpp"

namespace sp{
template <typename T, template <typename> typename Alloc>
class shared_ptr;

template <typename T, template <typename> typename Alloc = sp::allocator>
class ptr{
private:

    T* ptr_;
    SP_NO_UNIQUE_ADDRESS Alloc<T> alloc_;
    constexpr ptr(T* data, Alloc<T> alloc) noexcept : ptr_(data), alloc_(sp::move(alloc)){}
    constexpr ptr(T* data) noexcept : ptr_(data), alloc_(){}

    template <typename U, template<typename> typename Allocator, typename... Args>
    friend constexpr ptr<U, Allocator> make_ptr(Alloc<U>, Args&&...);

    template<typename U, template<typename> typename Allocator, typename... Args>
    friend constexpr ptr<U, Allocator> make_ptr(Args&&... args);

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
        SP_IF_EXPECT(this != &other){
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


// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


template<typename T, template <typename> typename Alloc = sp::allocator>
class shared_ptr{
public:
    struct control_block {
        private:

        template <typename U, typename... Args>
        SP_FORCEINLINE static constexpr U* construct_at(U* location, Args&&... args) {
            return ::new(sp::placement_tag{}, static_cast<void*>(location)) U(sp::forward<Args>(args)...);
        }

        alignas(T) unsigned char storage[sizeof(T)];

        public:

        size_type strong_count = 1;
        size_type weak_count = 0;
        SP_NO_UNIQUE_ADDRESS Alloc<control_block> alloc;

        SP_FORCEINLINE T* object() noexcept{
            return reinterpret_cast<T*>(storage);
        }

        SP_FORCEINLINE const T* object() const noexcept{
            return reinterpret_cast<const T*>(storage);
        }

        template<typename... Args>
        control_block(Alloc<control_block> alloc, Args&&... args) : alloc(sp::move(alloc)){
            construct_at(
                object(),
                sp::forward<Args>(args)...
            );
        }
    };
private:

    control_block* control_;
    
    constexpr shared_ptr(control_block* block) : control_(block){}

    template <typename U, template <typename> typename Allocator, typename... Args>
    friend constexpr shared_ptr<U, Allocator> make_shared(Allocator<T> alloc, Args&&... args);

    template <typename U, template <typename> typename Allocator, typename... Args>
    friend constexpr shared_ptr<U, Allocator> make_shared(Args&&... args);

public:

    constexpr shared_ptr() noexcept : control_(nullptr) {}
    constexpr shared_ptr(nullptr_t) noexcept : control_(nullptr) {}

    constexpr shared_ptr(const shared_ptr& other) : control_(other.control_){
        if(control_) ++control_->strong_count;
    }
    constexpr shared_ptr(shared_ptr&& other) noexcept : control_(other.control_){
        other.control_ = nullptr;
    }

    constexpr shared_ptr& operator=(const shared_ptr& other){
        SP_IF_NOT_EXPECT(this == &other) return *this;
        reset();
        control_ = other.control_;
        if(control_) ++control_->strong_count;
        return *this;
    }
    constexpr shared_ptr& operator=(shared_ptr&& other) noexcept{
        SP_IF_NOT_EXPECT(this == &other) return *this;
        reset();
        control_ = other.control_;
        other.control_ = nullptr;
        return *this;
    }

    SP_CONSTEXPR20 SP_FLATTEN ~shared_ptr() noexcept { reset(); }

    constexpr T* get() noexcept { return control_ ? control_->object() : nullptr; }
    constexpr const T* get() const noexcept { return control_ ? control_->object() : nullptr; }

    constexpr T& operator*() noexcept { return *control_->object(); }
    constexpr const T& operator*() const noexcept { return *control_->object(); }

    constexpr T* operator->() noexcept{
        return control_->object();
    }
    constexpr const T* operator->() const noexcept{
        return control_->object();
    }

    constexpr explicit operator bool() const noexcept { return control_ != nullptr; }

    constexpr bool operator==(decltype(nullptr)) const noexcept { return control_ == nullptr; }
    constexpr bool operator!=(decltype(nullptr)) const noexcept { return control_ != nullptr; }

    constexpr size_type use_count() const noexcept { return control_ ? control_->strong_count : 0; }
    constexpr bool unique() const noexcept { return control_ && control_->strong_count == 1; }

    constexpr void reset() noexcept{
        if(!control_) return;
        if(--control_->strong_count == 0){
            control_->object()->~T();
            if(control_->weak_count == 0){
                auto* block = control_;
                Alloc<control_block> alloc = sp::move(block->alloc);
                sp::allocator_traits<Alloc<control_block>>::destroy(
                    alloc,
                    block
                );
                sp::allocator_traits<Alloc<control_block>>::deallocate(
                    alloc,
                    block,
                    1
                );
            }
        }
        control_ = nullptr;
    }
    constexpr void swap(shared_ptr& other) noexcept { sp::swap(control_, other.control_); }

};

template<typename T, template<typename> typename Alloc = sp::allocator, typename... Args>
constexpr shared_ptr<T, Alloc> make_shared(Args&&... args){
    using block = typename shared_ptr<T, Alloc>::control_block;
    Alloc<block> alloc;
    block* memory = sp::allocator_traits<Alloc<block>>::allocate(alloc, 1);
    //try {
        sp::allocator_traits<Alloc<block>>::construct(
            alloc,
            memory,
            alloc,
            sp::forward<Args>(args)...
        );
    /*} catch(...) {
        sp::allocator_traits<Alloc<block>>::deallocate(
            alloc,
            memory,
            1
        );
        throw;
    }*/

    return shared_ptr<T, Alloc>(memory);
}

template<typename T, template<typename> typename Alloc = sp::allocator, typename... Args>
constexpr shared_ptr<T, Alloc> make_shared(Alloc<typename shared_ptr<T, Alloc>::control_block> alloc, Args&&... args){
    using block = typename shared_ptr<T, Alloc>::control_block;
    block* memory = sp::allocator_traits<Alloc<block>>::allocate(alloc, 1);
    //try{
        sp::allocator_traits<Alloc<block>>::construct(
            alloc,
            memory,
            alloc,
            sp::forward<Args>(args)...
        );

    /*}catch(...){
        sp::allocator_traits<Alloc<block>>::deallocate(
            alloc,
            memory,
            1
        );
        throw;
    }*/
    return shared_ptr<T, Alloc>(memory);
}


} // namespace sp

#endif // ____SP_POINTER____