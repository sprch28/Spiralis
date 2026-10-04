#ifndef ____SP_HBA____
#define ____SP_HBA____

/*  -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
                  Hierarchical  bitmask array

When treated like a black box, the hba appears to behave identically to a dynamic array (such as std::vector).
However, the difference is present in the implementation.
Unlike a traditional array, calling erase(idx) doesn't shift all the data down to fill the hole.
erase() punctures holes that are tracked by a hierarchical layout:
    - Layer 1: 64-bit bitmasks: 0 represents a hole, 1 represents an occupied slot.
    MSB-order for prototyping convenience: bit 63(leftmost bit) represents index 0.
    Example: [1, 2, 3, 4, 5] would be represented at layer 1 by: 11111000000000.....

    - Above layer 1: Instead of bitmasks, these represent prefix sums of their 64 child blocks in the previous layer.
    Each 64-bit integer represents the sum of occupied slots of 64 blocks in the layer directly beneath it.
    This scales largely: Layer 1 can hold data for 64 elements. Layer 2 can hold 4096, and so on.
    Layer 10 is the highest layer needed on a 64-bit system, as it can address ~1.1 quintillion elements per 64-bit integer.
    This scaling also allows for O(log_64 N) index-grabbing time complexity when data isn't contiguous.

compress() can be called to repair the data and make it contiguous again.
operator[] when the data is contiguous takes the fast path of directly grabbing the requested index.
This essentially makes it a lazy-erase array. You can erase any amount of elements, 
then call an O(N) compress() rather than repeated data shifting when calling erase() in traditional vectors.

Drawbacks:
- Index grabbing on punctured data is still fairly quick, but slower than when data is contiguous
- When even one element is erased, we must take the slow index grabbing path
- While metadata overhead is low, this data structure will take up a little more space than a traditional vector.

*///-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// To do:
// Three slashes indicate completion

// implement reallocate with a template param <compress>
/// change class definition to have template param <compresss_on_realloc>
/// On size/capacity modifying functions, add template param <compress>

/// Basic access / state
/// at(size_type logical_idx)
/// max_size()
/// is_contiguous() 

/// Element access
/// front()
/// back()

// Capacity / allocation
/// reserve(n)
// resize(n)
// shrink_to_fit()

// Insertion
/// emplace_back(Args&&... args)
/// push_back(const T& item)
/// push_back(T&& item)

// Removal
/// pop(size_type logical_idx): returns the popped element
/// pop_back()
/// pop_front()
/// clear()

// Erase strategies
// erase_unordered(size_type logical_idx)
// erase_compress(size_type logical_idx)
// erase_shift(size_type logical_idx)
// erase_range(size_type logical_first, size_type logical_last)
// erase_if(Func predicate)
// erase_range_if(size_type logical_first, size_type logical_last, Func predicate)

// HBA state / metadata
// hole_count()
// occupied()
// utilization()
// compress(float threshold)

// Iteration
// begin()
// end()
// cbegin()
// cend()
// rbegin()
// rend()

// Mapping
// physical_idx(logical_idx)
// logical_idx(physical_idx)

// Higher-order traversal
// for_each(Func f)
// for_each_physical(Func f)

// Copy / move / comparison
// operator=(const hba&)
// operator=(hba&&)
// operator==(const hba&)
// operator<=>
// friend void swap(hba&, hba&) noexcept

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
template <typename T, ull __num_layers=__SP_HBA_NUM_LAYERS__, bool _default_compress = __SP_HBA_DEFAULT_COMPRESS__, template <typename> typename Alloc = sp::allocator>
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

template <ull TargetLayer, ull CurrentLayer = 0>
SP_FORCEINLINE static constexpr ull _layer_offset_impl(size_type cap) noexcept{
    SP_IF_CONSTEXPR(CurrentLayer >= _num_layers){
        return 0;
    }else{
        const ull next = (cap + 63) >> 6;
        SP_IF_CONSTEXPR(CurrentLayer + 1 > TargetLayer){
            return next + _layer_offset_impl<TargetLayer, CurrentLayer + 1>(next);
        }else{
            return _layer_offset_impl<TargetLayer, CurrentLayer + 1>(next);
        }
    }
}

template <ull TargetLayer>
SP_FORCEINLINE static constexpr ull _layer_offset(size_type cap) noexcept{
    return _layer_offset_impl<TargetLayer>(cap);
}

SP_FORCEINLINE static constexpr ull _layer_offset(ull target_layer, size_type cap) noexcept{
    ull offset = 0;
    for(ull layer = _num_layers; layer > target_layer; --layer) offset += _layer_size(layer, cap);
    return offset;
}

SP_FORCEINLINE constexpr void _disable_slot(size_type idx){
    size_type layer1_block = _layer_offset<1>(_capacity) + (idx >> 6);
    _meta[layer1_block] &= ~(1ULL << (63 - (idx & 63)));
}

SP_FORCEINLINE constexpr void _enable_slot(size_type idx){
    size_type layer1_block = _layer_offset<1>(_capacity) + (idx >> 6);
    _meta[layer1_block] |= (1ULL << (63 - (idx & 63)));
}

template <ull Layer>
SP_FORCEINLINE constexpr void
_propagate_up_recursive(size_type block_idx, int _change_val) noexcept{
    const size_type meta_idx = _layer_offset<Layer>(_capacity) + block_idx;

    _meta[meta_idx] += _change_val;

    SP_IF_CONSTEXPR(Layer < _num_layers){
        _propagate_up_recursive<Layer + 1>(
            block_idx >> 6,
            _change_val
        );
    }
}

SP_FORCEINLINE constexpr void _propagate_up(size_type idx, int _change_val) noexcept{
    SP_IF_CONSTEXPR(_num_layers >= 2){
        const size_type block_idx = idx >> 12;
        _propagate_up_recursive<2>(block_idx, _change_val);
    }
}

SP_NODISCARD SP_FORCEINLINE constexpr bool _is_slot_active(size_type physical_idx) const noexcept{
    SP_IF_NOT_EXPECT(!_meta || physical_idx >= _capacity) return false;
    size_type mask_idx = physical_idx >> 6; 
    size_type bit_idx = physical_idx & 63;   
    return (_meta[_layer_offset<1>(_capacity) + mask_idx] & (1ULL << (63 - bit_idx))) != 0;
}

SP_NODISCARD SP_FORCEINLINE constexpr ull grow_capacity(ull size){ return sp::max((size_type)64, next_pow2(size+1)); }
 
template <ull CurrentLayer>
SP_FORCEINLINE SP_HOT constexpr void _descend_layers(size_type& remaining, size_type& hole_offset, size_type& block_offset) const noexcept{
    SP_IF_CONSTEXPR(CurrentLayer >= 2){ // We are only jumping down to layer 1 for the next part of the get_idx() logic
        size_type probe_idx = _layer_offset<CurrentLayer>(_capacity) + block_offset; // Grab first of 64 potential indices to jump down
        constexpr ull multiplied = 6 * CurrentLayer; // Scales with each layer
        while(_meta[probe_idx] < remaining){ // Probe until target child is found
            remaining -= _meta[probe_idx]; // Subtract the current prefix sum
            hole_offset += (1ULL << multiplied) - _meta[probe_idx++]; // Max elements addressible in current layer - actual numer of elements
            ++block_offset; // Move right by 1 at current layer
        }
        block_offset <<= 6; // Multiply by 64 to scale before jumping down again
        _descend_layers<CurrentLayer - 1>(remaining, hole_offset, block_offset); // Compile-time recursion
    }
}


// O(L*log_(64^L) N) -> O(log_64 N)
SP_NODISCARD SP_FORCEINLINE SP_HOT constexpr const size_type get_idx(size_type target_idx) const{
    size_type block_offset = 0; // Used for descending layers: Quick calculation when jumping down
    size_type hole_offset = 0; // How many holes to jump over at the end
    size_type remaining = target_idx; // How many elements still need to be seen
    _descend_layers<_num_layers>(remaining, hole_offset, block_offset); // Jump down to layer 1 for final logic; Target idx will be within 64 bitmasks
    size_type probe_idx = _layer_offset<1>(_capacity) + block_offset; // block_offset is scaled each time
    size_type cur = popcount(_meta[probe_idx]); // number of elements here
    // Jump over entire blocks at once
    while(cur && cur <= remaining){
        remaining -= cur; // Subtract number of elements from remaining
        hole_offset += (64 - cur); // add however many holes are in this block to the offset
        cur = popcount(_meta[++probe_idx]); // grab the data of the next block
    }
    ull meta_val = _meta[probe_idx]; // This is our target bitmask
    // Initial binary split chosen empirically.
    // 32 consistently provides the best overall performance across
    // lookup-heavy and erase-heavy benchmarks. Smaller splits (e.g. 16)
    // significantly regress erase throughput despite similar lookup cost.
    // Why the 32-bit split proves to be significantly faster than a deeper binary
    // search or a pure while-loop is still unknown.
    // It's suspected that 32-bit is quickest because it provides a single predictable branch
    // instead of branch mispredictions with deeper binary searches.
    int cnt_lo = popcount(meta_val >> 32); // Inspect the first 32 elements
    if(remaining>=cnt_lo){ // If true, the target index resides within the final 32 elements
        remaining -= cnt_lo; // Subtract number of elements from remaining
        hole_offset += (32 - cnt_lo); // Instantly add every hole in the lower half to our offset
        meta_val <<= 32; // move the higher half over
    }
    ull next_set_bit = leading_zeros(meta_val); // How many holes until we hit an element?
    while(remaining > 0){ // While there are still elements to count:
        hole_offset += next_set_bit; // Add our leading zeros to the total
        meta_val = (meta_val << next_set_bit) << 1; // Mask out all the zeros we counted
        next_set_bit = leading_zeros(meta_val); // Update the info for our modified bitmask
        --remaining; // We get rid of one existing element per iteration
    }
    if(meta_val) hole_offset += next_set_bit; // Account for holes immediately before the target element
    return target_idx + hole_offset; // Our real index + how many indices we must skip over
}

SP_FORCEINLINE constexpr void build_meta(){
    memset(_meta, 0, _calculate_meta_size(_capacity) * sizeof(ull));
    ull remaining = _size;
    ull idx = _layer_offset<1>(_capacity);
    while(remaining>=64){
        _meta[idx++] = ~0ULL;
        remaining -= 64;
    }if(remaining) _meta[idx] = ~0ULL << (64 - remaining);

    SP_IF_CONSTEXPR(_num_layers>1){
        ull child_start = _layer_offset<1>(_capacity); ull parent_start = _layer_offset(2, _capacity);
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

void destroy_elements(){
    if(!_data||!_meta) return;
    for(size_type i = 0; i < _capacity; ++i){
        if(_is_slot_active(i)) sp::allocator_traits<Alloc<T>>::destroy(_alloc, _data + i);
    }
}

void deallocate(){
    if(_meta) sp::allocator_traits<Alloc<ull>>::deallocate(_meta_alloc, _meta, _calculate_meta_size(_capacity));
    if(_data) sp::allocator_traits<Alloc<T>>::deallocate(_alloc, _data, _capacity);
}

// For now, compressed reallocation is forced.
// It will be difficult to perform non-compressed reallocation when the meta offsets need to grow.
template <bool compress=_default_compress>
SP_FORCEINLINE hba& reallocate(size_type n){
    //SP_IF_CONSTEXPR(compress){
        T* temp = sp::allocator_traits<Alloc<T>>::allocate(_alloc, n);
        size_type read_ptr = 0;
        size_type write_ptr = 0;
        while(read_ptr<_capacity){
            if(_is_slot_active(read_ptr)){
                SP_IF_CONSTEXPR(spt::is_trivially_copyable_v<T>) sp::allocator_traits<Alloc<T>>::construct(_alloc,temp+(write_ptr++),_data[read_ptr]);
                else sp::allocator_traits<Alloc<T>>::construct(_alloc,temp+(write_ptr++),sp::move(_data[read_ptr]));
            }
            ++read_ptr;
        }
        destroy_elements(); deallocate();

        _data = temp;
        temp = nullptr;

        _capacity = allocator_ext<Alloc<T>>::true_capacity(n);

        _meta = sp::allocator_traits<Alloc<ull>>::allocate(_meta_alloc, _calculate_meta_size(_capacity));

        build_meta();
        _is_contiguous = true;
    /*}else{

    }*/
    return *this;
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

constexpr size_type idx(size_type t) { return get_idx(t); }
SP_FORCEINLINE constexpr hba() : _data(nullptr), _meta(nullptr), _size(0), _capacity(allocator_ext<Alloc<T>>::true_capacity(0)), _is_contiguous(true){}
SP_FLATTEN constexpr hba(size_type size) : hba(size, T()){}
constexpr hba(size_type size, type_param val){
    size_type target_size = next_pow2(size);
    _SP_INIT_CDM_TS_
    _SP_APPLY_UNROLLED_(size, {
        sp::allocator_traits<Alloc<T>>::construct(_alloc, _data+_size,val);
        ++_size;
    });
    build_meta();
}
constexpr hba(std::initializer_list<T> list){
    size_type target_size = next_pow2(list.size());
    _SP_INIT_CDM_TS_
    for(type_param i : list){
        sp::allocator_traits<Alloc<T>>::construct(_alloc, _data+_size,i);
        ++_size;
    }
    build_meta();
}
#undef _SP_INIT_CDM_TS

SP_FORCEINLINE constexpr hba(const hba& other) : _size(other._size), _capacity(other._capacity), _is_contiguous(other._is_contiguous){
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
SP_FORCEINLINE constexpr hba(hba&& other) : _size(other._size), _capacity(other._capacity), 
_is_contiguous(other._is_contiguous),_data(sp::move(other._data)),_meta(sp::move(other._meta)),
_alloc(sp::move(other._alloc)),_meta_alloc(sp::move(other._meta_alloc)){
    other._size = 0; other._capacity = 0; other._is_contiguous = true;
    other._data = nullptr; other._meta = nullptr;
}

SP_CONSTEXPR20 ~hba(){
    if(_data) destroy_elements();
    if(_meta) deallocate();
}

//============================//============================//============================//============================
//============================//============================//============================//============================
//============================//============================//============================//============================

SP_FORCEINLINE constexpr const T& operator[](size_type target_idx) const { return (_is_contiguous ? _data[target_idx] : _data[get_idx(target_idx)]); }
SP_FORCEINLINE constexpr T& operator[](size_type target_idx) { return (_is_contiguous ? _data[target_idx] : _data[get_idx(target_idx)]); }
SP_FORCEINLINE constexpr const T& at(size_type target_idx) const { return (_is_contiguous ? _data[target_idx] : _data[get_idx(target_idx)]); }
SP_FORCEINLINE constexpr T& at(size_type target_idx) { return (_is_contiguous ? _data[target_idx] : _data[get_idx(target_idx)]); }
SP_FORCEINLINE constexpr size_type max_size() { return npos; }
SP_FORCEINLINE constexpr bool is_contiguous() { return _is_contiguous; }
SP_FORCEINLINE constexpr const T* data() const { return _data; }
SP_FORCEINLINE constexpr const ull* get_meta() const { return _meta; }
SP_FORCEINLINE constexpr ull get_meta_size() const { return _calculate_meta_size(_capacity); }
SP_FORCEINLINE constexpr size_type size() const { return _size; }
SP_FORCEINLINE constexpr size_type capacity() const { return _capacity; }
SP_FORCEINLINE constexpr void set_contig(bool condition) { _is_contiguous = condition; }
SP_FORCEINLINE constexpr bool empty() { return _size==0; }
SP_FORCEINLINE constexpr bool is_empty() { return _size==0; }
SP_FORCEINLINE constexpr bool is_slot_active(size_type slot) { return _is_slot_active(slot); }
/*SP_FORCEINLINE constexpr const T& front() const { return (_data != nullptr) ? _data[0] : T(); }
SP_FORCEINLINE constexpr T& front() { return (_data != nullptr) ? _data[0] : T(); }
SP_FORCEINLINE constexpr const T& back() const { return _data != nullptr ? _data[_size-1] : T(); }
SP_FORCEINLINE constexpr T& back() { return (_data != nullptr) ? _data[_size-1] : T(); }*/

template <bool compress = _default_compress>
SP_FORCEINLINE constexpr hba& reserve(size_type n){
    size_type target_size = grow_capacity(n);
    SP_IF_NOT_EXPECT(target_size<=n) return *this;
    SP_MUSTTAIL return reallocate<compress>(n);
}

// Compress: Two-pointer (read pointer and write pointer), O(N) Time, O(1) Space
SP_FORCEINLINE constexpr hba& compress(){
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

SP_FORCEINLINE constexpr hba& erase(size_type target_idx){
    const size_type idx = (_is_contiguous) ? target_idx : get_idx(target_idx);
    sp::allocator_traits<Alloc<T>>::destroy(_alloc, _data+idx);
    _disable_slot(idx);
    _propagate_up(idx, -1);
    _is_contiguous = false;
    --_size;
    return *this;
}

SP_FORCEINLINE constexpr hba& erase_unordered(size_type target_idx){
    const size_type idx = (_is_contiguous) ? target_idx : get_idx(target_idx);
    if(idx!=_size-1){
        const size_type end_idx = (_is_contiguous) ? _size-1 : get_idx(_size-1);
        sp::swap(_data[idx],_data[end_idx]);
        sp::allocator_traits<Alloc<T>>::destroy(_alloc, _data+end_idx);
        _disable_slot(end_idx);
        _propagate_up(end_idx,-1);
        --_size;
    }else{
        sp::allocator_traits<Alloc<T>>::destroy(_alloc, _data+idx);
        _disable_slot(idx);
        _propagate_up(idx,-1);
        --_size;
    }
    return *this;
}

/// erase_unordered(size_type logical_idx)
// erase_compress(size_type logical_idx)
// erase_shift(size_type logical_idx)
// erase_range(size_type logical_first, size_type logical_last)
// erase_if(Func predicate)
// erase_range_if(size_type logical_first, size_type logical_last, Func predicate)

template <bool compress = _default_compress, typename... Args>
SP_FORCEINLINE constexpr hba& emplace(size_type target_idx, Args&&... args){
    SP_IF_NOT_EXPECT(_size>=_capacity) reallocate<compress>(grow_capacity(_capacity));
    const size_type idx = (_is_contiguous) ? target_idx : get_idx(target_idx);
    T item_to_place(sp::forward<Args>(args)...);
    size_type hole_idx = idx;
    
    while(hole_idx < _capacity && _is_slot_active(hole_idx)){
        sp::swap(_data[hole_idx], item_to_place);
        ++hole_idx;
    }
    
    sp::allocator_traits<Alloc<T>>::construct(_alloc, _data + hole_idx, sp::move(item_to_place));
    _enable_slot(hole_idx);
    _propagate_up(hole_idx, 1);
    ++_size;

    return *this;
}

template <bool compress = _default_compress, typename... Args>
SP_FORCEINLINE constexpr hba& emplace_back(Args&&... args){
    // There are two possible options for the semantics of emplace_back
    // Option 1 is to only reallocate if size >= capacity, and simply shift elements backwards to a hole if the end is occupied.
    // Option 2 is what I decided to go with:
    // If _size>=_capacity OR the final slot is occupied, reallocation occurs.
    SP_IF_NOT_EXPECT(_size>=_capacity||_is_slot_active(_capacity-1)) reallocate<compress>(grow_capacity(_capacity));
    const size_type idx = (_is_contiguous) ? _size : get_idx(_size); // Index to the right of the furthest element
    sp::allocator_traits<Alloc<T>>::construct(_alloc, _data + idx, sp::forward<Args>(args)...);
    _enable_slot(idx);
    _propagate_up(idx, 1);
    ++_size;
    return *this;
}

template <bool compress = _default_compress, typename... Args>
SP_FORCEINLINE constexpr hba& emplace_front(Args&&... args) { return emplace<compress>(0, sp::forward<Args>(args)...); }

template <bool compress = _default_compress>
SP_FORCEINLINE constexpr hba& insert(size_type target_idx, const T& val) { return emplace<compress>(target_idx, val); }
template <bool compress = _default_compress>
SP_FORCEINLINE constexpr hba& insert(size_type target_idx, T&& val) { return emplace<compress>(target_idx, sp::move(val)); }
template <bool compress = _default_compress>
SP_FORCEINLINE constexpr hba& push_front(const T& val) { return emplace_front<compress>(val); }
template <bool compress = _default_compress>
SP_FORCEINLINE constexpr hba& push_front(T&& val) { return emplace_front<compress>(sp::move(val)); }
template <bool compress = _default_compress>
SP_FORCEINLINE constexpr hba& push_back(const T& val) { return emplace_back<compress>(val); }
template <bool compress = _default_compress>
SP_FORCEINLINE constexpr hba& push_back(T&& val) { return emplace_back<compress>(sp::move(val)); }

SP_FORCEINLINE constexpr T pop(size_type logical_idx){
    const size_type idx = (_is_contiguous) ? logical_idx : get_idx(logical_idx);
    T popped = sp::move(_data[idx]);
    _disable_slot(idx);
    _propagate_up(idx, -1);
    --_size;
    return popped;
}

SP_FORCEINLINE constexpr T pop_back() { return pop(_size-1); }
SP_FORCEINLINE constexpr T pop_front() { return pop(0); }
SP_FORCEINLINE constexpr hba& clear(){
    SP_IF_CONSTEXPR(!spt::is_trivially_destructible_v<T>){
        ull idx = 0;
        while(_size>0){
            if(_is_slot_active(idx)) {
                sp::allocator_traits<Alloc<T>>::destroy(_alloc, _data+idx);
                --_size;
            }
            ++idx;
        }
    }
    std::memset(_meta,0,_calculate_meta_size(_capacity)*sizeof(ull));
    _size = 0;
    _is_contiguous = true;
}

SP_FORCEINLINE void print(){
    sp::print("[");
    for(size_type i = 0; i < _size; ++i){
        sp::print((*this)[i]);
        SP_IF_EXPECT(i != _size - 1) sp::print(", ");
    }
    sp::println("]");
}
};
} // namespace sp
#endif