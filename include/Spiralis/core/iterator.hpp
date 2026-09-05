#ifndef ____SP_ITERATOR_HPP____
#define ____SP_ITERATOR_HPP____
#pragma once
#include "../setup/init.hpp"

namespace sp{

template <typename T>
class iterator{
private:
    T* _data;
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using difference_type = ptrdiff_t;
    // size_type is defined in init.hpp global namespace

    SP_FORCEINLINE constexpr iterator(pointer p = nullptr) : _data(p){}
    SP_FORCEINLINE constexpr iterator(const iterator& other) : _data(other._data) {}
    SP_FORCEINLINE constexpr iterator& operator=(const iterator& other) {
        SP_IF_EXPECT(this != &other) {
            _data = other._data;
        }
        return *this;
    }

    SP_FORCEINLINE constexpr iterator(pointer p, size_type index) : _data(p + index) {}

    SP_FORCEINLINE constexpr reference operator*() const { return *_data; }
    SP_FORCEINLINE constexpr pointer operator->() const { return _data; }

    SP_FORCEINLINE constexpr iterator& operator++() { ++_data; return *this; }
    SP_FORCEINLINE constexpr iterator& operator--() { --_data; return *this; }

    SP_FORCEINLINE constexpr iterator operator++(int) { iterator tmp(*this); ++_data; return tmp; }
    SP_FORCEINLINE constexpr iterator operator--(int) { iterator tmp(*this); --_data; return tmp; }

    SP_FORCEINLINE constexpr iterator& operator+=(difference_type n) { _data += n; return *this; }
    SP_FORCEINLINE constexpr iterator& operator-=(difference_type n) { _data -= n; return *this; }

    SP_FORCEINLINE constexpr friend bool operator==(const iterator& a, const iterator& b) { return a._data == b._data; }
    SP_FORCEINLINE constexpr friend bool operator!=(const iterator& a, const iterator& b) { return a._data != b._data; }
    SP_FORCEINLINE constexpr friend bool operator<(const iterator& a, const iterator& b) { return a._data < b._data; }
    SP_FORCEINLINE constexpr friend bool operator>(const iterator& a, const iterator& b) { return a._data > b._data; }
    SP_FORCEINLINE constexpr friend bool operator<=(const iterator& a, const iterator& b) { return a._data <= b._data; }
    SP_FORCEINLINE constexpr friend bool operator>=(const iterator& a, const iterator& b) { return a._data >= b._data; }

    SP_FORCEINLINE constexpr friend iterator operator+(const iterator& a, difference_type n) { return iterator(a._data + n); }
    SP_FORCEINLINE constexpr friend iterator operator-(const iterator& a, difference_type n) { return iterator(a._data - n); }

    SP_FORCEINLINE constexpr friend difference_type operator-(const iterator& a, const iterator& b) { return a._data - b._data; }

    SP_FORCEINLINE constexpr reference operator[](difference_type n) const { return _data[n]; }
};
template <typename It>
SP_FORCEINLINE constexpr It operator+(typename It::difference_type n, const It& it) { return it + n; }
}


namespace sp{

template <typename T>
using string_iterator = iterator<T>;

template <typename T>
using array_iterator = iterator<T>;

template <typename T>
using flat_tensor_iterator = iterator<T>;

}; // namespace sp

#endif