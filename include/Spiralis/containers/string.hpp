#ifndef ____SP_STRING____
#define ____SP_STRING____
#pragma once
#include "../setup/init.hpp"
#include "../core/allocators.hpp"
#include "../containers/pair.hpp"
#include "../core/exceptions.hpp"
#include "../math/math.hpp"
#include "../math/bit_manip.hpp"
#include "../core/iterator.hpp"
#include <cstring>
namespace sp{

#ifndef _SP_CHECK_SAFETY_LEVEL_
    #undef _SP_CHECK_SAFETY_LEVEL_
    #define _SP_CHECK_SAFETY_LEVEL_(level) SP_IF_CONSTEXPR(_safety_level>=level)
#endif

SP_HOT SP_FORCEINLINE constexpr ull strlen(const char* s) noexcept {
#if defined(__cpp_lib_is_constant_evaluated)
    if(__builtin_is_constant_evaluated()){
        const char* p = s;
        while (*p) ++p;
        return static_cast<ull>(p - s);
    }
#endif
#if __has_builtin(__builtin_strlen) || defined(__GNUC__)
    return __builtin_strlen(s);
#elif defined(_MSC_VER)
    return ::strlen(s);
#else
    const char* p = s;
    while (*p) ++p;
    return static_cast<size_type>(p - s);
#endif
}

SP_FORCEINLINE constexpr bool isspace(char c) { return (c >= 0 && c <= 127) && (c == ' ' || (c >= '\t' && c <= '\r')); }


template <short _safety_level = __SP_DEFAULT_SAFETY_LEVEL__, template <typename> typename Allocator = sp::allocator, bool _use_sso=true>
class string_impl{
private:
    friend class string_view;
    _SP_GRANT_IO_ACCESS_
    SP_FORCEINLINE const char* __getSpiralMessage() const { return c_str(); }
    SP_FORCEINLINE sp::pair<const char*, size_type> __getSpiralBinary() const { return {c_str(), size()*sizeof(char)}; }
    static constexpr short _other_safety_level = (short)(!((bool)_safety_level));
    template <typename Alloc, typename = void> struct allocator_ext {
        static constexpr ull true_capacity(ull n) noexcept { return n; }
    };
    template <typename Alloc> struct allocator_ext<Alloc, spt::void_t<decltype(Alloc::capacity_for(spt::declval<ull>()))>> {
        static constexpr ull true_capacity(ull n) noexcept { return Alloc::capacity_for(n); }
    };
    struct __small_mode{
        char _data[23]{};
        unsigned char _flag{0};   // Overlaps with the last byte of big mode _capacity
        constexpr __small_mode(){}
        constexpr void push_back(char element) {
            _flag &= 0x7F; // remove MSB
            _data[_flag] = element; // replace null terminator with element
            _data[++_flag] = '\0'; // increment size and add null terminator
            _flag |= 0x80; // re-apply MSB
        }
        constexpr void append(const char* element, size_type len, size_type combined_len){
            _flag &= 0x7F;
            memcpy(_data+_flag, element, len);
            _flag = combined_len;
            _data[_flag] = '\0';
            _flag |= 0x80;
        }
    };
    struct __big_mode{
        char* _data=nullptr;
        size_type _size=0;
        size_type _capacity=0;
        SP_NO_UNIQUE_ADDRESS Allocator<char> _alloc;

        constexpr __big_mode(){}
        constexpr __big_mode(__big_mode&& other) : _data(other._data), _size(other._size), _capacity(other._capacity){
            other._data = nullptr;
            other._size = 0;
            other._capacity = 0;
        }
        SP_CONSTEXPR20 ~__big_mode(){
            sp::allocator_traits<Allocator<char>>::deallocate(_alloc,_data,_capacity);
            _data = nullptr;
        }

        constexpr void reserve(size_type min_bytes){
            size_type true_cap = allocator_ext<Allocator<char>>::true_capacity(next_pow2(min_bytes));
            _data = sp::allocator_traits<Allocator<char>>::allocate(_alloc,true_cap);
            _capacity = true_cap;
            _size = 0;
        }

        constexpr void reserve_exact(size_type bytes){
            size_type true_cap = allocator_ext<Allocator<char>>::true_capacity(bytes);
            _data = sp::allocator_traits<Allocator<char>>::allocate(_alloc,true_cap);
            _capacity = true_cap;
            _size = 0;
        }

        constexpr void reallocate(size_type cap){
            size_type true_cap = allocator_ext<Allocator<char>>::true_capacity(cap);
            char* temp = sp::allocator_traits<Allocator<char>>::allocate(_alloc,true_cap);
            if(_data) memcpy(temp,_data,_size);
            _data = temp;
            _capacity = true_cap;
        }

        SP_FORCEINLINE SP_HOT constexpr void push_back(char elem){
            _SP_CHECK_SAFETY_LEVEL_(1) SP_IF_NOT_EXPECT(_size==_capacity) reallocate(next_pow2(_capacity+1));
            SP_IF_CONSTEXPR((spt::is_base_of_v<_spiral_alloc_traits,Allocator<char>>)){
                SP_IF_CONSTEXPR(Allocator<char>::can_avoid_construct_on_trivially_copyable){
                    _data[_size++] = elem;
                    return;
                }
            }
            sp::allocator_traits<Allocator<char>>::construct(_alloc,_data+_size,elem);
            _size++;
        }
    };
    union __owner_storage {
        __big_mode _B;
        __small_mode _S;
        SP_CONSTEXPR20 __owner_storage() {}
        SP_CONSTEXPR20 ~__owner_storage() {}
    };
    struct __no_sso{
        __big_mode _B;
        constexpr __no_sso(){}
        SP_CONSTEXPR20 ~__no_sso(){}
    };
    spt::conditional_t<_use_sso,__owner_storage,__no_sso> _str;
    
    SP_FORCEINLINE constexpr __big_mode& B() noexcept { return _str._B; }
    SP_FORCEINLINE constexpr const __big_mode& B() const noexcept { return _str._B; }

    SP_FORCEINLINE constexpr __small_mode& S() noexcept { return _str._S; }
    SP_FORCEINLINE constexpr const __small_mode& S() const noexcept { return _str._S; }

    template <bool __is_small>
    constexpr void set_flag() {
        SP_IF_CONSTEXPR(__is_small)  S()._flag |= 0x80;  
        else S()._flag &= 0x7F;  
    }

    SP_FORCEINLINE constexpr void set_small_length(size_type len) {
        S()._flag = (S()._flag & 0x80) | (len & 0x1F);
    }

    SP_FORCEINLINE constexpr void _inflate(size_type new_cap) {
        char temp[23]{};
        size_type current_sz = size(); 
        memcpy(temp, S()._data, current_sz);

        SP_IF_NOT_EXPECT(is_big()) B().~__big_mode();
        new (&B()) __big_mode();

        B().reserve(new_cap); // rounds to next pow2
        
        memcpy(B()._data, temp, current_sz);
        B()._size = current_sz;
        B()._data[current_sz] = '\0';
    }

    SP_FORCEINLINE constexpr void _deflate() {
        char temp[23]{};
        size_type current_sz = size();
        memcpy(temp, B()._data, current_sz);

        B().~__big_mode(); // Destroy big mode data

        memcpy(S()._data, temp, current_sz);
        S()._data[current_sz] = '\0';
        set_flag<true>();
        set_small_length(current_sz);
    }

// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// ============================================ PRIVATE HELPERS =============================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================


SP_FORCEINLINE constexpr string_impl& __priv_assign(const char* other, size_type len){
    if (len <= 22) {
        if (is_big()) B().~__big_mode();
        memcpy(S()._data, other, len);
        S()._data[len] = '\0';
        set_flag<true>();
        set_small_length(len);
    }else{
        if (is_small()) _inflate(len);
        memcpy(B()._data, other, len);
        B()._size = len;
        B()._data[len] = '\0';
    }
    return *this;
}

SP_FORCEINLINE constexpr string_impl& __priv_push_back(const char element){
    size_type len = length();
    if(len<22&&is_small()){
        S().push_back(element);
    }else{
        SP_IF_NOT_EXPECT(is_small()) _inflate(len+1);
        else SP_IF_NOT_EXPECT(len>B()._capacity) B().reallocate(next_pow2(len+1));
        B().push_back('\0'); // push_back handles reallocations and changing the length
        B()._data[len] = element; // set the new last char
    }
    return *this;
}

SP_FORCEINLINE constexpr string_impl& __priv_append(const char* other, size_type len){
    size_type current_len = length();
    size_type combined_len = current_len + len;
    
    if(combined_len <= 22&&is_small()){
        S().append(other, len, combined_len); 
    }else {
        SP_IF_NOT_EXPECT(is_small()) _inflate(combined_len + 1); 
        else SP_IF_NOT_EXPECT(combined_len >= B()._capacity) B().reallocate(next_pow2(combined_len+1));
        memcpy(B()._data + current_len, other, len);
        B()._size = combined_len;
        B()._data[combined_len] = '\0'; // Safe now because of the +1 above
    }
    return *this;
}

SP_FORCEINLINE constexpr string_impl& __priv_insert(size_type index, const char* other, size_type len){
    size_type current_len = length();
    _SP_CHECK_SAFETY_LEVEL_(1) SP_IF_NOT_EXPECT(index > current_len) throw std::out_of_range("Index out of range in string_impl insert");
    size_type combined_len = current_len + len;
    if(combined_len <= 22&&is_small()){
        char* SP_RESTRICT dta = S()._data;
        memmove(dta+index+len, dta+index, current_len-index);
        memcpy(dta+index, other, len);
        set_small_length(combined_len); // new length
        dta[combined_len] = '\0'; // null terminate at end
    }else{
        SP_IF_NOT_EXPECT(is_small()) _inflate(combined_len+1); // create big data
        else SP_IF_NOT_EXPECT(combined_len > B()._capacity) B().reallocate(next_pow2(combined_len+1)); // reallocation check
        char* SP_RESTRICT dta = B()._data;
        memmove(dta + index + len, dta + index, current_len - index); // Shift existing data to the right
        memcpy(dta + index, other, len); // Insert new data
        B()._size = combined_len; // new size
        dta[combined_len] = '\0'; // null terminate at end
    }
    return *this;
}

SP_FORCEINLINE constexpr string_impl& __priv_replace(size_type index, size_type len_to_replace, const char* other, size_type len){
    size_type current_len = length();
    _SP_CHECK_SAFETY_LEVEL_(1) SP_IF_NOT_EXPECT(index > current_len) throw std::out_of_range("Index out of range in string_impl replace");
    size_type new_len = current_len - len_to_replace + len;
    if(new_len <= 22&&is_small()){
        char* SP_RESTRICT dta = S()._data;
        memmove(dta + index + len, dta + index + len_to_replace, current_len - index - len_to_replace); // Shift existing data to the right or left depending on len vs len_to_replace
        memcpy(dta + index, other, len); // Insert new data
        set_small_length(new_len); // new length
        dta[new_len] = '\0'; // null terminate at end
    }else{
        SP_IF_NOT_EXPECT(is_small()) _inflate(new_len+1); // create big data
        else SP_IF_NOT_EXPECT(new_len > B()._capacity) B().reserve(new_len+1); // reallocation check
        char* SP_RESTRICT dta = B()._data;
        memmove(dta + index + len, dta + index + len_to_replace, current_len - index - len_to_replace); // Shift existing data to the right
        memcpy(dta + index, other, len); // Insert new data
        B()._size = new_len; // new size
        dta[new_len] = '\0'; // null terminate at end
    }
    return *this;
}




// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// ================================================= ACCESS =================================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================

public:

using iterator = string_iterator<char>;
using const_iterator = string_iterator<const char>;

SP_FORCEINLINE constexpr iterator begin() noexcept { return iterator(data()); }
SP_FORCEINLINE constexpr const_iterator begin() const noexcept { return const_iterator(data()); }
SP_FORCEINLINE constexpr const_iterator cbegin() const noexcept { return const_iterator(data()); }

SP_FORCEINLINE constexpr iterator end() noexcept { return iterator(data() + size()); }
SP_FORCEINLINE constexpr const_iterator end() const noexcept { return const_iterator(data()+size()); }
SP_FORCEINLINE constexpr const_iterator cend() const noexcept { return const_iterator(data()+size()); }

SP_NODISCARD SP_FORCEINLINE constexpr bool is_empty() const{ return size() == 0;}
_SP_FUNC_NIP_ constexpr bool is_small() const{ return S()._flag & 0x80; }
_SP_FUNC_NIP_ constexpr bool is_big() const{ return !is_small(); }
constexpr size_type size() const{ return is_small() ? (S()._flag & 0x1F) : B()._size; }
constexpr size_type length() const{ return is_small() ? (S()._flag & 0x1F) : B()._size; } // alias for size
constexpr size_type capacity() const{ return is_small() ? 22 : B()._capacity; }

constexpr char* data() { return is_small() ? S()._data : B()._data; }
constexpr const char* data() const { return is_small() ? S()._data : B()._data; }
constexpr const char* c_str() const { return (is_small() ? S()._data : B()._data); }

SP_NODISCARD SP_FORCEINLINE constexpr string_impl C() const { return string_impl(*this); }
SP_NODISCARD SP_FORCEINLINE constexpr string_impl clone() const { return string_impl(*this); }


// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// =========================================== OPERATOR OVERLOADS ===========================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================

_SP_FUNC_NIP_ constexpr bool operator==(const string_impl& other)const{
    SP_IF_NOT_EXPECT(this==&other) return true;
    return (size() == other.size()) && (memcmp(c_str(), other.c_str(), size()) == 0);
}
_SP_FUNC_NIP_ constexpr bool operator==(const char* other)const{
    return (size() == sp::strlen(other)) && (memcmp(c_str(), other, size()) == 0);
}

_SP_FUNC_NIFP_ constexpr bool operator!=(const string_impl& other)const{
    return !(*this==other);
}

SP_FLATTEN constexpr string_impl& operator=(const string_impl& other){
    return __priv_assign(other.data(), other.length());
}

constexpr string_impl& operator=(string_impl&& other) noexcept {
    SP_IF_NOT_EXPECT(this == &other) return *this;
    if(is_big()) B().~__big_mode();
    if(other.is_small()) {
        memcpy(S()._data, other.S()._data, 23);
        S()._flag = other.S()._flag;
    }
    else new(&B()) __big_mode(sp::move(other.B()));

    other.S()._data[0] = '\0';
    other.S()._flag = 0x80;
    
    return *this;
}

constexpr string_impl& operator=(const char* other){
    return __priv_assign(other, strlen(other));
}

SP_HOT SP_FORCEINLINE constexpr string_impl& operator+=(char c){ return __priv_push_back(c); }
SP_HOT SP_FORCEINLINE constexpr string_impl& operator+=(const char* other){ return __priv_append(other, strlen(other)); }
SP_HOT SP_FORCEINLINE constexpr string_impl& operator+=(const string_impl<_safety_level, Allocator>& other){ return __priv_append(other.c_str(), other.size()); }
SP_HOT SP_FORCEINLINE constexpr string_impl& operator+=(const string_impl<_other_safety_level, Allocator>& other){ return __priv_append(other.c_str(), other.size()); }

SP_HOT SP_FORCEINLINE constexpr string_impl operator*(size_type multiplier) const {
    string_impl result;
    size_type total_len = size() * multiplier;
    if (total_len <= 22){
        for (size_type i = 0; i < multiplier; ++i) result.__priv_append(c_str(), size());
    }else{
        result._inflate(total_len);
        for (size_type i = 0; i < multiplier; ++i) result.__priv_append(c_str(), size());
    }
    return result;
}


SP_HOT SP_FORCEINLINE constexpr string_impl& operator*=(size_type multiplier){
    size_type total_len = size() * multiplier;
    if(total_len <= 22){
        string_impl copy = C();
        clear();
        for(size_type i = 0; i < multiplier; ++i){
            __priv_append(copy.c_str(), copy.size());
        }
    }else{
        _inflate(total_len);
        string_impl copy = C();
        for(size_type i = 0; i < multiplier; ++i){
            __priv_append(copy.c_str(), copy.size());
        }
    }
    return *this;
}

SP_FORCEINLINE constexpr char& operator[](size_type index){
    return is_small() ? S()._data[index] : B()._data[index];
}

SP_FORCEINLINE constexpr const char& operator[](size_type index) const{
    return is_small() ? S()._data[index] : B()._data[index];
}

// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// ============================================== CONSTRUCTORS ==============================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================

constexpr string_impl() {
    S()._data[0] = '\0'; // Null terminator for small mode
    set_flag<true>(); // Start in small mode
    set_small_length(0);
}

template <typename T, typename = std::enable_if_t<spt::is_same_v<T, const char*>>>
constexpr string_impl(T str) : string_impl() {
    size_type len = strlen(str);
    if (len <= 22&&is_small()) {
        memcpy(S()._data, str, len);
        S()._data[len] = '\0';
        set_flag<true>();
        set_small_length(len);
    }else{
        _inflate(len);
        memcpy(B()._data, str, len);
        B()._size = len;
        B()._data[len] = '\0';
        set_flag<false>();
    }
}

SP_FORCEINLINE constexpr string_impl(const string_impl& other) : string_impl(){
    __priv_assign(other.c_str(), other.length());
}

SP_FORCEINLINE constexpr string_impl(string_impl&& other) : string_impl(){
    *this = sp::move(other);
}



constexpr string_impl(size_type count, char ch) : string_impl() {
    if(count <= 22){
        memset(S()._data, ch, count);
        S()._data[count] = '\0';
        set_flag<true>();
        set_small_length(count);
    }else{
        _inflate(count);
        memset(B()._data, ch, count);
        B()._size = count;
        B()._data[count] = '\0';
        set_flag<false>();
    }
}

// Constructor for string_impl literals: sp::string_impl s = "Hello";
template <size_t N>
SP_FORCEINLINE constexpr string_impl(const char (&str)[N]) : string_impl(){
    constexpr size_type len = N - 1;
    SP_IF_CONSTEXPR (len <= 22) {
        memcpy(S()._data, str, N);
        set_flag<true>();
        set_small_length(len);
    } else {
        _inflate(len);
        memcpy(B()._data, str, N);
        B()._size = len;
    }
}

// start and end idx
constexpr string_impl(const char* str, size_type len) : string_impl() {
    if (len <= 22) {
        memcpy(S()._data, str, len);
        S()._data[len] = '\0';
        set_flag<true>();
        set_small_length(len);
    }else{
        _inflate(len);
        memcpy(B()._data, str, len);
        B()._size = len;
        B()._data[len] = '\0';
        set_flag<false>();
    }
}

SP_CONSTEXPR20 ~string_impl() {
    if(is_big()) B().~__big_mode();
}


// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// =========================================== MUTATING FUNCTIONS ===========================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================


SP_FORCEINLINE constexpr char pop_back(){
    size_type len = size();
    size_type minus_one = len-1;
    char ret = '\0';
    if(len==0) return '\0';
    if(is_small()){
        ret = S()._data[minus_one];
        S()._data[minus_one] = '\0';
        set_small_length(minus_one);
    }else{
        ret = B()._data[minus_one];
        B()._data[minus_one] = '\0';
        B()._size = minus_one;
    }
    return ret;
}
SP_FORCEINLINE constexpr void clear() noexcept{
    if(is_big()) B().~__big_mode();
    S()._flag = 0;
    S()._data[0] = '\0';
    set_flag<true>();
}
SP_FORCEINLINE constexpr void clear_keep_capacity() noexcept{
    if(is_big()){
        B()._size = 0;
        B()._data[0] = '\0';
    }else{
        set_small_length(0);
        S()._data[0] = '\0';
    }
}
SP_COLD constexpr void shrink_to_fit(){
    SP_IF_EXPECT(is_big()) B().shrink_to_fit();
}
SP_HOT constexpr void reserve(size_type new_cap){
    size_type len = size();
    SP_IF_EXPECT(new_cap>size()&&new_cap>22){
        if(is_small()) _inflate(new_cap+1);
        else B().reserve(new_cap+1);
    }
}
SP_HOT constexpr void reserve_exact(size_type new_cap){
    size_type len = size();
    SP_IF_EXPECT(new_cap>size()&&new_cap>22){
        if(is_small()) _inflate(new_cap+1);
        else B().reserve_exact(new_cap+1);
    }
}
SP_HOT constexpr string_impl& trim_start(){
    size_type i = 0;
    while(i<size() && isspace(c_str()[i])) i++;
    if(i>0) erase(0, i);
    return *this;
}
SP_HOT constexpr string_impl& trim_end(){
    size_type i = size();
    while(i>0 && isspace(c_str()[i-1])) i--;
    if(i<size()) erase(i, size() - i);
    return *this;
}
SP_HOT constexpr string_impl& trim(){
    trim_end();
    trim_start();
    return *this;
}
SP_NODISCARD SP_HOT constexpr string_impl trimmed() const{
    string_impl temp(*this);
    temp.trim();
    return temp;
}
SP_HOT constexpr string_impl& pad_left(size_type total_width, char padding = ' '){
    // if total_width > length, shift by total_width - length
    size_type len = size();
    bool small = is_small();
    SP_IF_EXPECT(len<total_width){
        if(total_width<=22&&small){
            memmove(S()._data + (total_width - len), S()._data, len + 1); // shift right and include null terminator
            memset(S()._data, padding, total_width - len); // fill the left side with padding
            set_small_length(total_width);
        }else{
            if(small) _inflate(total_width);
            else if(total_width>capacity()) B().reserve(total_width);
            memmove(B()._data + (total_width - len), B()._data, len + 1); // shift right and include null terminator
            memset(B()._data, padding, total_width - len); // fill the left side with padding
            B()._size = total_width;
        }
    }
    return *this;
}
SP_HOT constexpr string_impl& pad_right(size_type total_width, char padding = ' '){
    size_type len = size();
    bool small = is_small();
    SP_IF_EXPECT(len<total_width){
        if(total_width<=22&&small){
            memset(S()._data+len, padding, total_width - len);
            S()._data[total_width] = '\0';
            set_small_length(total_width);
        }else{
            if(small) _inflate(total_width);
            else if(total_width>capacity()) B().reserve(total_width);
            memset(B()._data+len, padding, total_width - len);
            B()._data[total_width] = '\0';
            B()._size = total_width;
        }
    }
    return *this;
}
SP_HOT constexpr string_impl& to_lower(){
    char* dta = data();
    size_type sz = size();
    _SP_APPLY_UNROLLED_(sz, if((int)dta[i]>=65 && (int)dta[i]<=90) dta[i] += 32);
    return *this;
}
SP_HOT constexpr string_impl& to_upper(){
    char* dta = data();
    size_type sz = size();
    _SP_APPLY_UNROLLED_(sz, if((int)dta[i]>=97&&(int)dta[i]<=122) dta[i] -= 32);
    return *this;
}
SP_HOT constexpr string_impl& reverse(){
    char* dta = data();
    size_type sz = size();
    char temp = '\0';
    for(size_type i = 0, j = sz-1; i <= j; i++, j--){
        temp = dta[j];
        dta[j] = dta[i];
        dta[i] = temp;
    }
    return *this;
}
SP_NODISCARD SP_PURE constexpr size_type count(char c) const{
    const char* dta = data();
    size_type sz = size();
    size_type sum = 0;
    _SP_APPLY_UNROLLED_(sz, if(dta[i]==c) sum++);
    return sum;
}
SP_NODISCARD SP_PURE constexpr size_type find(char c, size_type pos = 0) const{
    const char* dta = data();
    size_type sz = size();
    _SP_APPLY_UNROLLED_(sz, SP_IF_NOT_EXPECT(i>=pos&&dta[i]==c) return i);
    return npos;
}
SP_NODISCARD SP_PURE constexpr size_type rfind(char c, size_type pos = npos) const{
    const char* dta = data();
    size_type sz = size();
    pos = min(pos, sz-1);
    for(size_type i = pos; i > 0; i--) SP_IF_NOT_EXPECT(dta[i]==c) return i;
    SP_IF_NOT_EXPECT(dta[0]==c) return 0;
    return npos;
}
SP_NODISCARD SP_PURE constexpr size_type find_first_of(const char set, size_type pos = 0) const{
    const char* dta = data();
    size_type sz = size();
    _SP_APPLY_UNROLLED_(sz, SP_IF_NOT_EXPECT(i>=pos&&dta[i]==set) return i);
    return npos;
}
SP_NODISCARD SP_PURE constexpr size_type find_last_of(const char set, size_type pos = npos) const{
    const char* dta = data();
    size_type sz = size();
    pos = min(pos, sz-1);
    for(size_type i = pos; i > 0; i--) SP_IF_NOT_EXPECT(dta[i]==set) return i;
    SP_IF_NOT_EXPECT(dta[0]==set) return 0;
    return npos;
}
SP_NODISCARD SP_PURE constexpr size_type find_first_not_of(const char set, size_type pos = 0) const{
    const char* dta = data();
    size_type sz = size();
    _SP_APPLY_UNROLLED_(sz, SP_IF_NOT_EXPECT(i>=pos&&dta[i]!=set) return i);
    return npos;
}
SP_NODISCARD SP_PURE constexpr bool starts_with(const char prefix, size_type pos = 0) const{
    const size_type sz = size();
    if(sz) return c_str()[0]==prefix;
    else return false;
}
SP_NODISCARD SP_PURE constexpr bool ends_with(const char suffix, size_type pos = npos) const{
    const size_type sz = size();
    if(sz) return c_str()[sz-1]==suffix;
    else return false;
}
SP_NODISCARD SP_PURE constexpr bool contains(const char target) const{
    const char* dta = data();
    size_type sz = size();
    _SP_APPLY_UNROLLED_(sz, SP_IF_NOT_EXPECT(dta[i]==target) return true);
    return false;
}
SP_NODISCARD SP_PURE constexpr bool starts_with(const char* prefix) const{
    const char* dta = data();
    size_type sz = size();
    size_type prefix_len = strlen(prefix);
    if(prefix_len>sz) return false;
    return memcmp(dta, prefix, prefix_len)==0;
}
SP_NODISCARD SP_PURE constexpr bool ends_with(const char* suffix) const{
    const char* dta = data();
    size_type sz = size();
    size_type suffix_len = strlen(suffix);
    if(suffix_len>sz) return false;
    return memcmp(dta+sz-suffix_len, suffix, suffix_len)==0;
}
SP_NODISCARD SP_PURE constexpr bool contains(const char* target) const{
    const char* dta = data();
    size_type sz = size();
    size_type target_len = strlen(target);
    if(target_len>sz) return false;
    _SP_APPLY_UNROLLED_(sz-target_len+1, SP_IF_NOT_EXPECT(memcmp(dta+i, target, target_len)==0) return true);
    return false;
}
SP_HOT constexpr string_impl& replace(size_type pos, size_type len, const char* str){
    return __priv_replace(pos, len, str, strlen(str));
}
SP_HOT constexpr string_impl& erase(size_type pos = 0, size_type len = npos){
    size_type sz = size();
    _SP_CHECK_SAFETY_LEVEL_(1) SP_IF_NOT_EXPECT(pos>sz) throw std::out_of_range("Position out of range in string_impl erase");
    if(pos+len>sz) len = sz - pos;
    SP_IF_NOT_EXPECT(len==0) return *this;
    if(is_small()){
        char temp[23]{};
        memcpy(temp, S()._data, pos); // copy up until the position
        memcpy(temp + pos, S()._data + pos + len, sz - pos - len); // copy the remaining data
        temp[sz - len] = '\0'; // new null terminator
        memcpy(S()._data, temp, sz - len + 1); // move the temp back into the small data(including null terminator)
        set_small_length(sz - len); // new length
    }else{
        memmove(B()._data + pos, B()._data + pos + len, sz - pos - len); // Shift existing data to the left
        B()._size = sz - len; // new size
        B()._data[sz - len] = '\0'; // null terminate at end
    }
    return *this;
}
SP_HOT constexpr string_impl& erase_all(char c){
    char* dta = data();
    size_type sz = size();
    size_type new_len = 0;
    for(size_type i = 0; i < sz; i++) if(dta[i]!=c) dta[new_len++] = dta[i];
    dta[new_len] = '\0';
    if(is_small()) set_small_length(new_len);
    else B()._size = new_len;
    return *this;
}
SP_HOT constexpr string_impl& squeeze(){
    char* dta = data();
    size_type sz = size();
    size_type new_len = 0;
    bool in_whitespace = false;
    for(size_type i = 0; i < sz; i++){
        if(isspace(dta[i])){
            if(!in_whitespace){
                dta[new_len++] = ' ';
                in_whitespace = true;
            }
        }else{
            dta[new_len++] = dta[i];
            in_whitespace = false;
        }
    }
    dta[new_len] = '\0';
    if(is_small()) set_small_length(new_len);
    else B()._size = new_len;
    return *this;
}
SP_FORCEINLINE constexpr void swap(string_impl& other) noexcept{
    SP_IF_NOT_EXPECT(this==&other) return;
    bool i_s = is_small();
    bool i_b = !i_s;
    bool o_s = other.is_small();
    bool o_b = !o_s;
    if(i_s&&o_s){
        char temp[23]{};
        memcpy(temp, S()._data, 24);
        memcpy(S()._data, other.S()._data, 24);
        memcpy(other.S()._data, temp, 24);
    }else if(i_b&&o_b){
        B().swap(other.B());
    }else if(i_s&&o_b){
        char temp[23]{};
        memcpy(temp, S()._data, 24);
        new(&B()) __big_mode(sp::move(other.B()));
        other.S()._data[0] = '\0';
        other.S()._flag = 0x80;
        memcpy(S()._data, temp, 24);
    }else{ // is_big()&&other.is_small()
        char temp[23]{};
        memcpy(temp, other.S()._data, 24);
        new(&other.B()) __big_mode(sp::move(B()));
        S()._data[0] = '\0';
        S()._flag = 0x80;
        memcpy(other.S()._data, temp, 24);
    }
}

SP_NODISCARD SP_HOT constexpr string_impl repeat(size_type times) const{
    string_impl result = *this;
    size_type sz = length();
    for(size_type i = 0; i < times-1; i++) result.__priv_append(c_str(),sz);
    return result;
}
SP_HOT constexpr string_impl& replace_all(const char target, const char replacement){
    char* dta = data();
    size_type sz = size();
    _SP_APPLY_UNROLLED_(sz, if(dta[i]==target)dta[i]=replacement);
    return *this;
}
SP_HOT constexpr string_impl& replace_all(const char* target, const char* replacement) {
    char* dta = data();
    const size_type sz = size();
    const size_type target_len = strlen(target);
    const size_type replacement_len = strlen(replacement);

    _SP_CHECK_SAFETY_LEVEL_(1) SP_IF_NOT_EXPECT(target_len == 0) return *this;

    if (target_len == replacement_len) {
        _SP_APPLY_UNROLLED_(sz - target_len + 1, 
            if (memcmp(dta + i, target, target_len) == 0) {
                memcpy(dta + i, replacement, replacement_len);
            }
        );
        return *this; 
    } else {
        size_type occurrences = 0;
        for (size_type i = 0; i <= sz - target_len; ) {
            if (memcmp(dta + i, target, target_len) == 0) {
                occurrences++;
                i += target_len;
            } else {
                i++;
            }
        }
        SP_IF_NOT_EXPECT(occurrences == 0) return *this;
        string_impl result;
        const size_type final_size = sz + (occurrences * (replacement_len - target_len));
        result.reserve_exact(final_size);

        for (size_type i = 0; i < sz; ) {
            if (i <= sz - target_len && memcmp(dta + i, target, target_len) == 0) {
                result.__priv_append(replacement, replacement_len);
                i += target_len;
            } else {
                size_type block_start = i;
                while (i < sz && !(i <= sz - target_len && memcmp(dta + i, target, target_len) == 0)) {
                    i++;
                }
                result.__priv_append(dta + block_start, i - block_start);
            }
        }
        *this = sp::move(result);
        return *this;
    }
}


// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// ================================================== DAY 2 =================================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================== SLICING, SPLITTING & FORMATTING ======================================
// =================================================== // ===================================================
// =================================================== // ===================================================
// =================================================== // ===================================================


SP_NODISCARD SP_HOT SP_FORCEINLINE constexpr string_impl slice(int start, int end) const{ 
    _SP_CHECK_SAFETY_LEVEL_(1) SP_IF_NOT_EXPECT(start>size()) throw std::out_of_range("Start index out of range in string_impl slice");
    _SP_CHECK_SAFETY_LEVEL_(1) SP_IF_NOT_EXPECT(end>size()) throw std::out_of_range("End index out of range in string_impl slice");
    while(start<0) start = size() + start;
    while(end<0) end = size() + end;
    return string_impl(c_str()+start, end-start); 
}
SP_NODISCARD SP_HOT constexpr string_impl substr(size_type pos, size_type len) const { 
    _SP_CHECK_SAFETY_LEVEL_(1) SP_IF_NOT_EXPECT(pos>size()) throw std::out_of_range("Position out of range in string_impl substr");
    _SP_CHECK_SAFETY_LEVEL_(1) SP_IF_NOT_EXPECT(pos+len>size()) throw std::out_of_range("Length out of range in string_impl substr");
    return string_impl(c_str()+pos, len); 
}
template <typename T, bool remove_empty=false>
SP_NODISCARD SP_HOT constexpr T split(char delimiter) const{
    const char* ptr = c_str();
    const size_type sz = size();
    size_type pos = 0;
    size_type next = 0;
    T result;
    while(next<sz){
        if(ptr[next]==delimiter){
            SP_IF_CONSTEXPR(remove_empty){
                size_type str_len = next-pos;
                if(str_len!=0) result.template push_back<1>(string_impl(ptr+pos, str_len));
            }else{ result.template push_back<1>(string_impl(ptr+pos, next-pos)); }
            pos = next+1;
        }
        next++;
    }
    SP_IF_CONSTEXPR(remove_empty){
        size_type str_len = next-pos;
        if(str_len!=0) result.template push_back<1>(string_impl(ptr+pos, str_len));
    }else{ result.template push_back<1>(string_impl(ptr+pos, next-pos)); }
    return result;
}
template <typename T, bool remove_empty=false>
SP_NODISCARD SP_HOT constexpr T split(const char* delimiter) const{
    const char* ptr = c_str();
    const size_type sz = size();
    const size_type delim_sz = strlen(delimiter);
    size_type pos = 0;
    size_type next = 0;
    T result;
    while(next<sz){
        if(memcmp(ptr+next, delimiter, delim_sz)==0){
            SP_IF_CONSTEXPR(remove_empty){
                size_type str_len = next-pos;
                if(str_len!=0) result.template push_back<1>(string_impl(ptr+pos, str_len));
            }else { result.template push_back<1>(string_impl(ptr+pos, next-pos)); }
            pos = next+delim_sz;
        }
        next++;
    }
    SP_IF_CONSTEXPR(remove_empty){
        size_type str_len = next-pos;
        if(str_len!=0) result.template push_back<1>(string_impl(ptr+pos, str_len));
    }else { result.template push_back<1>(string_impl(ptr+pos, next-pos)); }
    return result;
}
template <typename T>
SP_NODISCARD SP_HOT static constexpr string_impl join(const T& parts, const char* sep){
    string_impl result;
    if (parts.is_empty()) return result;
    size_type total_len = 0;
    for(size_type i = 0; i < parts.size(); i++) total_len += parts[i].size();
    total_len += (parts.size() - 1) * strlen(sep);
    result.reserve(total_len + 1);
    for(size_type i = 0; i < parts.size(); i++) {
        result += parts[i];
        SP_IF_EXPECT(i != parts.size() - 1) result += sep;
    }
    return result;
}
template <typename T>
SP_NODISCARD SP_HOT constexpr string_impl join(const T& parts) const {
    string_impl result;
    if (parts.is_empty()) return result;
    size_type total_len = 0;
    for(size_type i = 0; i < parts.size(); i++) total_len += parts[i].size();
    total_len += (parts.size() - 1) * this->size();
    result.reserve(total_len + 1);
    for(size_type i = 0; i < parts.size(); i++) {
        result += parts[i];
        SP_IF_EXPECT(i != parts.size() - 1) result += *this;
    }
    return result;
}
SP_HOT constexpr string_impl& insert(size_type pos, char c){ return __priv_insert(pos, &c, 1); }
SP_HOT constexpr string_impl& insert(size_type pos, size_type count, char c){
    string_impl temp(count, c);
    return __priv_insert(pos, temp.c_str(), count);
}
SP_HOT constexpr bool is_numeric() const{
    const char* dta = data();
    size_type sz = size();
    SP_IF_NOT_EXPECT(sz==0) return false;
    size_type i = 0;
    if(dta[0]=='-'||dta[0]=='+') i = 1; // optional sign
    bool decimal_point_seen = false;
    for(; i < sz; i++){
        SP_IF_NOT_EXPECT(dta[i]=='.'){
            if(decimal_point_seen) return false; // multiple decimal points not allowed
            decimal_point_seen = true;
        }else SP_IF_NOT_EXPECT(dta[i]<'0'||dta[i]>'9') return false; // non-digit character
    }
    return true;
}
SP_HOT constexpr bool is_alpha() const{
    const char* dta = data();
    size_type sz = size();
    SP_IF_NOT_EXPECT(sz==0) return false;
    for(size_type i = 0; i < sz; i++){
        SP_IF_NOT_EXPECT((dta[i]<'A'||dta[i]>'Z')&&(dta[i]<'a'||dta[i]>'z')) return false;
    }
    return true;
}
SP_HOT constexpr bool is_alnum() const{
    const char* dta = data();
    size_type sz = size();
    SP_IF_NOT_EXPECT(sz==0) return false;
    for(size_type i = 0; i < sz; i++){
        SP_IF_NOT_EXPECT((dta[i]<'A'||dta[i]>'Z')&&(dta[i]<'a'||dta[i]>'z')&&(dta[i]<'0'||dta[i]>'9')) return false;
    }
    return true;
}
SP_HOT constexpr bool is_whitespace() const{
    const char* dta = data();
    size_type sz = size();
    SP_IF_NOT_EXPECT(sz==0) return false;
    for(size_type i = 0; i < sz; i++){
        SP_IF_NOT_EXPECT(dta[i]!=' '&&dta[i]!='\t'&&dta[i]!='\n'&&dta[i]!='\r') return false;
    }
    return true;
}
SP_HOT constexpr bool is_hex() const{
    const char* dta = data();
    size_type sz = size();
    SP_IF_NOT_EXPECT(sz==0) return false;
    for(size_type i = 0; i < sz; i++){
        SP_IF_NOT_EXPECT((dta[i]<'0'||dta[i]>'9')&&(dta[i]<'A'||dta[i]>'F')&&(dta[i]<'a'||dta[i]>'f')) return false;
    }
    return true;
}
SP_COLD constexpr string_impl& wrap(size_type width){
    string_impl result;
    const char* dta = data();
    const size_type sz = size();
    size_type pos = 0;
    while(pos<sz){
        const size_type line_len = min(width, sz - pos);
        result += string_impl(dta + pos, line_len);
        result += '\n';
        pos += line_len;
    }
    *this = sp::move(result);
    return *this;
}
SP_HOT constexpr string_impl& capitalize(){
    SP_IF_EXPECT(size()){
        char* dta = data();
        const size_type sz = size();
        if((int)dta[0]>=97&&(int)dta[0]<=122) dta[0] -= 32;
        _SP_EXPLICIT_UNROLLED_(i, 1, sz, if((int)dta[i]>=65&&(int)dta[i]<=90) dta[i] += 32);
    }
    return *this;
}
SP_HOT constexpr string_impl& title_case(){
    SP_IF_EXPECT(size()){
        char* dta = data();
        const size_type sz = size();
        if((int)dta[0]>=97&&(int)dta[0]<=122) dta[0] -= 32;
        for(size_type i = 1; i < sz; i++){
            if(isspace(dta[i-1])){
                if((int)dta[i]>=97&&(int)dta[i]<=122) dta[i] -= 32; 
            } else if((int)dta[i]>=65&&(int)dta[i]<=90) dta[i] += 32;
            else if(dta[i]=='_') dta[i] = ' ';
        }
    }
    return *this;
}
SP_HOT constexpr string_impl& camel_case(){
    SP_IF_EXPECT(size()){ // sample: Input: "smart_p_string_impl" → Output: "smartPstring_impl"
        char* dta = data();
        const size_type sz = size();
        bool to_upper = false;
        size_type shifts = 0;
        for(size_type i = 0; i < sz; i++){
            if(dta[i]=='_'||isspace(dta[i])){
                to_upper = true;
                memmove(dta + i, dta + i + 1, sz - i); // shift left to remove the underscore/space
                i--; // stay at the same index for the next iteration since we just shifted
                shifts++;
            }else if(to_upper){
                if((int)dta[i]>=97&&(int)dta[i]<=122) dta[i] -= 32; // convert to uppercase
                to_upper = false;
            }else{
                if((int)dta[i]>=65&&(int)dta[i]<=90) dta[i] += 32; // convert to lowercase
            }
        }
        if(is_small()) set_small_length(sz - shifts);
        else B()._size = sz - shifts;
    }
    return *this;
}
SP_HOT constexpr string_impl& snake_case(){
    const size_type sz = size();
    SP_IF_EXPECT(sz){
        char* dta = data();
        for(size_type i = 0; i < sz; i++){
            if((int)dta[i]>=65&&(int)dta[i]<=90) dta[i] += 32;
            else if(isspace(dta[i])) dta[i] = '_';
        }
    }
    return *this;
}
SP_HOT constexpr string_impl& kebab_case(){
    const size_type sz = size();
    SP_IF_EXPECT(sz){
        char* dta = data();
        for(size_type i = 0; i < sz; i++){
            if((int)dta[i]>=65&&(int)dta[i]<=90) dta[i] += 32;
            else if(isspace(dta[i])) dta[i] = '-';
        }
    }
    return *this;
}
SP_HOT constexpr int compare(string_impl& other) const{
    const char* dta = data();
    const size_type sz = size();
    const char* other_dta = other.data();
    const size_type other_sz = other.size();
    size_type min_len = min(sz, other_sz);
    int cmp = memcmp(dta, other_dta, min_len);
    if(cmp!=0) return cmp;
    else if(sz<other_sz) return -1;
    else if(sz>other_sz) return 1;
    return 0;
}
SP_HOT constexpr int compare_case_insensitive(const string_impl& other) const{
    const char* dta = data();
    const size_type sz = size();
    const char* other_dta = other.data();
    const size_type other_sz = other.size();
    size_type min_len = min(sz, other_sz);
    for(size_type i = 0; i < min_len; i++){
        char c1 = dta[i];
        char c2 = other_dta[i];
        if(c1>=65&&c1<=90) c1 += 32;
        if(c2>=65&&c2<=90) c2 += 32;
        if(c1!=c2) return (int)c1 - (int)c2;
    }
    if(sz<other_sz) return -1;
    else if(sz>other_sz) return 1;
    return 0;
}
SP_HOT constexpr string_impl& fill(char c){
    char* dta = data();
    size_type sz = size();
    memset(dta, c, sz);
    return *this;
}
SP_HOT constexpr void resize(size_type n, char c = '\0'){
    size_type sz = size();
    if(n<sz){
        if(is_small()){
            S()._data[n] = '\0';
            set_small_length(n);
        }else{
            B()._size = n;
            B()._data[n] = '\0';
        }
    }else if(n>sz){
        reserve(n);
        char* dta = data();
        memset(dta + sz, c, n - sz);
        dta[n] = '\0';
        if(is_small()) set_small_length(n);
        else B()._size = n;
    }
}

SP_HOT SP_FORCEINLINE constexpr string_impl& append(const char* other){ return __priv_append(other, strlen(other)); }
SP_HOT SP_FORCEINLINE constexpr string_impl& append(const string_impl& other){ return __priv_append(other.c_str(), other.size()); }
SP_HOT SP_FORCEINLINE constexpr string_impl& append(char other){ return __priv_append(&other, 1); }
SP_HOT SP_FORCEINLINE constexpr string_impl& append(const char* other, size_type size){ return __priv_append(other, size);}
SP_HOT SP_FORCEINLINE constexpr string_impl& append(const string_impl<_other_safety_level,Allocator>& other){ return __priv_append(other.c_str(), other.size()); }

SP_HOT SP_FORCEINLINE constexpr string_impl& assign(const char* other){ return __priv_assign(other, strlen(other)); }
SP_HOT SP_FORCEINLINE constexpr string_impl& assign(const string_impl& other){ return __priv_assign(other.c_str(), other.size()); }
SP_HOT SP_FORCEINLINE constexpr string_impl& assign(char other){ return __priv_assign(&other, 1); }
SP_HOT SP_FORCEINLINE constexpr string_impl& assign(const char* other, size_type size){ return __priv_assign(other, size);}
SP_HOT SP_FORCEINLINE constexpr string_impl& assign(const string_impl<_other_safety_level,Allocator>& other){ return __priv_assign(other.c_str(), other.size()); }

SP_NODISCARD SP_FORCEINLINE constexpr char& at(size_type index){
    _SP_CHECK_SAFETY_LEVEL_(1) SP_IF_NOT_EXPECT(index>=size()) throw std::out_of_range("Index out of range in string_impl at");
    return is_small() ? S()._data[index] : B()._data[index];
}
SP_NODISCARD SP_FORCEINLINE constexpr const char& at(size_type index) const{
    _SP_CHECK_SAFETY_LEVEL_(1) SP_IF_NOT_EXPECT(index>=size()) throw std::out_of_range("Index out of range in string_impl at");
    return is_small() ? S()._data[index] : B()._data[index];
}

SP_HOT SP_FORCEINLINE constexpr const char& back() const { if(is_big()) return B()._data[B()._size-1]; else return S()._data[(S()._flag & 0x1F) - 1]; }
SP_HOT SP_FORCEINLINE constexpr char& back() { if(is_big()) return B()._data[B()._size-1]; else return S()._data[(S()._flag & 0x1F) - 1]; }
SP_HOT SP_FORCEINLINE constexpr size_type copy(char* dest, size_type count, size_type pos = 0 ) const{
    _SP_CHECK_SAFETY_LEVEL_(1) SP_IF_NOT_EXPECT(pos>size()) throw std::out_of_range("Position out of range in string_impl copy");
    if(pos+count>size()) count = size() - pos;
    memcpy(dest, c_str()+pos, count);
    return count;
}
SP_HOT SP_FORCEINLINE constexpr const char& front() const{ return c_str()[0]; }
SP_HOT SP_FORCEINLINE constexpr char& front() { return data()[0]; }
SP_HOT SP_FORCEINLINE constexpr size_type max_size() const { return sp::npos; }

SP_HOT SP_FORCEINLINE constexpr string_impl& push_back(char element) { return __priv_push_back(element); }
SP_HOT SP_FORCEINLINE constexpr string_impl& refresh_size(){
    // for when the data is modified directly
    if(is_big()){
        B()._size = strlen(B()._data);
    }else{
        set_small_length(strlen(S()._data));
    }
    return *this;
}
};
template <short level>
SP_HOT SP_FORCEINLINE constexpr string_impl<level> operator*(size_type multiplier, const string_impl<level>& str){
    return str * multiplier;
}

template <short level, template <typename> class alloc, bool use_sso>
SP_FORCEINLINE constexpr bool operator==(const char* other,const string_impl<level, alloc, use_sso>& str){
    return str==other;
}

class string_view{
public:
    constexpr string_view(const char* cstr, size_type sz) : data(cstr), size(sz), max_size(sz){}
    constexpr string_view() : data(nullptr), size(0), max_size(0){}
    const char* data;
    size_type size;
    size_type max_size;
    constexpr string_view& move_to_front(){
        data -= (max_size - size);
        size = max_size;
        return *this;
    }
    constexpr string_view& move_to_back(){
        data += (size-1);
        size = 1;
        return *this;
    }
    constexpr string_view& operator++(){
        if(size>1) { data++; size--; }
        return *this;
    }
    constexpr string_view& operator--(){
        if(size<max_size) { data--; size++; }
        return *this;
    }
    constexpr string_view operator++(int) {
        string_view temp = *this;
        ++(*this);
        return temp;
    }
    constexpr string_view operator--(int) {
        string_view temp = *this;
        --(*this);
        return temp;
    }
    constexpr string_view& advance_window(){
        ++data; ++max_size;
        return *this;
    }
};

inline constexpr bool operator==(const string_view& lhs, const string_view& rhs) {
    if (lhs.size != rhs.size) {
        return false;
    }
    return std::memcmp(lhs.data, rhs.data, lhs.size) == 0;
}

template <short safety>
inline constexpr bool operator==(const string_view& sv, const sp::string_impl<safety>& str) {
    if (sv.size != str.size()) {
        return false;
    }
    return std::memcmp(sv.data, str.c_str(), sv.size) == 0;
}

template <short safety>
inline constexpr bool operator==(const sp::string_impl<safety>& str, const string_view& sv) {
    return sv == str;
}


class compressed_string_view{
public:
    constexpr compressed_string_view(const char* cstr, size_type sz) : _data(cstr), _size(sz){}
    constexpr compressed_string_view() : _data(nullptr), _size(0){}
    private:
    const char* _data;
    size_type _size;
    public:
    constexpr compressed_string_view& operator++(){
        if(_size>1) { _data++; _size--; }
        return *this;
    }
    constexpr compressed_string_view& operator--(){
        --_data; ++_size;
        return *this;
    }
    constexpr compressed_string_view operator++(int) {
        compressed_string_view temp = *this;
        ++(*this);
        return temp;
    }
    constexpr compressed_string_view operator--(int) {
        compressed_string_view temp = *this;
        --(*this);
        return temp;
    }
    constexpr compressed_string_view& advance_window(){
        ++_data;
        return *this;
    }
    SP_FORCEINLINE constexpr size_type size() const { return _size; }
    SP_FORCEINLINE constexpr const char* c_str() const { return _data; }
    SP_FORCEINLINE constexpr const char* data() const { return _data; }
};

inline constexpr bool operator==(const compressed_string_view& lhs, const compressed_string_view& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    return std::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
}

template <short safety>
inline constexpr bool operator==(const compressed_string_view& sv, const sp::string_impl<safety>& str) {
    if (sv.size() != str.size()) {
        return false;
    }
    return std::memcmp(sv.data(), str.c_str(), sv.size()) == 0;
}

template <short safety>
inline constexpr bool operator==(const sp::string_impl<safety>& str, const compressed_string_view& sv) {
    return sv == str;
}

// // operator<<
// template <short level>
// std::ostream& operator<<(std::ostream& os, const string_impl<level>& str){
//     return os << str.c_str();
// }
// // operator>>
// template <short level>
// std::istream& operator>>(std::istream& is, string_impl<level>& str){
//     char buffer[1024];
//     is >> buffer;
//     str = buffer;
//     return is;
// }

using string = string_impl<1>;
using ustring = string_impl<0>;
} // namespace sp
SP_FORCEINLINE SP_CONSTEXPR20 sp::string_impl<__SP_DEFAULT_SAFETY_LEVEL__,sp::allocator,true> operator""_sp(const char* str, size_type N){
    return sp::string_impl<__SP_DEFAULT_SAFETY_LEVEL__,sp::allocator,true>(str, N);
}
#endif // ____SP_STRING____