#ifndef ____SP_HBA____
#define ____SP_HBA____
// hirearchial bitmask array
#pragma once
#include "../setup/init.hpp"
#include "../io/IO.hpp"
#include "../core/allocators.hpp"
#include "../core/type_traits.hpp"
#include "../math/bit_manip.hpp"
#include "../math/algorithm.hpp"
#include <initializer_list>
#include <cstring>
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
#endif
namespace sp{
#ifdef __SP_HBA_NUM_LAYERS__
    template <typename T, ull __num_layers=__SP_HBA_NUM_LAYERS__, template <typename> typename Alloc = sp::allocator>
#else
    template <typename T, ull __num_layers=1, template <typename> typename Alloc = sp::allocator>
#endif
class hba{
private:
static constexpr ull __max_layers = 10;
static constexpr ull __min_layers = 1;
static constexpr ull _num_layers = (__num_layers<__min_layers ? __min_layers : (__num_layers>__max_layers ? __max_layers : __num_layers));
ull* _meta = nullptr;
T* _data = nullptr;
size_type _size = 0;
size_type _capacity = 0;
bool _is_contiguous = true;
SP_NO_UNIQUE_ADDRESS Alloc<T> _alloc;
SP_NO_UNIQUE_ADDRESS Alloc<ull> _meta_alloc;

_SP_GRANT_IO_ACCESS_
using is_always_equal = spt::true_type;
using type_param = spt::conditional_t<spt::is_trivially_copyable_v<T> && sizeof(T) <= 16, T, const T&>;
static constexpr bool _trivially_copyable = spt::is_trivially_copyable_v<T>;

template <typename _Alloc, typename = void> struct allocator_ext {
    static constexpr ull true_capacity(ull n) noexcept { return n; }
};
template <typename _Alloc> struct allocator_ext<_Alloc, spt::void_t<decltype(_Alloc::capacity_for(spt::declval<ull>()))>> {
    static constexpr ull true_capacity(ull n) noexcept { return _Alloc::capacity_for(n); }
};

static constexpr SP_FORCEINLINE SP_PURE ull _calculate_meta_size(size_type cap){
    ull total_words = 0;
    ull current_layer_blocks = cap;
    for(int i = 0; i < _num_layers; ++i){
        current_layer_blocks = (current_layer_blocks + 63) >> 6;
        total_words += current_layer_blocks;
    }
    return total_words;
}

static constexpr ull _layer_size(ull layer, ull cap) {
    ull blocks = cap;
    for(ull i = 0; i < layer; ++i) blocks = (blocks + 63) >> 6;
    return blocks;
}

static constexpr ull _layer_offset(ull target_layer, ull cap) {
    ull offset = 0;
    for(ull i = _num_layers; i > target_layer; --i) offset += _layer_size(i, cap);
    return offset;
}

SP_FORCEINLINE void _disable_slot(size_type idx){
    size_type layer1_block = _layer_offset(1, _capacity) + (idx >> 6);
    _meta[layer1_block] &= ~(1ULL << (63 - (idx & 63)));
}

SP_FORCEINLINE void _enable_slot(size_type idx){
    size_type layer1_block = _layer_offset(1, _capacity) + (idx >> 6);
    _meta[layer1_block] |= (1ULL << (63 - (idx & 63)));
}

SP_FORCEINLINE void _propagate_up(size_type idx, int _change_val) {
    size_type block_idx = idx >> 12; // Start at layer 2 parent block (64 * 64 = 4096)
    for (ull layer = 2; layer <= _num_layers; ++layer){
        size_type meta_idx = _layer_offset(layer, _capacity) + block_idx;
        _meta[meta_idx] += _change_val;
        block_idx >>= 6;
    }
}

SP_NODISCARD SP_FORCEINLINE bool _is_slot_active(size_type physical_idx) const noexcept{
    size_type mask_idx = physical_idx >> 6; 
    size_type bit_idx = physical_idx & 63;   
    return (_meta[_layer_offset(1, _capacity) + mask_idx] & (1ULL << (63 - bit_idx))) != 0;
}

SP_NODISCARD SP_FORCEINLINE ull grow_capacity(ull size){ return sp::max((size_type)64, next_pow2(size)); }
 
template <ull CurrentLayer>
SP_FORCEINLINE SP_HOT void _descend_layers(size_type& remaining, size_type& hole_offset, size_type& block_offset) const noexcept {
    SP_IF_CONSTEXPR(CurrentLayer >= 2){
        size_type probe_idx = _layer_offset(CurrentLayer, _capacity) + block_offset;
        static constexpr ull multiplied = 6 * CurrentLayer;
        while(_meta[probe_idx] < remaining){
            remaining -= _meta[probe_idx];
            hole_offset += (1ULL << multiplied) - _meta[probe_idx++];
            block_offset++;
        }
        block_offset <<= 6;
        _descend_layers<CurrentLayer - 1>(remaining, hole_offset, block_offset);
    }
}


// O(L*log_(64^L) N) -> O(log_64 N)
SP_NODISCARD SP_FORCEINLINE SP_HOT const size_type get_idx(size_type target_idx) const {
    size_type block_offset = 0;
    size_type hole_offset = 0;
    size_type remaining = target_idx;
    _descend_layers<_num_layers>(remaining, hole_offset, block_offset);
    size_type probe_idx = _layer_offset(1, _capacity) + block_offset;
    size_type cur = popcount(_meta[probe_idx]);
    while (cur < remaining) {
        remaining -= cur;
        hole_offset += (64 - cur);
        cur = popcount(_meta[++probe_idx]);
    }
    ull meta_val = _meta[probe_idx];
    // Initial binary split chosen empirically.
    // 32 consistently provides the best overall performance across
    // lookup-heavy and erase-heavy benchmarks. Smaller splits (e.g. 16)
    // significantly regress erase throughput despite similar lookup cost.
    // why the 32-bit split proves to be significantly faster than a deeper binary
    // search or a pure while-loop is still unknown.
    // It's suspected that 32-bit is quickest because it provides a single predictable branch
    // instead of branch mispredictions with deeper binary searches.
    int cnt_lo = popcount(meta_val >> 32);
    if(remaining>=cnt_lo){
        remaining -= cnt_lo;
        hole_offset += (32 - cnt_lo);
        meta_val <<= 32;
    }
    ull next_set_bit = leading_zeros(meta_val);
    while(remaining > 0){
        hole_offset += next_set_bit;
        meta_val = (meta_val << next_set_bit) << 1;
        next_set_bit = leading_zeros(meta_val);
        --remaining;
    }
    if(meta_val) hole_offset += next_set_bit;
    return target_idx + hole_offset;
}

SP_FORCEINLINE void build_meta(){
    memset(_meta, 0, _calculate_meta_size(_capacity) * sizeof(ull));
    ull remaining = _size;
    ull idx = _layer_offset(1, _capacity);
    while(remaining>=64){
        _meta[idx++] = ~0ULL;
        remaining -= 64;
    }if(remaining) _meta[idx] = ~0ULL << (64 - remaining);

    SP_IF_CONSTEXPR(_num_layers>1){
        ull child_start = _layer_offset(1, _capacity); ull parent_start = _layer_offset(2, _capacity);
        ull child_count = _layer_size(1, _capacity); ull parent_count = _layer_size(2, _capacity);
        for(ull p = 0; p < parent_count; ++p){
            ull sum = 0; ull child_base = p << 6;
            for(ull c = 0; c < 64; ++c){
                ull child_idx = child_base + c;
                SP_IF_NOT_EXPECT(child_idx>=child_count) break;
                sum += popcount(_meta[child_start + child_idx]);
            }
            _meta[parent_start + p] = sum;
        }
    }

    for(ull layer = 2; layer < _num_layers; ++layer){
        ull child_start = _layer_offset(layer, _capacity);
        ull child_count = _layer_size(layer, _capacity);
        
        ull parent_start = _layer_offset(layer + 1, _capacity);
        ull parent_count = _layer_size(layer + 1, _capacity);

        for(ull p = 0; p < parent_count; p++) { // parent word
            ull sum = 0;
            ull child_base = p << 6; 
            for(ull c = 0; c < 64; c++) { // each parent word sums up to 64 child words
                ull child_idx = child_base + c;
                SP_IF_NOT_EXPECT(child_idx >= child_count) break;
                sum += _meta[child_start + child_idx];
            }
            _meta[parent_start + p] = sum;
        }
    }
}
//============================//============================//============================//============================
//============================//============================//============================//============================
//============================//============================//============================//============================
public:
//============================//============================//============================//============================
//============================//============================//============================//============================
//============================//============================//============================//============================
#define _SP_INIT_CDM_TS_ \
_capacity = allocator_ext<Alloc<T>>::true_capacity(target_size); \
_data = sp::allocator_traits<Alloc<T>>::allocate(_alloc,_capacity); \
_meta = sp::allocator_traits<Alloc<ull>>::allocate(_meta_alloc, _calculate_meta_size(_capacity));

size_type idx(size_type t) { return get_idx(t); }
SP_FORCEINLINE hba() : _data(nullptr), _meta(nullptr), _size(0), _capacity(allocator_ext<Alloc<T>>::true_capacity(0)), _is_contiguous(true){}
SP_FLATTEN hba(size_type size) : hba(size, T(0)){}
hba(size_type size, type_param val){
    size_type target_size = next_pow2(grow_capacity(size));
    _SP_INIT_CDM_TS_
    _SP_APPLY_UNROLLED_(size, {
        sp::allocator_traits<Alloc<T>>::construct(_alloc, _data+_size,val);
        _size++;
    });
    build_meta();
}
hba(std::initializer_list<T> list){
    size_type target_size = next_pow2(grow_capacity(list.size()));
    _SP_INIT_CDM_TS_
    for(type_param i : list){
        sp::allocator_traits<Alloc<T>>::construct(_alloc, _data+_size,i);
        _size++;
    }
    build_meta();
}
#undef _SP_INIT_CDM_TS

SP_FORCEINLINE hba(const hba& other) : _size(other._size), _capacity(other._capacity), _is_contiguous(other._is_contiguous){
    const size_type sz = _calculate_meta_size(_capacity);
    _data = sp::allocator_traits<Alloc<T>>::allocate(_alloc,_capacity);
    _meta = sp::allocator_traits<Alloc<ull>>::allocate(_meta_alloc,sz);
    
    SP_IF_CONSTEXPR(spt::is_trivially_copyable_v<T>){
        if(_is_contiguous) memcpy(_data, other._data, _size*sizeof(T));
        else{
            _SP_APPLY_UNROLLED_(_capacity, {if(other._is_slot_active(i)) _data[i] = other._data[i];});
        }
        memcpy(_meta, other._meta, sz*sizeof(ull));
    }else{
        _SP_APPLY_UNROLLED_(_capacity, {
            if(other._is_slot_active(i)) {
                sp::allocator_traits<Alloc<T>>::construct(_alloc, _data + i, other._data[i]);
            }
        });
        _SP_APPLY_UNROLLED_(sz, _meta[i] = other._meta[i]);
    }
}
SP_FORCEINLINE hba(hba&& other) : _size(other._size), _capacity(other._capacity), 
_is_contiguous(other._is_contiguous),_data(sp::move(other._data)),_meta(sp::move(other._meta)),
_alloc(sp::move(other._alloc)),_meta_alloc(sp::move(other._meta_alloc)){
    other._size = 0; other._capacity = 0; other._is_contiguous = true;
    other._data = nullptr; other._meta = nullptr;
}

~hba(){ // TEMPORARY: inefficient
    if(_data){
        for(size_type i = 0; i < _capacity; ++i){
            if(_is_slot_active(i)) sp::allocator_traits<Alloc<T>>::destroy(_alloc, _data + i);
        }
        sp::allocator_traits<Alloc<T>>::deallocate(_alloc, _data, _capacity);
    }
    if(_meta) sp::allocator_traits<Alloc<ull>>::deallocate(_meta_alloc, _meta, _calculate_meta_size(_capacity));
}

//============================//============================//============================//============================
//============================//============================//============================//============================
//============================//============================//============================//============================

SP_FORCEINLINE const T& operator[](size_type target_idx) const { return (_is_contiguous ? _data[target_idx] : _data[get_idx(target_idx)]); }
SP_FORCEINLINE T& operator[](size_type target_idx) { return (_is_contiguous ? _data[target_idx] : _data[get_idx(target_idx)]); }
SP_FORCEINLINE const T* data() const { return _data; }
SP_FORCEINLINE const ull* get_meta() const { return _meta; }
SP_FORCEINLINE ull get_meta_size() const { return _calculate_meta_size(_capacity); }
SP_FORCEINLINE size_type size() const { return _size; }
SP_FORCEINLINE size_type capacity() const { return _capacity; }
SP_FORCEINLINE void set_contig(bool condition) { _is_contiguous = condition; }
SP_FORCEINLINE bool empty() { return _size==0; }
SP_FORCEINLINE bool is_empty() { return _size==0; }

// Compress: Two-pointer (read pointer and write pointer), O(N) Time, O(1) Space
SP_FORCEINLINE hba& compress(){
    size_type read_ptr = 0;
    size_type write_ptr = 0;
    while(read_ptr<_capacity){
        if(_is_slot_active(read_ptr)){
            SP_IF_CONSTEXPR(spt::is_trivially_copyable_v<T>) _data[write_ptr++] = _data[read_ptr];
            else _data[write_ptr++] = sp::move(_data[read_ptr]);
        }
        ++read_ptr;
    }
    build_meta();
    _is_contiguous = true;
    return *this;
}

SP_FORCEINLINE hba& erase(size_type target_idx){
    const size_type idx = get_idx(target_idx);
    sp::allocator_traits<Alloc<T>>::destroy(_alloc, _data+idx);
    _disable_slot(idx);
    _propagate_up(idx, -1);
    _is_contiguous = false;
    _size--;
    return *this;
}

SP_FORCEINLINE hba& insert(size_type target_idx, const T& val) {
    const size_type idx = get_idx(target_idx);
    T item_to_place = val; 
    size_type hole_idx = idx;
    while (hole_idx < _capacity - 1 && _is_slot_active(hole_idx)) {
        sp::swap(_data[hole_idx], item_to_place);
        hole_idx++;
    }
    sp::allocator_traits<Alloc<T>>::construct(_alloc, _data + hole_idx, sp::move(item_to_place));
    _enable_slot(hole_idx);
    _propagate_up(hole_idx, 1);
    _size++;

    return *this;
}

SP_FORCEINLINE void print(){
    std::cout << "[";
    for(size_type i = 0; i < _size; i++){
        std::cout << (*this)[i];
        if(i != _size - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
}
};
} // namespace sp
#endif