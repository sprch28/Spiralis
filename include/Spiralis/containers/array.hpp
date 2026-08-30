#ifndef ____SP_ARRAY_HPP____
#define ____SP_ARRAY_HPP____
#pragma once
#include "../setup/init.hpp"
#include "../core/exceptions.hpp"
#include "../core/type_traits.hpp"
#include "../core/allocators.hpp"
#include "../containers/pair.hpp"
#include "../math/bit_manip.hpp"
#include "../math/math.hpp"
#include "../core/iterator.hpp"

#include <iostream>
#include <new>
#include <initializer_list>
#include <cstring>
#include <algorithm>
#include <memory>
#include <limits>

#if defined(__APPLE__)
    #include <mach/vm_statistics.h>
    #include <mach/mach_init.h>
#elif defined(__linux__)
    #include <sys/mman.h>
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
    //#include <intrin.h> 
    //#include <immintrin.h>
    #include <malloc.h>
#endif


// SAFETY LEVELS:

// 0: No safety checks, requires valid params and sufficient capacity when altering size
// 1: Bounds checking on functions, reallocation checks

// Temporary Macros

#ifndef _SP_CHECK_SAFETY_
    #define _SP_CHECK_SAFETY_(level) SP_IF_CONSTEXPR(safety>=level)
#endif

#ifndef _SP_CHECK_SAFETY_LEVEL_
    #define _SP_CHECK_SAFETY_LEVEL_(level) SP_IF_CONSTEXPR(_safety_level>=level)
#endif

#ifndef _SP_SAFETY_TEMPLATE_
    #define _SP_SAFETY_TEMPLATE_ template<short safety = _safety_level>
#endif

namespace sp{
template <typename T, short _safety_level = __SP_DEFAULT_SAFETY_LEVEL__, template <typename> typename Allocator = sp::allocator>
class alignas(spt::get_allocator_alignment<Allocator<T>>()
? sp_cache_line_size 
: sp::max(alignof(Allocator<T>),sp::max(alignof(T*),alignof(ull)))) array{
private:
    _SP_GRANT_IO_ACCESS_

    using is_always_equal = spt::true_type;
    using type_param = spt::conditional_t<spt::is_trivially_copyable_v<T> || sizeof(T) <= 8, T, const T&>;

    static constexpr bool _trivially_copyable = spt::is_trivially_copyable_v<T>;
    static constexpr bool _is_aligned = (spt::get_allocator_alignment<Allocator<T>>() ? 1 : 0);
    
    alignas((_is_aligned) ? sp_cache_line_size : sp::max(alignof(Allocator<T>),sp::max(alignof(T*),alignof(ull)))) 
    T* _data;
    ull _size;
    ull _capacity;
    SP_NO_UNIQUE_ADDRESS Allocator<T> _alloc;

    template <typename Alloc, typename = void> struct allocator_ext {
        static constexpr ull true_capacity(ull n) noexcept { return n; }
    };
    template <typename Alloc> struct allocator_ext<Alloc, spt::void_t<decltype(Alloc::capacity_for(spt::declval<ull>()))>> {
        static constexpr ull true_capacity(ull n) noexcept { return Alloc::capacity_for(n); }
    };

    SP_FORCEINLINE sp::pair<T*, size_type> __getSpiralBinary() const{
        return {_data, _size*sizeof(T)};
    }
    SP_FORCEINLINE const char* __getSpiralMessage() const;

    SP_FORCEINLINE void destroy_elements(){ 
        SP_IF_CONSTEXPR(!spt::is_trivially_destructible_v<T>) _SP_APPLY_UNROLLED_(_size, sp::allocator_traits<Allocator<T>>::destroy(_alloc, _data+i)); 
    }

    template <bool is_new = false>
    _SP_FUNC_NI_ SP_COLD constexpr ull grow_capacity(ull min) const{
        ull new_cap;
        SP_IF_CONSTEXPR(is_new) new_cap = 8;
        else new_cap = _capacity + (_capacity >> 1);
        SP_IF_NOT_EXPECT(new_cap<min) new_cap = min;
    #if defined(__GNUC__) || defined(__clang__)
        return 1ULL << (64 - __builtin_clzll(new_cap - 1));
    #elif defined(_MSC_VER)
        unsigned long index;
        if(_BitScanReverse64(&index, new_cap - 1)){
            return 1ULL << (index + 1);
        }
        return 8;
    #else
            ull val = new_cap - 1;
            val |= val >> 1; val |= val >> 2; val |= val >> 4;
            val |= val >> 8; val |= val >> 16; val |= val >> 32;
            return val + 1;
    #endif
    }

    SP_NOINLINE SP_COLD constexpr void reallocate(ull new_cap){
        ull true_cap = allocator_ext<Allocator<T>>::true_capacity(new_cap);
        T* new_block = true_cap ? sp::allocator_traits<Allocator<T>>::allocate(_alloc, true_cap) : nullptr;
        if(_size && new_block){
            SP_IF_CONSTEXPR(_trivially_copyable){
                memmove(new_block, _data, _size * sizeof(T));
            }
            else SP_IF_CONSTEXPR(spt::is_nothrow_move_constructible_v<T>){
                std::uninitialized_move(_data, _data + _size, new_block);
            }
            else{
                std::uninitialized_copy(_data, _data + _size, new_block);
            }
        }
        SP_IF_CONSTEXPR(!spt::is_trivially_destructible_v<T>){
            destroy_elements();
        }
        sp::allocator_traits<Allocator<T>>::deallocate(_alloc, _data, _capacity);
        _data = new_block;
        _capacity = true_cap;
    }

    SP_FORCEINLINE constexpr void move_data(size_type from, size_type to){
        SP_IF_CONSTEXPR(_trivially_copyable){
            _data[to] = _data[from];
        }else{
            _data[to] = sp::move(_data[from]);
        }
    }

public:
    using iterator = array_iterator<T>;
    using const_iterator = const array_iterator<T>;
    template<typename>
    struct is_array_specialization : spt::false_type {};

    template<typename U>
    struct is_array_specialization<sp::array<U>> : spt::true_type {};

    template<typename U>
    inline static constexpr bool is_array_specialization_s = is_array_specialization<U>::value;


    /**
     * @brief default constructor
     */
    constexpr array() : _data(nullptr), _size(0), _capacity(0) { }

    /**
     * @brief constructor with size
     * @param size initial size of array
     */
    constexpr array(ull size) : _data(nullptr), _size(0), _capacity(allocator_ext<Allocator<T>>::true_capacity(grow_capacity<true>(size))) {
        _SP_CHECK_SAFETY_LEVEL_(1) SP_IF_NOT_EXPECT(_capacity == 0) return;
        _data = sp::allocator_traits<Allocator<T>>::allocate(_alloc, _capacity); //sp::allocator_traits<Allocator<T>>::allocate(_alloc, _capacity);
        try {
            _SP_APPLY_UNROLLED_(size, sp::allocator_traits<Allocator<T>>::construct(_alloc,_data+i); _size++);
        } catch (...) {
            destroy_elements();
            sp::allocator_traits<Allocator<T>>::deallocate(_alloc,_data,_capacity);//sp::allocator_traits<Allocator<T>>::deallocate(_alloc, _data, _capacity);
            _data = nullptr;
            throw;
        }
    }


    /**
     * @brief constructor with size and default value
     * @param count initial size of array
     * @param value default value to fill array with
     */
    constexpr array(ull count, type_param value)
        : _data(nullptr), _size(0), _capacity(allocator_ext<Allocator<T>>::true_capacity((ull)(count + (count >> 1))))
    {
        _SP_CHECK_SAFETY_LEVEL_(1) SP_IF_NOT_EXPECT(_capacity == 0) return;

        _data = sp::allocator_traits<Allocator<T>>::allocate(_alloc, _capacity);
        try {
            _SP_APPLY_UNROLLED_(count, sp::allocator_traits<Allocator<T>>::construct(_alloc,_data+i,value); _size++);
        } catch (...) {
            destroy_elements();
            sp::allocator_traits<Allocator<T>>::deallocate(_alloc, _data, _capacity);
            _data = nullptr;
            throw;
        }
    }

    /**
     * @brief constructor with raw data
     * @param data pointer to the raw data
     * @param size size of the array
     */
    constexpr array(const T* data, ull size) : _data(nullptr), _size(0), _capacity(allocator_ext<Allocator<T>>::true_capacity(size)){
        _SP_CHECK_SAFETY_LEVEL_(1) SP_IF_NOT_EXPECT(_capacity == 0) return;
        _data = sp::allocator_traits<Allocator<T>>::allocate(_alloc, _capacity);
        SP_IF_CONSTEXPR (spt::is_nothrow_copy_constructible<T>::value) {
            _SP_APPLY_UNROLLED_(size, sp::allocator_traits<Allocator<T>>::construct(_alloc,_data+i,data[i]); _size++);
        } else {
            try {
                _SP_APPLY_UNROLLED_(size, sp::allocator_traits<Allocator<T>>::construct(_alloc,_data+i,data[i]); _size++);
            } catch (...) {
                destroy_elements();
                sp::allocator_traits<Allocator<T>>::deallocate(_alloc, _data, _capacity);
                _data = nullptr;
                throw;
            }
        }
    }

    /**
     * @brief copy constructor
     * @param other array to copy from
     */
    constexpr array(const array& other) : _alloc(other._alloc), _data(nullptr), _size(other._size), _capacity(other._capacity){
        _SP_CHECK_SAFETY_LEVEL_(1) SP_IF_NOT_EXPECT(_capacity == 0) return;
        _data = sp::allocator_traits<Allocator<T>>::allocate(_alloc, _capacity);
        SP_IF_CONSTEXPR (_trivially_copyable) {
            std::memcpy(_data, other._data, _size * sizeof(T));
        } else {
            try {
                _SP_APPLY_UNROLLED_(_size, sp::allocator_traits<Allocator<T>>::construct(_alloc,_data+i,other._data[i]));
            } catch (...) {
                destroy_elements();
                sp::allocator_traits<Allocator<T>>::deallocate(_alloc, _data, _capacity);
                throw;
            }
        }
    }

    /**
     * @brief move constructor
     * @param other array to move from
     */
    constexpr array(array&& other) noexcept
        : _data(other._data),
          _size(other._size),
          _capacity(other._capacity)
    {
        other._data = nullptr;
        other._size = 0;
        other._capacity = 0;
    }

    /**
     * @brief initializer list constructor
     * @param list initializer list to copy from
     */
    constexpr array(std::initializer_list<T> list)
        : _data(nullptr), _size(0), _capacity(allocator_ext<Allocator<T>>::true_capacity(list.size()))
    {
        _SP_CHECK_SAFETY_LEVEL_(1) SP_IF_NOT_EXPECT(_capacity == 0) return;
        _data = sp::allocator_traits<Allocator<T>>::allocate(_alloc, _capacity);
        try {
            ull index = 0;
            for (type_param item : list) {
                sp::allocator_traits<Allocator<T>>::construct(_alloc,_data + (index++),item);
                _size++;
            }
        } catch (...) {
            destroy_elements();
            sp::allocator_traits<Allocator<T>>::deallocate(_alloc, _data, _capacity);
            _data = nullptr;
            throw;
        }
    }

    /**
     * @brief initializer list constructor with extra capacity
     * @param list initializer list to copy from
     * @param extra_capacity additional capacity to allocate
     */
    constexpr array(std::initializer_list<T> list, ull extra_capacity)
        : _data(nullptr), _size(0), _capacity(allocator_ext<Allocator<T>>::true_capacity(list.size()+extra_capacity))
    {
        _SP_CHECK_SAFETY_LEVEL_(1) if (_capacity == 0)_SP_UNLIKELY_ return;
        _data = sp::allocator_traits<Allocator<T>>::allocate(_alloc, _capacity);
        try {
            ull index = 0;
            for (type_param item : list) {
                sp::allocator_traits<Allocator<T>>::construct(_alloc,_data + (index++),item); _size++;
            }
        } catch (...) {
            destroy_elements();
            sp::allocator_traits<Allocator<T>>::deallocate(_alloc, _data, _capacity);
            _data = nullptr;
            throw;
        }
    }

    /** 
     * @brief constructor with iterators
     * @param first iterator to the first element
     * @param last iterator to the last element
     */
    /*template<typename InputIt>
    array(InputIt first, InputIt last) : _data(nullptr), _size(0), _capacity(allocator_ext<Allocator<T>>::true_capacity(std::distance(first, last))){
        _SP_CHECK_SAFETY_LEVEL_(1) SP_IF_NOT_EXPECT(_capacity == 0) return;
        _data = sp::allocator_traits<Allocator<T>>::allocate(_alloc, _capacity);
        try {
            for (InputIt it = first; it != last; ++it) {
                sp::allocator_traits<Allocator<T>>::construct(_alloc,_data+_size,*it); _size++;
            }
        } catch (...) {
            destroy_elements();
            sp::allocator_traits<Allocator<T>>::deallocate(_alloc, _data, _capacity);
            _data = nullptr;
            throw;
        }
    }*/


    /**
     * @brief destructor
     */
    #if ___SP_CPP_VER___ >= 20
    constexpr 
    #endif 
    ~array() noexcept {
        destroy_elements();
        sp::allocator_traits<Allocator<T>>::deallocate(_alloc, _data, _capacity);//sp::allocator_traits<Allocator<T>>::deallocate(_alloc, _data, _capacity);
    }









//==========================================================================================================================================
//==========================================================================================================================================
//====================================================== UTILITIES =========================================================================
//==========================================================================================================================================
//==========================================================================================================================================

    SP_FORCEINLINE constexpr bool is_aligned() const{
        return _is_aligned;
    }

    /**
     * @brief grab aligned data: Skips alignment checks
     * @return align-assumed data
     * @warning can fail silently if data isn't aligned/constructed
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE SP_PURE constexpr T* aligned_data() noexcept {
        SP_IF_CONSTEXPR(_is_aligned) return static_cast<T*>(_SP_ASSUME_ALIGNED_(_data, sp_cache_line_size));
        else return _data;
    }


    /**
     * @brief Reserve exact capacity for the array.
     * @param capacity new capacity
     */
    _SP_SAFETY_TEMPLATE_
    SP_COLD SP_FORCEINLINE constexpr void reserve_exact(ull capacity){
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(capacity < _size) throw exceptions::ArrayException("Cannot reserve less than current size.");
        SP_IF_NOT_EXPECT(capacity != _capacity) reallocate(capacity);
    }

    /**
     * @brief Reserve capacity for the array(rounded to next pow2)
     * @param new_capacity the new capacity to reserve
     */
    _SP_SAFETY_TEMPLATE_
    SP_COLD SP_FORCEINLINE constexpr void reserve(ull new_capacity){
        ull target = grow_capacity(new_capacity);
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(target <= _capacity) return;
        reallocate(target);
    }

    /**
     * @brief Reserve capacity for the array.
     * @param new_capacity the new capacity to add
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE constexpr void reserve_extra(ull new_capacity){
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(new_capacity<1) return;
        reallocate(_capacity+new_capacity);
    }

    /**
     * @brief Reserve capacity for the array.
     * @param new_capacity the new capacity to add
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE constexpr void reserve_extra_rounded(ull new_capacity){
        ull target = grow_capacity(new_capacity+_capacity);
        reallocate(target);
    }

    /**
     * @brief Access element at index.
     * @param index index of the element to access
     * @return reference to the element at the specified index
     */
    _SP_FUNC_NIP_ constexpr T& operator[](ull index) { return _data[index]; }
    /**
     * @brief Access element at index (const version).
     * @param index index of the element to access
     * @return const reference to the element at the specified index
     */
    _SP_FUNC_NIP_ constexpr const T& operator[](ull index) const { return _data[index]; }
private:
    static constexpr SP_FORCEINLINE size_type get_lookahead(){
        return sp_cache_line_size / sizeof(T);//(sizeof(T) + sp_cache_line_size - 1) / sp_cache_line_size;
    }
public:
    _SP_FUNC_NI_ constexpr T& access_and_prefetch(ull index) {
        SP_IF_CONSTEXPR(_is_aligned){
            _SP_PREFETCH_(&_data[index+get_lookahead()], 0, 1);
        }
        return _data[index];
    }

    _SP_FUNC_NI_ constexpr const T& access_and_prefetch(ull index) const {
        SP_IF_CONSTEXPR(_is_aligned){
            _SP_PREFETCH_(&_data[index+get_lookahead()], 0, 1);
        }
        return _data[index];
    }

    /**
     * @brief Access element at index.
     * @param index index of the element to access
     * @return reference to the element at the specified index
     * @throws ArrayException if index is out of bounds & safety level is 1+
     */
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NIP_ constexpr T& at(ull index){
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(index>=_size) throw exceptions::ArrayException("Index out of range.");
        return _data[index];
    }

    /**
     * @brief Access element at index (const version).
     * @param index index of the element to access
     * @return const reference to the element at the specified index
     * @throws ArrayException if index is out of bounds & safety level is 1+
     */
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NIP_ constexpr const T& at(ull index) const{
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(index>=_size) throw exceptions::ArrayException("Index out of range.");
        return _data[index];
    }

    /**
     * @brief Get the current size of the array.
     * @return current size of the array
     */
    _SP_FUNC_NIP_ constexpr ull length() const { return _size; }
    /**
     * @brief Get the current size of the array.
     * @return current size of the array
     */
    _SP_FUNC_NIP_ constexpr ull size() const noexcept { return _size; }
    /**
     * @brief Get the current capacity of the array.
     * @return current capacity of the array
     */
    _SP_FUNC_NIP_ constexpr ull capacity() const noexcept { return _capacity; }
    /**
     * @brief Check if the array is empty.
     * @return true if the array is empty, false otherwise
     */
    _SP_FUNC_NIP_ constexpr bool is_empty() const noexcept { return _size == 0; }

    /**
     * @brief Check if the array is empty (alias for is_empty; STL compatibility)
     * @return true if the array is empty, false otherwise
     */
    _SP_FUNC_NIP_ constexpr bool empty() const noexcept { return _size == 0; }

    /**
     * @brief get the max size of the array
     * @return the max size of an unsigned long long
     */
    _SP_FUNC_NI_ SP_CONST constexpr ull max_size() const noexcept { return npos; }

    /**
     * @brief explicit bool conversion
     */
    explicit constexpr operator bool() const noexcept { return !is_empty(); }

    /**
     * @brief Get a reference to the front element in the array.
     * @return reference to the front element
     */
    _SP_FUNC_NIP_ constexpr T& front() { return _data[0]; }
    /**
     * @brief Get a const reference to the front element in the array.
     * @return const reference to the front element
     */
    _SP_FUNC_NIP_ constexpr const T& front() const { return _data[0]; }
    /**
     * @brief Get a reference to the last element in the array.
     * @return reference to the last element
     */
    _SP_FUNC_NIP_ constexpr T& back() { return _data[_size - 1]; }
    /**
     * @brief Get a const reference to the last element in the array.
     * @return const reference to the last element
     */
    _SP_FUNC_NIP_ constexpr const T& back() const { return _data[_size - 1]; }


    /**
     * @brief Check if the array is equal to another array.
     * @param other array to check against
     * @return true if the arrays are equal, false otherwise
     */
    SP_NODISCARD constexpr bool equals(const array<T, _safety_level>& other) const {
        SP_IF_NOT_EXPECT(_size != other._size) return false;
        SP_IF_CONSTEXPR (_trivially_copyable && sizeof(T) == sizeof(int)){
            return std::memcmp(_data, other._data, _size * sizeof(T)) == 0;
        }
    _SP_APPLY_UNROLLED_(_size, SP_IF_NOT_EXPECT(!(_data[i]==other._data[i])) return false);
        return true;
    }


    SP_FLATTEN constexpr friend bool operator==(const array& a, const array& b) noexcept { return a.equals(b); }
    SP_FLATTEN constexpr friend bool operator!=(const array& a, const array& b) noexcept { return !a.equals(b); }

    /**
     * @brief Get a pointer to the underlying data.
     * @return pointer to the underlying data
     */
    _SP_FUNC_NIFP_ constexpr T* data() noexcept { SP_IF_CONSTEXPR(_is_aligned) return aligned_data(); else return _data; }
    /**
     * @brief Get a const pointer to the underlying data.
     * @return const pointer to the underlying data
     */
    _SP_FUNC_NIP_ constexpr const T* data() const noexcept { return _data; }

    _SP_FUNC_NIFP_ constexpr T* D() noexcept { return data(); }
    _SP_FUNC_NIFP_ constexpr T* D() const noexcept { return data(); }

    /**
     * @brief Get an iterator to the beginning of the array.
     * @return iterator to the beginning
     */
    _SP_FUNC_NI_ constexpr iterator begin() noexcept { return iterator(_data); }
    /**
     * @brief Get an iterator to the end of the array.
     * @return iterator to the end
     */
    _SP_FUNC_NI_ constexpr iterator end() noexcept { return iterator(_data + _size); }

    // in array.hpp
    _SP_FUNC_NI_ constexpr const_iterator begin() const noexcept { return const_iterator(_data); }
    _SP_FUNC_NI_ constexpr const_iterator end() const noexcept { return const_iterator(_data + _size); }

    /**
     * @brief Get a const iterator to the beginning of the array.
     * @return const iterator to the beginning
     */
    _SP_FUNC_NI_ constexpr const_iterator cbegin() const noexcept { return const_iterator(_data); }
    /**
     * @brief Get a const iterator to the end of the array.
     * @return const iterator to the end
     */
    _SP_FUNC_NI_ constexpr const_iterator cend() const noexcept { return const_iterator(_data + _size); }

    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    /**
     * @brief Get a reverse iterator to the beginning of the array.
     * @return reverse iterator to the beginning
     */
    _SP_FUNC_NI_ constexpr reverse_iterator rbegin() noexcept{ return reverse_iterator(end()); }
    /**
     * @brief Get a reverse iterator to the end of the array.
     * @return reverse iterator to the end
     */
    _SP_FUNC_NIF_ constexpr reverse_iterator rend() noexcept{ return reverse_iterator(begin()); }

    /**
     * @brief Get a const reverse iterator to the beginning of the array.
     * @return const reverse iterator to the beginning
     */
    _SP_FUNC_NI_ constexpr const reverse_iterator crbegin() const noexcept{ return const_reverse_iterator(end()); }
    /**
     * @brief Get a const reverse iterator to the end of the array.
     * @return const reverse iterator to the end
     */
    _SP_FUNC_NI_ constexpr const reverse_iterator crend() const noexcept{ return const_reverse_iterator(begin()); }

    /**
     * @brief copy assignment operator
     * @param other array to copy from
     * @return reference to this array
     */
    constexpr array& operator=(const array& other){
        SP_IF_NOT_EXPECT(this == &other) return *this;
        array temp(other);
        sp::swap(_data, temp._data);
        sp::swap(_size, temp._size);
        sp::swap(_capacity, temp._capacity);
        sp::swap(_alloc,temp._alloc);
        return *this;
    }


    /**
     * @brief Move assignment operator.
     * @param other array to move from
     * @return reference to this array
     */
    constexpr array& operator=(array&& other) noexcept {
        SP_IF_EXPECT(this != &other){
            destroy_elements();
            sp::allocator_traits<Allocator<T>>::deallocate(_alloc, _data, _capacity);

            _data = other._data;
            _size = other._size;
            _capacity = other._capacity;

            other._data = nullptr;
            other._size = 0;
            other._capacity = 0;
        }
        return *this;
    }

    /**
     * @brief return the default safety level of the array class
     * @return safety level of the array class
     */
    _SP_FUNC_NI_ SP_CONST static constexpr short safety_level() { return _safety_level; }

    /**
     * @brief Print the contents of the array to the console.
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE constexpr void print() const {
        // 1. Save state
        std::ios_base::fmtflags old_flags = std::cout.flags();
        std::streamsize old_prec = std::cout.precision();

        std::cout << sp::console::FG_MAGENTA << "{" << sp::console::RESET_EFFECTS;

        // 2. Set to Max Precision
        // max_digits10 ensures that if you read this number back in, 
        // you get the exact same bit-pattern.
        std::cout.precision(std::numeric_limits<double>::max_digits10); 

        for (ull i = 0; i < _size; i++) {
            std::cout << _data[i] << sp::console::FG_RED << ((i == _size - 1) ? "" : ",") << sp::console::RESET_EFFECTS;
        }

        std::cout << sp::console::FG_MAGENTA << "}" << sp::console::RESET_EFFECTS << std::endl;

        // 3. Restore state
        std::cout.flags(old_flags);
        std::cout.precision(old_prec);
    }

//==========================================================================================================================================
//==========================================================================================================================================
//====================================================== MODIFICATION ======================================================================
//==========================================================================================================================================
//==========================================================================================================================================
    
public:

    /**
     * @brief Emplace a new element at the specified index.
     * @param index index to emplace at
     * @param args arguments to forward to the element constructor
     */
    template <short safety = _safety_level, typename... Args>
    SP_HOT constexpr void emplace(ull index, Args&&... args) {
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(index > _size) throw exceptions::ArrayException("Index out of range.");
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(_size >= _capacity) reallocate(grow_capacity(_size + 1));
        T* pos = _data + index;
        if(index == _size) sp::allocator_traits<Allocator<T>>::construct(_alloc, pos, sp::forward<Args>(args)...);
        else SP_IF_CONSTEXPR (_trivially_copyable) {
            memmove(pos + 1, pos, (_size - index) * sizeof(T));
            sp::allocator_traits<Allocator<T>>::construct(_alloc, pos, sp::forward<Args>(args)...);
        }else{
            sp::allocator_traits<Allocator<T>>::construct(_alloc, _data + _size, sp::move(_data[_size - 1]));
            for (ull i = _size - 1; i > index; --i) _data[i] = sp::move(_data[i - 1]);
            sp::allocator_traits<Allocator<T>>::destroy(_alloc, pos);
            sp::allocator_traits<Allocator<T>>::construct(_alloc, pos, sp::forward<Args>(args)...);
        }
        ++_size;
    }

    /**
     * @brief Insert a new element at the front of the array.
     * @param args arguments to forward to the element constructor
     */
    template <short safety = _safety_level, typename... Args>
    SP_FORCEINLINE SP_HOT constexpr void emplace_front(Args&&... args) {
        emplace<safety>(0, sp::forward<Args>(args)...);
    }

    /**
     * @brief Insert a new element at the front of the array.
     * @param item element to insert
     */
    _SP_SAFETY_TEMPLATE_
    SP_HOT SP_FLATTEN constexpr void push_front(type_param item) { 
        emplace_front<safety>(sp::move(item)); 
    }

    /**
     * @brief Emplace a new element at the end of the array.
     * @param args arguments to forward to the element constructor
     */
    template <short safety = _safety_level, typename... Args>
    SP_FORCEINLINE SP_HOT constexpr void emplace_back(Args&&... args) {
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(_size >= _capacity) reallocate(grow_capacity(_size + 1));
        SP_IF_CONSTEXPR (sizeof(T) >= 32) { 
            _SP_PREFETCH_(&_data[_size + 1], 1, 3);
        }
        sp::allocator_traits<Allocator<T>>::construct(_alloc, _data + _size, sp::forward<Args>(args)...);
        ++_size;
    }

    /**
     * @brief Push a new element to the back of the array.
     * @param item element to push
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE SP_HOT SP_FLATTEN constexpr void push_back(type_param item) { emplace_back<safety>(item); }

    /**
     * @brief Clear the array and resize to a new size.
     * @param new_size new size of the array
     */
    _SP_SAFETY_TEMPLATE_
    constexpr void clear_and_resize(ull new_size) {
        destroy_elements();
        ull final_size = new_size>0 ? next_pow2(new_size) : new_size;
        if(new_size > _capacity){
            sp::allocator_traits<Allocator<T>>::deallocate(_alloc, _data, _capacity);
            SP_IF_EXPECT(new_size>0) _data = sp::allocator_traits<Allocator<T>>::allocate(_alloc, final_size);
            else _data = nullptr;
            _capacity = new_size;
        }
        ull tsz = _size;
        _size = 0;
        _SP_EXPLICIT_UNROLLED_(i, tsz, final_size, sp::allocator_traits<Allocator<T>>::construct(_alloc, _data+i); _size++);
    }

    /**
     * @brief Remove the element at the specified index.
     * @param index index of the element to remove
     * @return the removed element
     */
    _SP_SAFETY_TEMPLATE_
    constexpr T pop(ull index) {
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(index >= _size) throw exceptions::ArrayException("Index out of range.");
        T removed_element = sp::move(_data[index]);
        if(index < _size - 1) _SP_EXPLICIT_UNROLLED_(i, index, _size - 1, _data[i] = sp::move(_data[i + 1]));
        sp::allocator_traits<Allocator<T>>::destroy(_alloc, _data + (--_size));
        return removed_element;
    }

    /**
     * @brief Resize the array to a new size.
     * @param count new size of the array
     */
    _SP_SAFETY_TEMPLATE_
    constexpr void resize(ull count) {
        if(count > _size){
            if (count > _capacity) reallocate(grow_capacity(count));
            SP_IF_CONSTEXPR (!spt::is_trivially_default_constructible<T>::value) _SP_EXPLICIT_UNROLLED_(i, _size, count, sp::allocator_traits<Allocator<T>>::construct(_alloc,_data+i));
        }
        else if(count < _size) SP_IF_CONSTEXPR (!spt::is_trivially_destructible_v<T>) _SP_EXPLICIT_UNROLLED_(i, count, _size, sp::allocator_traits<Allocator<T>>::destroy(_alloc,_data+i));
        _size = count;
    }

    /**
     * @brief Insert an element into the array at the specified index.
     * @param index index to insert at
     * @param value element to insert
     */
    _SP_SAFETY_TEMPLATE_
    constexpr void insert(ull index, type_param value) { emplace<safety>(index, value); }

    /**
     * @brief Erase the element at the specified index.
     * @param index index of the element to erase
     */
    _SP_SAFETY_TEMPLATE_
    constexpr void erase(ull index){
    _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(index >= _size) return;
        sp::allocator_traits<Allocator<T>>::destroy(_alloc,_data+index);
        SP_IF_CONSTEXPR(_trivially_copyable){
            memmove(_data + index, _data + index + 1, (_size - index - 1) * sizeof(T));
        }else{
            _SP_EXPLICIT_UNROLLED_(i, index, _size - 1, _data[i] = sp::move(_data[i + 1]));
            sp::allocator_traits<Allocator<T>>::destroy(_alloc,_data+_size-1);
        }
       --_size;
    }

    /**
     * @brief Erase the element at the specified index.
     * @param index index of the element to erase
     */
    _SP_SAFETY_TEMPLATE_
    constexpr void erase_swap(ull index){
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(index>=_size) throw exceptions::ArrayException("Index out of range.");
        SP_IF_EXPECT(index!=_size-1) _data[index] = sp::move(_data[_size-1]);
        sp::allocator_traits<Allocator<T>>::destroy(_alloc,_data+_size-1);
        --_size;
    }

    /**
     * @brief Shrink the capacity of the array to fit its size.
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE constexpr void shrink_to_fit(){
        SP_IF_EXPECT(_size < _capacity){
            SP_IF_NOT_EXPECT(_size == 0){
                sp::allocator_traits<Allocator<T>>::deallocate(_alloc, _data, _capacity);
                _data = nullptr;
                _capacity = 0;
            }else reallocate(_size);
        }
    }

    /**
     * @brief Clear the array's size
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE constexpr void clear() noexcept{
        destroy_elements();
        _size = 0;
    }

    /**
     * @brief Remove the front element from the array.
     * @return the popped value
     */
    _SP_SAFETY_TEMPLATE_
    constexpr T pop_front() { return pop(0); }

    /**
     * @brief Remove the back element from the array.
     * @return the popped value
     */
    _SP_SAFETY_TEMPLATE_
    constexpr T pop_back(){ return (SP_EXPECT(_size>0,true)) ? pop(_size-1) : pop(0); }

    /**
     * @brief Check if the array contains the specified element.
     * @param item element to check for
     * @return true if the element is found, false otherwise
     */
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NIP_ constexpr bool contains(type_param item){
        _SP_APPLY_UNROLLED_(_size, SP_IF_EXPECT(_data[i]==item) return true);
        return false;
    }

    /**
     * @brief Check if the array contains all elements from another array.
     * @param other array to check against
     * @return true if all elements are found, false otherwise
     */
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NIP_ constexpr bool contains_all(const array<T, _safety_level>& other){
        _SP_APPLY_UNROLLED_(other._size, SP_IF_NOT_EXPECT(!contains(other[i])) return false);
        return true;
    }

    /**
     * @brief Check if the array contains any element from another array.
     * @param other array to check against
     * @return true if any element is found, false otherwise
     */
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NIP_ constexpr bool contains_any(const array<T, _safety_level>& other){
        _SP_APPLY_UNROLLED_(other._size, SP_IF_NOT_EXPECT(contains(other[i])) return true);
        return false;
    }

    /**
     * @brief Get the index of the specified element.
     * @param item element to find
     * @return index of the element, or -1 if not found
     */
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NIP_ constexpr long long index_of(type_param item) noexcept{
        _SP_APPLY_UNROLLED_(_size, SP_IF_NOT_EXPECT(_data[i]==item) return i);
        return -1;
    }

    /**
     * @brief Swap the elements at the specified indices.
     * @param a index of the mid element
     * @param b index of the second element
     */
    _SP_SAFETY_TEMPLATE_
    constexpr void swap_elements(ull a, ull b){
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(a >= _size || b >= _size) throw exceptions::ArrayException("Index out of range.");
        sp::swap(_data[a], _data[b]);
    }

    /**
     * @brief Reverse the elements in the array.
     * @return array<T, _safety_level>
     */
    _SP_SAFETY_TEMPLATE_
    constexpr array<T, _safety_level> reverse() const {
        array<T, _safety_level> result(_size);
        _SP_APPLY_UNROLLED_(_size, result._data[i] = _data[_size - 1 - i]);
        return result;
    }

    /**
     * @brief Reverse the elements in the array.
     * @return *this: enables chaining
     */
    _SP_SAFETY_TEMPLATE_
    constexpr array<T, _safety_level>& reverse_(){
        ull i = 0, j = _size - 1;
        while (i < j) {
            T temp = _data[i];
            _data[i++] = _data[j];
            _data[j--] = temp;
        }
        return *this;
    }

    /**
     * @brief Reverse the elements in the specified range.
     * @param start start index of the range
     * @param end end index of the range
     * @return new array with reversed elements in the specified range
     */
    _SP_SAFETY_TEMPLATE_
    constexpr array<T, _safety_level> reverse_range(ull start, ull end) const {
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(start>=end||end>_size||_size==0) throw exceptions::ArrayException("Invalid range.");
        array<T, _safety_level> result = *this;
        --end;
        while (start < end){
            T tmp = result._data[start];
            result._data[start++] = result._data[end];
            result._data[end--] = tmp;
        }
        return result;
    }

    /**
     * @brief Reverse the elements in the specified range.
     * @param start start index of the range
     * @param end end index of the range
     * @return this: enables chaining
     * @warning modifies the original array
     */
    _SP_SAFETY_TEMPLATE_
    constexpr array<T, _safety_level>& reverse_range_(ull start, ull end){
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(start>=end||end>_size||_size==0) throw exceptions::ArrayException("Invalid range.");
        --end;
        while(start<end){
            T tmp = _data[start];
            _data[start++] = _data[end];
            _data[end--] = tmp;
        }
        return *this;
    }

    /**
     * @brief Rotate the elements to the left by the specified number of positions.
     * @param num number of positions to rotate
     * @return new array with rotated elements
     */
    _SP_SAFETY_TEMPLATE_
    constexpr array<T, _safety_level> rotate_left(ull num) const {
        array<T, _safety_level> result = *this;
        result.template rotate_left_<safety>(num);
        return result;
    }

    /**
     * @brief Rotate the elements to the left by the specified number of positions (in-place).
     * @param num number of positions to rotate
     * @return reference to this array (enables chaining)
     * @warning modifies the original array
     */
    _SP_SAFETY_TEMPLATE_
    constexpr array<T, _safety_level>& rotate_left_(ull num) {
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(_size <= 1 || num == 0) return *this;
        num %= _size;
        reverse_range_(0, num);
        reverse_range_(num, _size);
        reverse_();
        return *this;
    }

    /**
     * @brief Rotate the elements to the right by the specified number of positions.
     * @param num number of positions to rotate
     * @return new array with rotated elements
     */
    _SP_SAFETY_TEMPLATE_
    constexpr array<T, _safety_level> rotate_right(ull num) const {
        array<T, _safety_level> result = *this;
        result.template rotate_right_<safety>(num);
        return result;
    }

    /**
     * @brief Rotate the elements to the right by the specified number of positions (in-place).
     * @param num number of positions to rotate
     * @return reference to this array (enables chaining)
     * @warning modifies the original array
     */
    _SP_SAFETY_TEMPLATE_
    constexpr array<T, _safety_level>& rotate_right_(ull num) {
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(_size <= 1 || num == 0) return *this;
        num = num % _size;
        return rotate_left_<safety>(_size - num);
    }

    /** 
     * @brief Find the mid occurrence of the specified value.
     * @param value element to find
     * @return index of the element, or _size if not found
     */
    _SP_SAFETY_TEMPLATE_
    SP_NODISCARD SP_PURE SP_FORCEINLINE constexpr ull find(type_param value) const noexcept {
        const ull block_size = 4;
        ull blocks = _size / block_size;
    #if __SP_UNROLL_LOOPS__ == 1
        for (ull i = 0; i + 3 < blocks; i+= 4) {
            const T* p = _data + i * 4;
            SP_IF_NOT_EXPECT (p[0] == value) return i * block_size;
            SP_IF_NOT_EXPECT (p[1] == value) return i * block_size + 1;
            SP_IF_NOT_EXPECT (p[2] == value) return i * block_size + 2;
            SP_IF_NOT_EXPECT (p[3] == value) return i * block_size + 3;
        }
    for (ull i = blocks * block_size; i < _size; i++) SP_IF_NOT_EXPECT (_data[i] == value) return i;
    #else
    for (ull i = 0; i < _size; i++) SP_IF_NOT_EXPECT(_data[i] == value) return i;
    #endif
        return _size;
    }




    /**
     * @brief Find the last occurrence of the specified value.
     * @param value element to find
     * @return index of the element, or _size if not found
     */
    _SP_SAFETY_TEMPLATE_
    SP_NODISCARD SP_PURE SP_FORCEINLINE constexpr ull find_last(type_param value) const{
    #if __SP_UNROLL_LOOPS__ == 1
        ull i = _size-1;
        while(i>=3){
            SP_IF_NOT_EXPECT(_data[i]==value) return i;
            SP_IF_NOT_EXPECT(_data[i-1]==value) return i-1;
            SP_IF_NOT_EXPECT(_data[i-2]==value) return i-2;
            SP_IF_NOT_EXPECT(_data[i-3]==value) return i-3;
            i -= 4;
        }
        i++;
    while(i>0) { SP_IF_NOT_EXPECT(predicate(_data[i-1])) return i-1; i--; }
    #else
    for (ull i = _size; i > 0; i--) SP_IF_NOT_EXPECT(predicate(_data[i - 1])) return i - 1;
    #endif
        return _size;
    }

    /**
     * @brief Find the mid element that matches the predicate.
     * @param predicate function to test each element
     * @return index of the element, or _size if not found
     */
    template <short safety = _safety_level, typename Func>
    SP_NODISCARD SP_PURE SP_FORCEINLINE constexpr ull find_if(Func predicate) const{
        const ull block_size = 4;
        ull blocks = _size / block_size;
    #if __SP_UNROLL_LOOPS__ == 1
        for (ull i = 0; i + 3 < blocks; i+=4) {
            const T* p = _data + i * 4;
            SP_IF_NOT_EXPECT(predicate(p[0])) return i * block_size;
            SP_IF_NOT_EXPECT(predicate(p[1])) return i * block_size + 1;
            SP_IF_NOT_EXPECT(predicate(p[2])) return i * block_size + 2;
            SP_IF_NOT_EXPECT(predicate(p[3])) return i * block_size + 3;
        }
    for (ull i = blocks * block_size; i < _size; i++) SP_IF_NOT_EXPECT(predicate(_data[i])) return i;
    #else
    for (ull i = 0; i < _size; i++) SP_IF_NOT_EXPECT(predicate(_data[i])) return i;
    #endif
        return _size;
    }

    /**
     * @brief Find the last element that matches the predicate.
     * @param predicate function to test each element
     * @return index of the element, or _size if not found
     */
    template <short safety = _safety_level, typename Func>
    SP_NODISCARD SP_PURE SP_FORCEINLINE constexpr ull find_last_if(Func predicate) const{
    #if __SP_UNROLL_LOOPS__ == 1
        ull i = _size-1;
        while(i>=3){
            SP_IF_NOT_EXPECT(predicate(_data[i])) return i;
            SP_IF_NOT_EXPECT(predicate(_data[i - 1])) return i - 1;
            SP_IF_NOT_EXPECT(predicate(_data[i - 2])) return i - 2;
            SP_IF_NOT_EXPECT(predicate(_data[i - 3])) return i - 3;
            i -= 4;
        }
        i++;
    while(i>0) { SP_IF_NOT_EXPECT(predicate(_data[i-1])) return i-1; i--; }
    #else
        for (ull i = _size; i > 0; i--) SP_IF_NOT_EXPECT(predicate(_data[i - 1])) return i - 1;
    #endif
        return _size;
    }

    /**
     * @brief Apply a function to each element in the array.
     * @param func function to apply
     * @warning if lambda takes refrenced T as param, original can be modified
     */
    template <short safety = _safety_level, typename Func>
    SP_FORCEINLINE constexpr void for_each(Func func) { _SP_APPLY_UNROLLED_(_size, func(_data[i])); }

    /**
     * @brief Count the occurrences of the specified value.
     * @param value element to count
     * @return number of occurrences
     */
    _SP_SAFETY_TEMPLATE_
    SP_NODISCARD SP_PURE SP_FORCEINLINE constexpr ull count(type_param value){
        ull counter = 0;
        _SP_APPLY_UNROLLED_(_size, if(_data[i]==value) counter++);
        return counter;
    }

    /**
     * @brief Get the indices of all occurrences of the specified value.
     * @param value element to find
     * @return array of indices where the element is found
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE constexpr array<ull> indices_of(type_param value) const{
        array<ull> result;
        _SP_APPLY_UNROLLED_(_size, if(_data[i]==value) result.push_back(i));
        return result;
    }

    /**
     * @brief Remove the mid occurrence of the specified value.
     * @param value element to remove
     * @return true if the element was found and removed, false otherwise
     * @warning modifies the original array
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE constexpr bool remove_mid(type_param value){
        ull idx = find(value);
        SP_IF_NOT_EXPECT(idx==_size) return false;
        erase(idx);
        return true;
    }

    /**
     * @brief Remove the last occurrence of the specified value.
     * @param value element to remove
     * @return true if the element was found and removed, false otherwise
     * @warning modifies the original array
     */
    SP_FORCEINLINE constexpr bool remove_last(type_param value){
        ull idx = find_last(value);
        SP_IF_NOT_EXPECT(idx==_size) return false;
        erase(idx);
        return true;
    }

    /**
     * @brief Remove elements that match the predicate.
     * @param pred function to test each element
     * @return number of elements removed
     * @warning modifies the original array
     */
    template <short safety = _safety_level, typename Pred>
    SP_FORCEINLINE constexpr ull remove_if(Pred pred){
        ull counter = 0;    
        while(true){
            ull idx = find_if(pred);
            SP_IF_NOT_EXPECT(idx==_size) return counter;
            erase(idx);
            counter++;
        }
    }

    /**
     * @brief Remove all occurrences of the specified value.
     * @param value element to remove
     * @return number of elements removed
     * @warning modifies the original array
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE constexpr ull remove_all(type_param value){
        ull counter = 0;
        while(true){
            ull idx = find(value);
            SP_IF_NOT_EXPECT(idx==_size) return counter;
            erase(idx);
            counter++;
        }
    }

    /**
     * @brief Sort the array using the specified comparator in place.
     * @param comp comparator function
     * @note uses std::sort backend
     * @return *this: enables chaining
     */
    template <typename Comp>
    SP_FORCEINLINE constexpr array<T, _safety_level>& sort_by_(Comp comp) {
        _SP_CHECK_SAFETY_LEVEL_(1) SP_IF_NOT_EXPECT(_size == 0) throw exceptions::ArrayException("Cannot sort an empty array.");
        std::sort(_data, _data + _size, comp);
        return *this;
    }


    /**
     * @brief Sort the array using the specified comparator.
     * @param comp comparator function
     * @note uses std::sort backend
     */
    template <typename Comp>
    SP_FORCEINLINE constexpr array<T, _safety_level> sort_by(Comp comp) {
        _SP_CHECK_SAFETY_LEVEL_(1) SP_IF_NOT_EXPECT(_size == 0)throw exceptions::ArrayException("Cannot sort an empty array.");
        array<T, _safety_level> result(*this);
        std::sort(result._data, result._data + result._size, comp);
        return result;
    }

    /** 
     * @brief Fill the array with the specified value.
     * @param value element to fill the array with
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE constexpr array& fill(type_param value) {
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(_size == 0) throw exceptions::ArrayException("Cannot fill an empty array.");
        SP_IF_CONSTEXPR(_trivially_copyable){
            std::memset(_data, value, _size * sizeof(T));
        }else _SP_APPLY_UNROLLED_(_size, _data[i] = value);
        return *this;
    }

    /**
     * @brief Swap the contents of this array with another array.
     * @param other array to swap with
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE constexpr void swap(array<T, _safety_level>& other) noexcept{
        sp::swap(_data, other._data);
        sp::swap(_size, other._size);
        sp::swap(_capacity, other._capacity);
    }

    /**
     * @brief Check if the array starts with the specified prefix.
     * @param prefix array to check against
     * @return true if the array starts with the prefix, false otherwise
     */
    _SP_SAFETY_TEMPLATE_
    SP_NODISCARD SP_PURE SP_FORCEINLINE constexpr bool starts_with(const array<T, _safety_level>& prefix) const {
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(prefix._size > _size) return false;
        _SP_APPLY_UNROLLED_(prefix._size, SP_IF_NOT_EXPECT(!(_data[i]==prefix._data[i])) return false);
        return true;
    }

    /**
     * @brief Check if the array ends with the specified suffix.
     * @param suffix array to check against
     * @return true if the array ends with the suffix, false otherwise
     */
    _SP_SAFETY_TEMPLATE_
    SP_NODISCARD SP_PURE SP_FORCEINLINE constexpr bool ends_with(const array<T, _safety_level>& suffix) const {
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(suffix._size > _size) return false;
        _SP_APPLY_UNROLLED_(suffix._size, SP_IF_NOT_EXPECT(!(_data[_size - suffix._size + i] == suffix._data[i])) return false);
        return true;
    }

    /**
     * @brief Check if the array is equal to another array.
     * @param other array to check against
     * @return true if the arrays are equal, false otherwise
     */
    _SP_SAFETY_TEMPLATE_
    SP_NODISCARD SP_PURE SP_FORCEINLINE constexpr bool equals(const array<T, _safety_level>& other) const {
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(_size != other._size) return false;
        SP_IF_CONSTEXPR (_trivially_copyable && sizeof(T) == sizeof(int)){
            return std::memcmp(_data, other._data, _size * sizeof(T)) == 0;
        }
        _SP_APPLY_UNROLLED_(_size, SP_IF_NOT_EXPECT(!(_data[i]==other._data[i])) return false);
        return true;
    }

    /**
     * @brief Check if a range of the array is equal to another array.
     * @param other array to check against
     * @param start start index of the range
     * @param length length of the range
     * @return true if the ranges are equal, false otherwise
     */
    _SP_SAFETY_TEMPLATE_
    SP_NODISCARD SP_PURE SP_FORCEINLINE constexpr bool equals_range(const array<T, _safety_level>& other, ull start, ull length) const {
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(start + length > _size || length > other._size) return false;
        _SP_APPLY_UNROLLED_(length, SP_IF_NOT_EXPECT(!(_data[start + i] == other._data[i])) return false);
        return true;
    }

    /**
     * @brief Find the first element that does not match the predicate.
     * @param predicate function to test each element
     * @return index of the first element that does not match, or size() if all match
     */
    template <typename Func>
    SP_NODISCARD SP_PURE SP_FORCEINLINE constexpr ull find_if_not(Func predicate) const{
        _SP_APPLY_UNROLLED_(_size, SP_IF_NOT_EXPECT(!predicate(_data[i])) return i);
        return _size;
    }

    /**
     * @brief Find the last element that does not match the predicate.
     * @param predicate function to test each element
     * @return index of the last element that does not match, or size() if all match
     */
    template <typename Func>
    SP_NODISCARD SP_PURE SP_FORCEINLINE constexpr ull find_last_if_not(Func predicate) const{
        _SP_CHECK_SAFETY_LEVEL_(1) SP_IF_NOT_EXPECT(_size == 0) return _size;
    #if __SP_UNROLL_LOOPS__ == 1
        ull i = _size;
        while(i >= 4){
            i -=4;
            SP_IF_NOT_EXPECT(!predicate(_data[i+3])) return i+3;
            SP_IF_NOT_EXPECT(!predicate(_data[i+2])) return i+2;
            SP_IF_NOT_EXPECT(!predicate(_data[i+1])) return i+1;
            SP_IF_NOT_EXPECT(!predicate(_data[i])) return i;
        }
        while(i > 0){
            i--;
            SP_IF_NOT_EXPECT(!predicate(_data[i])) return i;
        }
    #else
    for(ull i = _size-1; i > 0; i--) SP_IF_NOT_EXPECT(!predicate(_data[i])) return i;
    #endif
        return _size;
    }

    /**
     * @brief Find the nth occurrence of the specified value.
     * @param value element to find
     * @param n occurrence to find (0-based)
     * @return index of the nth occurrence, or size() if not found
     */
    _SP_SAFETY_TEMPLATE_
    SP_NODISCARD SP_PURE SP_FORCEINLINE constexpr ull find_nth(type_param value, ull n) const{
        ull count = 1;
        _SP_APPLY_UNROLLED_(
            _size,
            if(_data[i]==value){
                SP_IF_NOT_EXPECT(count==n) return i;
                count++;
            }
        );
        return _size;
    }

    /**
     * @brief Find the nth element that matches the predicate.
     * @param predicate function to test each element
     * @param n occurrence to find (0-based)
     * @return index of the nth matching element, or size() if not found
     */
    template <typename Func>
    SP_NODISCARD SP_PURE SP_FORCEINLINE constexpr ull find_nth_if(Func predicate, ull n) const{
        ull count = 1;
        _SP_APPLY_UNROLLED_(
            _size,
            if(predicate(_data[i])){
                SP_IF_NOT_EXPECT(count==n) return i;
                count++;
            }
        );
        return _size;
    }

    /**
     * @brief Find the range of a subarray within the array.
     * @param subarray array to find
     * @return index of the start of the subarray, or size() if not found
     */
    _SP_SAFETY_TEMPLATE_
    SP_NODISCARD SP_PURE SP_FORCEINLINE constexpr ull find_range(const array<T, _safety_level>& subarray) const {
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(subarray._size == 0 || subarray._size > _size) return _size;
        for (ull i = 0; i <= _size - subarray._size; i++) {
            bool match = true;
            for (ull j = 0; j < subarray._size; j++) {
                SP_IF_NOT_EXPECT(!(_data[i + j] == subarray._data[j])) {
                    match = false;
                    break;
                }
            }
            SP_IF_NOT_EXPECT(match) return i;
        }
        return _size;
    }

    /**
     * @brief Find the last occurrence of a subarray within the array.
     * @param subarray array to find
     * @return index of the start of the last occurrence of the subarray, or size() if not found
     */
    _SP_SAFETY_TEMPLATE_
    SP_NODISCARD SP_PURE SP_FORCEINLINE constexpr ull find_last_range(const array<T, _safety_level>& subarray) const {
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(subarray._size == 0 || subarray._size > _size) return _size;
        for (ull i = _size - subarray._size; i != (ull)-1; i--) {
            bool match = true;
            for (ull j = 0; j < subarray._size; j++) {
                SP_IF_NOT_EXPECT(!(_data[i + j] == subarray._data[j])) {
                    match = false;
                    break;
                }
            }
            SP_IF_NOT_EXPECT(match) return i;
        }
        return _size;
    }

    /**
     * @brief Check if any element matches the predicate.
     * @param predicate function to test each element
     * @return true if any element matches, false otherwise
     */
    template <typename Func>
    SP_NODISCARD SP_PURE SP_FORCEINLINE constexpr bool any_of(Func predicate) const {
        _SP_APPLY_UNROLLED_(_size, SP_IF_NOT_EXPECT(predicate(_data[i])) return true);
        return false;
    }

    /**
     * @brief Check if all elements match the predicate.
     * @param predicate function to test each element
     * @return true if all elements match, false otherwise
     */
    template <typename Func>
    SP_NODISCARD SP_PURE SP_FORCEINLINE constexpr bool all_of(Func predicate) const {
        _SP_APPLY_UNROLLED_(_size, SP_IF_NOT_EXPECT(!predicate(_data[i])) return false);
        return true;
    }

    /**
     * @brief Check if no elements match the predicate.
     * @param predicate function to test each element
     * @return true if no elements match, false otherwise
     */
    template <typename Func>
    SP_NODISCARD SP_PURE SP_FORCEINLINE constexpr bool none_of(Func predicate) const {
        _SP_APPLY_UNROLLED_(_size, SP_IF_NOT_EXPECT(predicate(_data[i])) return false);
        return true;
    }

    /**
     * @brief Get a slice of the array.
     * @param start start index of the slice
     * @param length length of the slice
     * @return a new array containing the sliced elements
     */
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NI_ constexpr array<T, _safety_level> slice(ull start, ull length) const {
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(start + length > _size) throw exceptions::ArrayException("Slice out of range.");
        array<T, _safety_level> result(length);
        _SP_APPLY_UNROLLED_(length, result[i] = _data[start+i]);
        return result;
    }

    /**
     * @brief Erase a range of elements from the array.
     * @param start start index of the range
     * @param length length of the range
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE constexpr void erase_range(ull start, ull length){
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(start + length > _size) throw exceptions::ArrayException("Range out of range.");
        _SP_EXPLICIT_UNROLLED_(i, start, start + length, sp::allocator_traits<Allocator<T>>::destroy(_alloc,_data+i));
        _SP_EXPLICIT_UNROLLED_(i, start + length, _size, sp::allocator_traits<Allocator<T>>::construct(_alloc,_data+i-length,sp::move(_data[i])));
        _SP_EXPLICIT_UNROLLED_(i, _size - length, _size, sp::allocator_traits<Allocator<T>>::destroy(_alloc,_data+i));
        _size -= length;
    }

    /**
     * @brief Insert a range of elements into the array.
     * @param index index at which to insert
     * @param other array to insert
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE constexpr void insert_range(ull index, const array<T, _safety_level>& other){
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(index > _size) throw exceptions::ArrayException("Index out of range.");
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(_size + other._size > _capacity) reallocate(grow_capacity(_size + other._size));
        for(ull i = _size + other._size - 1; i >= index + other._size; i--) sp::allocator_traits<Allocator<T>>::construct(_alloc,_data+i,sp::move(_data[i - other._size]));
        _SP_APPLY_UNROLLED_(other._size, sp::allocator_traits<Allocator<T>>::construct(_alloc,_data+index+i,other._data[i]));
        _size += other._size;
    }

    /**
     * @brief Append elements from another array to the end of this array.
     * @param other array to append
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE constexpr void append(const array<T, _safety_level>& other){
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(_size + other._size > _capacity) reallocate(grow_capacity(_size + other._size));
        _SP_APPLY_UNROLLED_(other._size, sp::allocator_traits<Allocator<T>>::construct(_alloc,_data+_size+i,other._data[i]));
        _size += other._size;
    }

    /**
     * @brief Prepend elements from another array to the beginning of this array.
     * @param other array to prepend
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE constexpr void prepend(const array<T, _safety_level>& other){
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(_size + other._size > _capacity) reallocate(grow_capacity(_size + other._size));
        for(ull i = _size + other._size - 1; i >= other._size; i--) sp::allocator_traits<Allocator<T>>::construct(_alloc,_data+i,sp::move(_data[i - other._size]));
        _SP_APPLY_UNROLLED_(other._size, sp::allocator_traits<Allocator<T>>::construct(_alloc,_data+i,other._data[i]));
        _size += other._size;
    }

    /**
     * @brief Replace a range of elements in the array.
     * @param start start index of the range
     * @param length length of the range
     * @param other array to replace with
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE constexpr void replace_range(ull start, ull length, const array<T, _safety_level>& other){
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(start + length > _size) throw exceptions::ArrayException("Range out of range.");
        if(length != other._size){
            erase_range(start, length);
            insert_range(start, other);
        } else _SP_APPLY_UNROLLED_(length, _data[start+i] = other._data[i]);
    }

    /**
     * @brief Shrink the array by a certain amount.
     * @param amount amount to shrink by
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE constexpr void shrink_by(ull amount){
        SP_IF_NOT_EXPECT(amount >= _size) clear();
        else { _SP_APPLY_UNROLLED_(_size, sp::allocator_traits<Allocator<T>>::destroy(_alloc,_data+i)); _size -= amount; }
    }

    /**
     * @brief Grow the array by a certain amount.
     * @param amount amount to grow by
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE constexpr void grow_by(ull amount){
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(_size + amount > _capacity) reallocate(grow_capacity(_size + amount));
        _SP_APPLY_UNROLLED_(_size + amount, sp::allocator_traits<Allocator<T>>::construct(_alloc,_data+i));
        _size += amount;
    }

     /**
     * @brief Trim the array by removing trailing elements equal to the default-constructed value.
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE constexpr void trim(){
        T comparison = T();
        while(_size > 0 && _data[_size - 1] == comparison) {
            sp::allocator_traits<Allocator<T>>::destroy(_alloc,_data+_size-1);
            _size--;
        }
    }

    /**
     * @brief Map a function over the elements of the array.
     * @param func function to apply
     * @return new array with mapped values
     */
    template <typename Func>
    _SP_FUNC_NI_ constexpr array<T, _safety_level> map(Func func) const {
        array<T, _safety_level> result(_size);
        _SP_APPLY_UNROLLED_(_size, result[i] = func(_data[i]));
        return result;
    }

    /**
     * @brief Filter the elements of the array based on a predicate.
     * @param predicate function to test each element
     * @return new array with filtered values
     */
    template <typename Func>
    _SP_FUNC_NI_ constexpr array<T, _safety_level> filter(Func predicate) const{
        array<T, _safety_level> result;
        _SP_APPLY_UNROLLED_(_size, if(predicate(_data[i])) result.push_back(_data[i]));
        return result;
    }

    /**
     * @brief filter the elements of the array based on a predicate in place.
     * @param predicate function to test each element
     * @return *this: enables chaining
     * @warning modifies the original
     */
    template <typename Func>
    SP_FORCEINLINE constexpr array<T, _safety_level>& filter_(Func predicate) {
        _SP_APPLY_UNROLLED_(_size, if(!predicate(_data[i])){
            erase(i);
            i--;
        });
        return *this;
    }

    /**
     * @brief Reduce the elements of the array using a binary function.
     * @param func binary function to apply
     * @param initial initial value for the reduction
     * @return reduced value
     */
    template <typename Func>
    _SP_FUNC_NI_ constexpr T reduce(Func func, T initial) const {
        T result = initial;
        _SP_APPLY_UNROLLED_(_size, result = func(result, _data[i]));
        return result;
    }

    /**
     * @brief Remove duplicate elements from the array.
     * @return resulting array
     */
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NI_ constexpr array<T, _safety_level> unique(){
        array<T, _safety_level> result;
        _SP_APPLY_UNROLLED_(_size, if(!result.contains(_data[i])) result.push_back(_data[i]));
        return result;
    }

    /**
     * @brief Remove duplicate elements from the array.
     * @return *this: enables chaining
     * @warning modifies the original
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE constexpr array<T, _safety_level>& unique_(){
        array<T, _safety_level> result;
        _SP_APPLY_UNROLLED_(_size, if(!result.contains(_data[i])) result.push_back(_data[i]));
        swap(result);
        return *this;
    }

    /**
     * @brief Remove consecutive duplicate elements from the array.
     * @return result: The filtered array
     */
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NI_ constexpr array<T, _safety_level> dedupe_consecutive(){
        array<T, _safety_level> result;
        result.push_back(_data[0]);
        _SP_EXPLICIT_UNROLLED_(i, 1, _size, if(!(_data[i]==_data[i-1])) result.push_back(_data[i]));
        return result;
    }

    /**
     * @brief Remove consecutive duplicate elements from the array.
     * @return *this: enables chaining
     * @warning modifies the original
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE constexpr array<T, _safety_level> dedupe_consecutive_(){
        _SP_CHECK_SAFETY_(1) if(_size==0)_SP_UNLIKELY_ throw exceptions::ArrayException("Cannot dedupe an empty array.");
        array<T, _safety_level> result;
        result.push_back(_data[0]);
        _SP_EXPLICIT_UNROLLED_(i, 1, _size, if(!(_data[i]==_data[i-1])) result.push_back(_data[i]));
        swap(result);
        return *this;
    }

    /**
     * @brief Check if an index is valid for the array.
     * @param index index to check
     * @return true if the index is valid, false otherwise
     */
    _SP_SAFETY_TEMPLATE_
    SP_NODISCARD SP_PURE SP_FORCEINLINE constexpr bool is_valid_index(ull index) const {
        return index < _size;
    }

    /**
     * @brief Get a reference to the element at the specified index without bounds checking.
     * @param index index of the element
     * @return reference to the element
     */
    _SP_SAFETY_TEMPLATE_
    SP_NODISCARD SP_PURE SP_FORCEINLINE constexpr T& at_unchecked(ull index) {
        return _data[index];
    }

    /**
     * @brief Assign values from an initializer list to the array.
     * @param list initializer list of values
     * @return reference to the array
     */
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE constexpr array& assign(std::initializer_list<T> list) {
        clear_and_resize(list.size());
        ull index = 0;
        for (type_param item : list) _data[index++] = item;
        return *this;
    }

    /**
     * @brief Create a copy of the array.
     * @return new array with copied values
     */
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NI_ constexpr array clone() const {
        array copy(_size);
        _SP_APPLY_UNROLLED_(_size, copy[i] = _data[i]);
        return copy;
    }

    /**
     * @brief Create a new array filled with a specific value.
     * @param size size of the new array
     * @param value value to fill the array with
     * @return new array filled with the specified value
     */
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NI_ static constexpr array<T, _safety_level> filled(ull size, type_param value) {
        array<T, _safety_level> arr(size);
        _SP_APPLY_UNROLLED_(size, arr[i] = value);
        return arr;
    }

    /**
     * @brief Create a new array with a specific capacity.
     * @param capacity capacity of the new array
     * @return new array with the specified capacity
     */
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NI_ static constexpr array<T, _safety_level> with_capacity(ull capacity) {
        array<T, _safety_level> arr;
        arr.reserve(capacity);
        return arr;
    }

    /**
     * @brief Partition the array around a pivot element.
     * @param pivot_index index of the pivot element
     */
    _SP_SAFETY_TEMPLATE_
    constexpr void partition(ull pivot_index){
        _SP_CHECK_SAFETY_(1) if (pivot_index >= _size)_SP_UNLIKELY_ throw exceptions::ArrayException("Index out of range.");
        T pivot = _data[pivot_index];
        ull i = 0;
        ull j = _size - 1;
        while(i <= j){
            while(i < _size && _data[i] < pivot) i++;
            while(j > 0 && _data[j] >= pivot) j--;
            if(i < j){
                swap_elements(i, j);
                i++;
                j--;
            }
        }
    }

    /**
     * @brief Stable partition the array around a pivolt element.
     * @param pivot_index index of the pivot element
     */
    _SP_SAFETY_TEMPLATE_
    constexpr void stable_partition(ull pivot_index){
        _SP_CHECK_SAFETY_(1) if (pivot_index >= _size)_SP_UNLIKELY_ throw exceptions::ArrayException("Index out of range.");
        T pivot = _data[pivot_index];
        array<T, _safety_level> less;
        array<T, _safety_level> greater_equal;
        for(ull i = 0; i < _size; i++){
            if(_data[i] < pivot) less.push_back(_data[i]); 
            else greater_equal.push_back(_data[i]);
        }
        ull index = 0;
        _SP_APPLY_UNROLLED_(less._size, _data[index++] = less[i]);
        _SP_APPLY_UNROLLED_(greater_equal._size, _data[index++] = greater_equal[i]);
    }
};

template <typename T, short safety>
std::ostream& operator<<(std::ostream& os, const array<T, safety>& arr) {
    os << sp::console::FG_BRIGHT_MAGENTA << "[" << sp::console::RESET_EFFECTS;
    for (ull i = 0; i < arr.size(); i++) {
        os << arr[i];
        if (i < arr.size() - 1) os << ", ";
    }
    os << sp::console::FG_BRIGHT_MAGENTA << "]" << sp::console::RESET_EFFECTS;
    return os;
}

template <typename T> using vector = sp::array<T, 1, sp::allocator>;
template <typename T> using uvector = sp::array<T, 0, sp::allocator>;

template <typename T> using aligned_vec = sp::array<T, 1, sp::aligned_allocator>;
template <typename T> using aligned_uvec = sp::array<T, 0, sp::aligned_allocator>;

template <typename T> using page_vec = sp::array<T, 1, sp::page_allocator>;
template <typename T> using page_uvec = sp::array<T, 0, sp::page_allocator>;

template <typename T, size_type N> using stack_vec = sp::array<T, 1, bind_alloc<sp::unbinded_stack_allocator, N>::template r>;
template <typename T, size_type N> using stack_uvec = sp::array<T, 0, bind_alloc<sp::unbinded_stack_allocator, N>::template r>;

namespace vec{
    template <typename T> using vector = sp::array<T, 1, sp::allocator>;
    template <typename T> using uvector = sp::array<T, 0, sp::allocator>;

    template <typename T> using aligned_vec = sp::array<T, 1, sp::aligned_allocator>;
    template <typename T> using aligned_uvec = sp::array<T, 0, sp::aligned_allocator>;

    template <typename T> using page_vec = sp::array<T, 1, sp::page_allocator>;
    template <typename T> using page_uvec = sp::array<T, 0, sp::page_allocator>;

    template <typename T, size_type N> using stack_vec = sp::array<T, 1, bind_alloc<sp::unbinded_stack_allocator, N>::template r>;
    template <typename T, size_type N> using stack_uvec = sp::array<T, 0, bind_alloc<sp::unbinded_stack_allocator, N>::template r>;
};


}
#ifdef _SP_CHECK_SAFETY_
    #undef _SP_CHECK_SAFETY_
#endif

#ifdef _SP_CHECK_SAFETY_LEVEL_
    #undef _SP_CHECK_SAFETY_LEVEL_
#endif

#ifdef _SP_SAFETY_TEMPLATE_
    #undef _SP_SAFETY_TEMPLATE_
#endif

#endif // ____SP_ARRAY_HPP____