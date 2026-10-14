#ifndef ____SP_POINTER____
#define ____SP_POINTER____
#pragma once

#include "../setup/init.hpp"
#include "../core/type_traits.hpp"
#include "../core/allocators.hpp"

namespace sp{
template <typename T, template <typename> typename Alloc>
class shared_ptr;

template <typename T, template <typename> typename Alloc>
class weak_ptr;

/// Unique owning pointer — sole owner of a heap-allocated object.
/// Non-copyable; ownership transfers via move only.
/// @tparam T     Pointed-to type.
/// @tparam Alloc Allocator template used to allocate and construct @c T.
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

    /// Move constructor. Leaves @p other null.
    constexpr ptr(ptr&& other) noexcept : ptr_(other.ptr_), alloc_(sp::move(other.alloc_)) { other.ptr_ = nullptr; }
    /// Destroys the managed object and deallocates its memory.
    SP_CONSTEXPR20 ~ptr() noexcept{
        if(ptr_){
            allocator_traits<Alloc<T>>::destroy(alloc_, ptr_);
            allocator_traits<Alloc<T>>::deallocate(alloc_, ptr_, 1);
        }
    }
    ptr(const ptr&) = delete;
    ptr& operator=(const ptr&) = delete;

    /// @return Raw pointer to the managed object, or @c nullptr.
    constexpr T* get() noexcept { return ptr_; }
    /// @return Raw pointer to the managed object, or @c nullptr (const).
    constexpr const T* get() const noexcept { return ptr_; }

    /// @return Reference to the allocator.
    constexpr Alloc<T>& get_allocator() noexcept { return alloc_; }
    /// @return Const reference to the allocator.
    constexpr const Alloc<T>& get_allocator() const noexcept { return alloc_; }

    /// Member access on the managed object.
    constexpr T* operator->() noexcept { return ptr_; }
    /// Member access on the managed object (const).
    constexpr const T* operator->() const noexcept { return ptr_; }

    /// Dereferences the managed object.
    constexpr T& operator*() noexcept { return *ptr_; }
    /// Dereferences the managed object (const).
    constexpr const T& operator*() const noexcept { return *ptr_; }

    /// @return @c true if the pointer is non-null.
    constexpr explicit operator bool() const noexcept { return ptr_ != nullptr; }

    /// Releases ownership without destroying the object.
    /// @return The raw pointer; caller is responsible for cleanup.
    constexpr T* release() noexcept { T* temp = ptr_; ptr_ = nullptr; return temp; }

    /// Destroys the managed object and sets the pointer to null.
    constexpr void reset() noexcept{
        if(ptr_){
            sp::allocator_traits<Alloc<T>>::destroy(alloc_, ptr_);
            sp::allocator_traits<Alloc<T>>::deallocate(alloc_, ptr_, 1);
            ptr_ = nullptr;
        }
    }

    /// Swaps ownership with @p other.
    constexpr void swap(ptr& other) noexcept{
        sp::swap(ptr_, other.ptr_);
        sp::swap(alloc_, other.alloc_);
    }

    constexpr bool operator==(decltype(nullptr)) const noexcept { return ptr_ == nullptr; }
    constexpr bool operator!=(decltype(nullptr)) const noexcept { return ptr_ != nullptr; }

    /// Move assignment. Destroys the currently held object, then takes ownership from @p other.
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

/// Allocates and constructs a @c T using a default-constructed @c Alloc.
/// @tparam T     Type to construct.
/// @tparam Alloc Allocator template.
/// @tparam Args  Constructor argument types.
/// @param args   Arguments forwarded to @c T's constructor.
/// @return A @c ptr<T, Alloc> owning the newly constructed object.
template <typename T, template<typename> typename Alloc = sp::allocator, typename... Args>
constexpr ptr<T, Alloc> make_ptr(Args&&... args){
    Alloc<T> alloc;
    T* data = sp::allocator_traits<Alloc<T>>::allocate(alloc, 1);
    sp::allocator_traits<Alloc<T>>::construct(alloc, data, sp::forward<Args>(args)...);
    return ptr<T, Alloc>(data, alloc);
}

/// Allocates and constructs a @c T using the provided @p alloc instance.
/// @param alloc  Allocator instance to use.
/// @param args   Arguments forwarded to @c T's constructor.
/// @return A @c ptr<T, Alloc> owning the newly constructed object.
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


/// Reference-counted shared ownership pointer.
/// The managed object and its control block are allocated together in a single
/// allocation (make_shared pattern), keeping them on the same cache line.
/// The control block stores both a strong and a weak reference count;
/// the object is destroyed when the strong count hits zero, and the block
/// itself is freed when the weak count also hits zero.
/// @tparam T     Pointed-to type.
/// @tparam Alloc Allocator template used for the control block allocation.
template<typename T, template <typename> typename Alloc = sp::allocator>
class shared_ptr{
public:
    /// Internal reference-counting block. Stores the object inline to avoid
    /// a second allocation. Holds strong/weak counts and the allocator instance.
    struct control_block {
        private:

        template <typename U, typename... Args>
        SP_FORCEINLINE static constexpr U* construct_at(U* location, Args&&... args) {
            return ::new(sp::placement_tag{}, static_cast<void*>(location)) U(sp::forward<Args>(args)...);
        }

        alignas(T) unsigned char storage[sizeof(T)];

        public:

        size_type strong_count = 1; ///< Number of owning @c shared_ptr instances.
        size_type weak_count = 0;    ///< Number of non-owning @c weak_ptr instances.
        SP_NO_UNIQUE_ADDRESS Alloc<control_block> alloc; ///< Allocator used to free this block.

        /// @return Pointer to the @c T object stored inside this block.
        SP_FORCEINLINE T* object() noexcept{
            return reinterpret_cast<T*>(storage);
        }
        /// @return Const pointer to the @c T object stored inside this block.
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

    friend class weak_ptr<T, Alloc>;

public:

    /// Constructs a null @c shared_ptr.
    constexpr shared_ptr() noexcept : control_(nullptr) {}
    /// Constructs a null @c shared_ptr from @c nullptr.
    constexpr shared_ptr(nullptr_t) noexcept : control_(nullptr) {}

    /// Copy constructor. Increments the strong reference count.
    constexpr shared_ptr(const shared_ptr& other) : control_(other.control_){
        if(control_) ++control_->strong_count;
    }
    /// Move constructor. Leaves @p other null.
    constexpr shared_ptr(shared_ptr&& other) noexcept : control_(other.control_){
        other.control_ = nullptr;
    }

    /// Copy assignment. Releases current ownership, then shares @p other's object.
    constexpr shared_ptr& operator=(const shared_ptr& other){
        SP_IF_NOT_EXPECT(this == &other) return *this;
        reset();
        control_ = other.control_;
        if(control_) ++control_->strong_count;
        return *this;
    }
    /// Move assignment. Releases current ownership, then takes @p other's object.
    constexpr shared_ptr& operator=(shared_ptr&& other) noexcept{
        SP_IF_NOT_EXPECT(this == &other) return *this;
        reset();
        control_ = other.control_;
        other.control_ = nullptr;
        return *this;
    }

    /// Destructor. Calls @c reset().
    SP_CONSTEXPR20 SP_FLATTEN ~shared_ptr() noexcept { reset(); }

    /// @return Raw pointer to the managed object, or @c nullptr.
    constexpr T* get() noexcept { return control_ ? control_->object() : nullptr; }
    /// @return Raw pointer to the managed object, or @c nullptr (const).
    constexpr const T* get() const noexcept { return control_ ? control_->object() : nullptr; }

    /// Dereferences the managed object.
    constexpr T& operator*() noexcept { return *control_->object(); }
    /// Dereferences the managed object (const).
    constexpr const T& operator*() const noexcept { return *control_->object(); }

    /// Member access on the managed object.
    constexpr T* operator->() noexcept{
        return control_->object();
    }
    /// Member access on the managed object (const).
    constexpr const T* operator->() const noexcept{
        return control_->object();
    }

    /// @return @c true if the pointer is non-null.
    constexpr explicit operator bool() const noexcept { return control_ != nullptr; }

    constexpr bool operator==(decltype(nullptr)) const noexcept { return control_ == nullptr; }
    constexpr bool operator!=(decltype(nullptr)) const noexcept { return control_ != nullptr; }

    /// @return Number of @c shared_ptr instances sharing this object, or 0 if null.
    constexpr size_type use_count() const noexcept { return control_ ? control_->strong_count : 0; }
    /// @return @c true if this is the sole owner of the managed object.
    constexpr bool unique() const noexcept { return control_ && control_->strong_count == 1; }

    /// Releases ownership. Destroys the object when the strong count hits zero;
    /// frees the control block when the weak count also hits zero.
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
    /// Swaps the managed object with @p other in O(1).
    constexpr void swap(shared_ptr& other) noexcept { sp::swap(control_, other.control_); }

};

/// Allocates a @c T and its control block in a single allocation.
/// @tparam T     Type to construct.
/// @tparam Alloc Allocator template.
/// @tparam Args  Constructor argument types.
/// @param args   Arguments forwarded to @c T's constructor.
/// @return A @c shared_ptr<T, Alloc> with strong count 1.
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

/// Allocates a @c T and its control block using the provided @p alloc instance.
/// @param alloc  Allocator instance to use for the control block.
/// @param args   Arguments forwarded to @c T's constructor.
/// @return A @c shared_ptr<T, Alloc> with strong count 1.
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

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

/// Non-owning observer of a @c shared_ptr-managed object.
/// Does not affect the strong reference count; the object may be destroyed
/// while weak_ptrs still exist.  Call @c lock() to obtain a temporary
/// @c shared_ptr — it returns @c nullptr if the object has already been freed.
/// @tparam T     Pointed-to type.
/// @tparam Alloc Allocator template; must match the @c shared_ptr being observed.
template<typename T, template <typename> typename Alloc = sp::allocator>
class weak_ptr {
    using block_t = typename shared_ptr<T, Alloc>::control_block;

    block_t* control_ = nullptr;

public:
    /// Constructs a null weak_ptr.
    constexpr weak_ptr() noexcept = default;

    /// Constructs from a @c shared_ptr, incrementing the weak reference count.
    constexpr weak_ptr(const shared_ptr<T, Alloc>& sp) noexcept : control_(sp.control_) {
        if(control_) ++control_->weak_count;
    }

    /// Copy constructor.
    constexpr weak_ptr(const weak_ptr& other) noexcept : control_(other.control_) {
        if(control_) ++control_->weak_count;
    }

    /// Move constructor.  Leaves @p other null.
    constexpr weak_ptr(weak_ptr&& other) noexcept : control_(other.control_) {
        other.control_ = nullptr;
    }

    /// Copy assignment.
    constexpr weak_ptr& operator=(const weak_ptr& other) noexcept {
        SP_IF_NOT_EXPECT(this == &other) return *this;
        reset();
        control_ = other.control_;
        if(control_) ++control_->weak_count;
        return *this;
    }

    /// Move assignment.
    constexpr weak_ptr& operator=(weak_ptr&& other) noexcept {
        SP_IF_NOT_EXPECT(this == &other) return *this;
        reset();
        control_ = other.control_;
        other.control_ = nullptr;
        return *this;
    }

    /// Assign from a @c shared_ptr.
    constexpr weak_ptr& operator=(const shared_ptr<T, Alloc>& sp) noexcept {
        reset();
        control_ = sp.control_;
        if(control_) ++control_->weak_count;
        return *this;
    }

    SP_CONSTEXPR20 ~weak_ptr() noexcept { reset(); }

    /// Decrements the weak count and frees the control block if both counts hit zero.
    constexpr void reset() noexcept {
        if(!control_) return;
        if(--control_->weak_count == 0 && control_->strong_count == 0) {
            // Object is already destroyed (strong_count hit 0 first);
            // we are the last weak observer — free the block.
            auto* block = control_;
            Alloc<block_t> alloc = sp::move(block->alloc);
            sp::allocator_traits<Alloc<block_t>>::destroy(alloc, block);
            sp::allocator_traits<Alloc<block_t>>::deallocate(alloc, block, 1);
        }
        control_ = nullptr;
    }

    /// @return @c true if the observed object has already been destroyed.
    constexpr bool expired() const noexcept {
        return !control_ || control_->strong_count == 0;
    }

    /// @return Current strong reference count, or 0 if expired.
    constexpr size_type use_count() const noexcept {
        return control_ ? control_->strong_count : 0;
    }

    /// Attempts to obtain a @c shared_ptr to the managed object.
    /// @return A valid @c shared_ptr if the object is still alive, otherwise a null @c shared_ptr.
    constexpr shared_ptr<T, Alloc> lock() const noexcept {
        if(expired()) return shared_ptr<T, Alloc>(nullptr);
        ++control_->strong_count;
        return shared_ptr<T, Alloc>(control_);
    }

    /// Swaps the observed object with @p other in O(1).
    constexpr void swap(weak_ptr& other) noexcept { sp::swap(control_, other.control_); }

    constexpr bool operator==(decltype(nullptr)) const noexcept { return expired(); }
    constexpr bool operator!=(decltype(nullptr)) const noexcept { return !expired(); }
    constexpr explicit operator bool() const noexcept { return !expired(); }
};

/// Convenience factory: creates a @c weak_ptr from an existing @c shared_ptr.
/// @return A @c weak_ptr observing the same object as @p sp.
template<typename T, template<typename> typename Alloc = sp::allocator>
constexpr weak_ptr<T, Alloc> make_weak(const shared_ptr<T, Alloc>& sp) noexcept {
    return weak_ptr<T, Alloc>(sp);
}

} // namespace sp

#endif // ____SP_POINTER____