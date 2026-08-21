#ifndef ____SP_POINTER____
#define ____SP_POINTER____
#pragma once
#include "../setup/init.hpp"
#include "../core/type_traits.hpp"
namespace sp{

template <typename T, bool track_size = true, template <typename> class Allocator = sp::allocator>
class ptr_list{
private:
    T* data = nullptr;
    size_type size = 0;
    SP_NO_UNIQUE_ADDRESS Allocator<T> alloc;
    SP_FORCEINLINE constexpr void destroy(){
        if(data!=nullptr){
            for(ull idx = 0; idx < size; ++idx) sp::allocator_traits<Allocator<T>>::destroy(alloc, &data+idx);
            sp::allocator_traits<Allocator<T>>::deallocate(alloc, data, size);
        }
    }
public:
    SP_FORCEINLINE constexpr ptr_list(T val){
        data = sp::allocator_traits<Allocator<T>>::allocate(alloc, 1);
        sp::allocator_traits<Allocator<T>>::construct(alloc, data, val);
        if constexpr(track_size) size = 1;
    }
    SP_FORCEINLINE constexpr ptr_list(std::initializer_list<T> vals){
        size = vals.size();
        size_type idx = 0;
        sp::allocator_traits<Allocator<T>>::allocate(alloc, size);
        for(auto& i : vals) sp::allocator_traits<Allocator<T>>::construct(alloc, &data+idx, i);
    }
    SP_FORCEINLINE constexpr ptr_list(size_type sz, T default_val){ 
        size = sz;
        sp::allocator_traits<Allocator<T>>::allocate(alloc, size);
        for(size_type idx = 0; idx < sz; ++idx) sp::allocator_traits<Allocator<T>>::construct(alloc, &data+idx, default_val);
    }
    SP_FORCEINLINE constexpr ptr_list(const ptr_list& other) = delete;
    SP_FORCEINLINE constexpr ptr_list(ptr_list&& other){
        if(data) destroy();
        data = other.data;
        size = other.size;
        alloc = sp::move(other.alloc);
        other.data = nullptr;
        other.size = 0;
    }
    SP_FORCEINLINE constexpr T* get() { return data; }
    SP_FORCEINLINE constexpr const T* cget() const { return data; }
    SP_FORCEINLINE constexpr sp::pair<T*, size_type> get_pair() { return {data, size}; }
    SP_FORCEINLINE constexpr const sp::pair<T*, size_type> cget_pair() const { return {data, size}; }
    SP_FORCEINLINE constexpr ~ptr_list(){ 
        destroy();
    }
};

template <typename T, template <typename> class Allocator = sp::allocator>
class ptr{
private:
    T* data = nullptr;
    SP_NO_UNIQUE_ADDRESS Allocator<T> alloc;
    SP_FORCEINLINE constexpr void destroy(){ 
        if(data!=nullptr){
            sp::allocator_traits<Allocator<T>>::destroy(alloc, &data);
            sp::allocator_traits<Allocator<T>>::deallocate(alloc, data, 1);
        }
    }
public:
    SP_FORCEINLINE constexpr ptr(T val){
        data = sp::allocator_traits<Allocator<T>>::allocate(alloc, 1);
        sp::allocator_traits<Allocator<T>>::construct(alloc, data, val);
    }
    SP_FORCEINLINE constexpr ~ptr(){
        destroy();
    }
    SP_FORCEINLINE constexpr ptr(const ptr& other) = delete;
    SP_FORCEINLINE constexpr ptr(ptr&& other){
        data = other.data;
        alloc = sp::move(other.alloc);
        other.data = nullptr;
    }


    SP_FORCEINLINE constexpr T* get() { return data; }
    SP_FORCEINLINE constexpr const T* cget() const { return data; }
};

};
#endif