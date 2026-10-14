#ifndef ____SP_HASH_MAP____
#define ____SP_HASH_MAP____
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cmath>

#include "../setup/init.hpp"
#include "../core/type_traits.hpp"
#include "../containers/string.hpp"
#include "../containers/array.hpp" // uses backend of my array class in "unsafe mode": 5x faster than clang/LLVM vector class
#include "../math/hashes.hpp" // the class with default hash function
#include "../math/bit_manip.hpp" // includes useful functions such as wrappers for __builtin_ctzll (count trailing zeros on a long long)

#ifndef _SP_CHECK_SAFETY_
    #define _SP_CHECK_SAFETY_(level) SP_IF_CONSTEXPR(safety>=level)
#endif

#ifndef _SP_SAFETY_TEMPLATE_
    #define _SP_SAFETY_TEMPLATE_ template<short safety=_safety_level>
#endif

#define _SP_PROBE_LOOP_(...) \
        size_type cap = _buckets.size(); \
        size_type mask = cap - 1; \
        size_type probe=0; \
        SP_IF_CONSTEXPR(hash_with_cap) probe = (Hash()(key, cap) & mask); \
        else probe = (Hash()(key) & mask); \
        prefetch_info(probe, mask); \
        while(get_state(probe)){ \
            _SP_CHECK_SAFETY_(1) if(_buckets[probe].first==key) { __VA_ARGS__ } \
            probe = (probe + 1) & mask; \
            prefetch_info(probe, mask); \
        }

#define _SP_SIZE_REALLOC_CHECK_(func, K, V) \
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(_count>=_max_count){ \
            rehash(); \
            goto __sp_start; \
        } \
        _SP_CHECK_SAFETY_(2){ \
            try{ \
                _buckets[probe].first = Key(K); \
                _buckets[probe].second = Value(V); \
            }catch(...){ \
                throw exceptions::MapException((std::string("Exception occurred during ") + #func).c_str()); \
            } \
        }else{ \
            _buckets[probe].first = Key(K); \
            _buckets[probe].second = Value(V); \
        }

namespace sp {

/// Open-addressing hash map with linear probing and a bitset occupancy array.
/// Aligned to the system cache line to prevent false sharing.
/// @tparam Key           Key type.
/// @tparam Value         Mapped value type.
/// @tparam _safety_level 0 = no checks, 1 = bounds+realloc guards, 2 = exception-wrapped ops.
/// @tparam _threshold    Integer load-factor percentage (0–100) before automatic rehash.
/// @tparam Hash          Hash functor; may expose `operator()(key, cap)` for capacity-aware hashing.
/// @tparam Allocator     Allocator template; user writes `sp::allocator`, not `sp::allocator<int>`.
template <typename Key, typename Value, short _safety_level=1, int _threshold = 70, typename Hash = sp::basic_hash, template <typename> class Allocator = sp::allocator>
class alignas(sp_cache_line_size) hash_map {
private:
struct Entry {
    Key first;   ///< Lookup key; duplicates are not allowed.
    Value second; ///< Value associated with the key.
    constexpr Entry(){}
    SP_CONSTEXPR20 ~Entry(){}
    constexpr bool operator==(const Entry& other) const {
        return first == other.first && second == other.second;
    }
    constexpr bool operator!=(const Entry& other) const {
        return !(*this == other);
    }
};
    
    _SP_GRANT_IO_ACCESS_
    SP_FORCEINLINE sp::pair<Entry*, size_type> __getSpiralBinary() const{ return {_buckets._data, _max_count*sizeof(Entry)}; }
    SP_FORCEINLINE const char* __getSpiralMessage() const;

    size_type _count = 0;     ///< Number of entries currently in the map.
    size_type _max_count = 0;  ///< Bucket count × threshold: max entries before rehash.

    sp::array<Entry, 0, Allocator> _buckets; ///< Flat bucket array.
    sp::array<ull, 0, Allocator> _states;    ///< Bitset: each bit marks a bucket as occupied.

    static constexpr double threshold = static_cast<double>(_threshold) / 100;

    static constexpr bool key_trivially_copyable = spt::is_trivially_copyable_v<Key>;
    static constexpr bool value_trivially_copyable = spt::is_trivially_copyable_v<Value>;
    static constexpr bool entry_trivially_copyable = (key_trivially_copyable && value_trivially_copyable);
    static constexpr bool hash_with_cap = SP_HAS_METHOD(Hash, with_cap);

    using entry_loop_type = spt::conditional_t<entry_trivially_copyable, Entry, Entry&>;
    using const_entry_loop_type = spt::conditional_t<entry_trivially_copyable, const Entry, const Entry&>;

public:

// ====================================================================================================================================
// ====================================================================================================================================
// ====================================================================================================================================
// ====================================================================================================================================

/// Bidirectional iterator that skips empty buckets.
/// When the load factor is below 30 % it "teleports" by scanning the 64-bit
/// state chunks instead of checking every slot individually.
/// @tparam IsConst `true` for `const_iterator`, `false` for mutable `iterator`.
template <bool IsConst>
class iterator_impl {
    friend class hash_map;
private:
    using bucket_ptr = spt::conditional_t<IsConst, const Entry*, Entry*>;
    using state_ptr  = const ull*;

    bucket_ptr _ptr_buckets;
    state_ptr  _ptr_states;
    size_type  _index;
    size_type  _capacity;
    bool _teleport;

    // Private constructor for hash_map::begin() and end()
    constexpr iterator_impl(bucket_ptr b, state_ptr s, size_type idx, size_type cap, bool teleport = false) : _ptr_buckets(b), _ptr_states(s), _index(idx), _capacity(cap), _teleport(teleport) {}

public:
    //using iterator_category = sp::forward_iterator_tag;
    using Val = spt::conditional_t<IsConst, const Entry, Entry>;
    using difference_type = std::ptrdiff_t;
    using pointer = Val*;
    using reference = Val&;

    constexpr iterator_impl() : _ptr_buckets(nullptr), _ptr_states(nullptr), _index(0), _capacity(0), _teleport(false) {}

    // Allow conversion from iterator to const_iterator
    template <bool OtherIsConst>
    constexpr iterator_impl(const iterator_impl<OtherIsConst>& other) 
        : _ptr_buckets(other._ptr_buckets), _ptr_states(other._ptr_states), 
          _index(other._index), _capacity(other._capacity), _teleport(other._teleport) {}

    constexpr reference operator*() const { return _ptr_buckets[_index]; }
    constexpr pointer operator->() const { return _ptr_buckets+_index; }

    SP_HOT constexpr iterator_impl& operator++(){ // teleports between occupied buckets instead of checking each one's occupation status
        if(_teleport){
            _index++;
            SP_IF_NOT_EXPECT(_index>=_capacity){ // return if at end
                _index = _capacity; // safe guard
                return *this; 
            }
            size_type chunk_idx = _index >> 6;
            size_type mask = (~0ULL << (_index & 63)); // filter to unviewed bits (all bits to the LEFT of target index)
            size_type current_chunk = _ptr_states[chunk_idx] & mask; // apply the mask to the 64 "loaded" states: only display "new" ones
            size_type max_state_len = _capacity >> 6; // ensure we don't pass this
            if(!current_chunk){ // if empty, jump to next 64-bit chunk
                size_type max_state_len = _capacity >> 6;
                size_type p_idx = (chunk_idx + 16) & (max_state_len - 1); // pointer mask
                _SP_PREFETCH_(&_ptr_states[p_idx], 0, 3); // prefetch ahead to reduce CPU stall time
                do {
                    chunk_idx++;
                    SP_IF_NOT_EXPECT(chunk_idx >= max_state_len) { // passed the end chunk
                        _index = _capacity;
                        return *this;
                    }
                    current_chunk = _ptr_states[chunk_idx];
                } while(!current_chunk);
            }
            // loop ensures a set bit exists; no need for ffsll, can use ctzll
            _index = (chunk_idx << 6) + trailing_zeros(current_chunk);
            return *this;
        }else{
            do{
                SP_IF_CONSTEXPR(sizeof(Entry)>=16){
                    size_type lookahead = (_index + lookahead_len()) & (_capacity-1);
                    _SP_PREFETCH_(&_ptr_buckets[lookahead], 0, 1);
                    _SP_PREFETCH_(&_ptr_states[lookahead >> 6], 0, 1);
                }
                _index++;
                SP_IF_NOT_EXPECT(_index>=_capacity){
                    _index = _capacity;
                    return *this;
                }
            }while(!(_ptr_states[_index >> 6] & (1ULL << (_index & 63))));
            return *this;
        }
    }
    SP_HOT SP_FLATTEN constexpr iterator_impl operator++(int){
        iterator_impl temp(*this);
        ++(*this);
        return temp;
    }

    SP_HOT constexpr iterator_impl& operator--(){
        if(_teleport){
            _index--;
            SP_IF_NOT_EXPECT(_index==0){
                return *this;
            }
            size_type chunk_idx = _index >> 6;
            size_type mask = (~0ULL >> (63-(_index&63)));
            size_type current_chunk = _ptr_states[chunk_idx] & mask;
            if(!current_chunk){
                do{
                    chunk_idx--;
                    current_chunk = _ptr_states[chunk_idx];
                    SP_IF_NOT_EXPECT(chunk_idx==0&&!current_chunk){
                        _index = 0;
                        return *this;
                    }
                }while(!current_chunk);
            }
            _index = (chunk_idx << 6) + (63-leading_zeros(current_chunk));
            return *this;
        }else{
            do{
                _index--;
                SP_IF_NOT_EXPECT(_index==0){
                    return *this;
                }
            }while(!(_ptr_states[_index >> 6] & (1ULL << (_index & 63))));
            return *this;
        }
    }
    SP_HOT SP_FLATTEN constexpr iterator_impl operator--(int){
        iterator_impl temp = *this;
        --(*this);
        return temp;
    }

    constexpr bool operator==(const iterator_impl& other) const { return _index == other._index; }
    constexpr bool operator!=(const iterator_impl& other) const { return _index != other._index; }
};

    using iterator               = iterator_impl<false>;
    using const_iterator         = iterator_impl<true>;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

// ====================================================================================================================================
// ====================================================================================================================================
// ====================================================================================================================================
// ====================================================================================================================================

private:

    static constexpr SP_FORCEINLINE size_type lookahead_len(){
        return sp_cache_line_size / sizeof(Entry);
    }

    static constexpr SP_FORCEINLINE SP_CONST size_type p_state_array_size(size_type cap) {
        return (cap + 63) >> 6;
    }

    SP_FORCEINLINE SP_PURE constexpr bool get_state(size_type idx) const {
        return (_states[idx >> 6] & (1ULL << (idx & 63)));
    }

    SP_FORCEINLINE constexpr void set_state(size_type idx) {
        _states[idx >> 6] |= (1ULL << (idx & 63));
    }

    SP_FORCEINLINE constexpr void clear_state(size_type idx) {
        _states[idx >> 6] &= ~(1ULL << (idx & 63));
    }

    SP_FORCEINLINE constexpr hash_map& init_buckets() {
        size_type size = _buckets.size();
        size_type bytes = p_state_array_size(size);
        memset(_states.data(), 0, _states.size() * sizeof(ull));
        _max_count = static_cast<size_type>(size * threshold);
        return *this;
    }

    SP_FORCEINLINE constexpr void prefetch_info(size_type probe, size_type mask){
        SP_IF_CONSTEXPR(sizeof(Entry)>=16){
            size_type lookahead = (probe + lookahead_len()) & mask;
            _SP_PREFETCH_(&_buckets[lookahead], 0, 1);
            _SP_PREFETCH_(&_states[lookahead >> 6], 0, 1);
        }
    }
    // const version
    SP_FORCEINLINE constexpr void prefetch_info(size_type probe, size_type mask) const {
        SP_IF_CONSTEXPR(sizeof(Entry)>=16){
            size_type lookahead = (probe + lookahead_len()) & mask;
            _SP_PREFETCH_(&_buckets[lookahead], 0, 1);
            _SP_PREFETCH_(&_states[lookahead >> 6], 0, 1);
        }
    }

    SP_FORCEINLINE constexpr size_type get_chunk_idx(size_type probe){
        return probe >> 6;
    }

    SP_FORCEINLINE constexpr size_type get_mask(size_type index){
        return (~0ULL << (index & 63));
    }

    SP_FORCEINLINE constexpr size_type get_current_chunk(size_type chunk_idx, size_type mask){
        return _states[chunk_idx] & mask;
    }

    SP_COLD SP_FLATTEN constexpr void rehash(size_type bucket_count=0){ // OPTIMIZATION AVAILABLE: NOT and ctzll to find next hole
        size_type count = (bucket_count==0) ? next_pow2((size_type)(_buckets.size()/threshold)) : next_pow2(bucket_count);
        sp::array<Entry, 0, Allocator> temp(count);
        sp::array<ull, 0, Allocator> temp_states(p_state_array_size(count));
        _max_count = static_cast<size_type>(count * threshold);
        for(entry_loop_type e : *this){
            size_type mask = count - 1;
            size_type probe = Hash()(e.first) & mask;
            while((temp_states[probe >> 6] & (1ULL << (probe & 63)))){
                probe = (probe + 1) & mask;
                prefetch_info(probe, mask);
            }
            SP_IF_CONSTEXPR(key_trivially_copyable) temp[probe].first = sp::move(e.first);
            else temp[probe].first = e.first;
            SP_IF_CONSTEXPR(value_trivially_copyable) temp[probe].second = sp::move(e.second);
            else temp[probe].second = e.second;
            temp_states[probe >> 6] |= (1ULL << (probe & 63));
        }
        _buckets = sp::move(temp);
        _states = sp::move(temp_states);
    }

public:

    // ─────────────────────────────────────────────────────────────────────────────
    // Constructors / assignment
    // ─────────────────────────────────────────────────────────────────────────────

    /// Default constructor — allocates 16 buckets.
    SP_FORCEINLINE constexpr hash_map() : _buckets(16), _count(0), _max_count(static_cast<size_type>(16 * threshold)), _states(p_state_array_size(16)) {
        init_buckets();
    }
    
    /// Constructs with at least @p size bucket slots (rounded up to next power of two, min 16).
    SP_FORCEINLINE constexpr hash_map(size_type size) : _count(0) {
        size_type num = next_pow2(size);
        if (num < 16) num = 16;
        _buckets = sp::array<Entry, 0, Allocator>(num);
        _states = sp::array<ull, 0, Allocator>(p_state_array_size(num));
        init_buckets();
    }

    /// Constructs from an initializer list of `{key, value}` pairs.
    SP_FLATTEN constexpr hash_map(std::initializer_list<sp::pair<Key,Value>> list) : _count(0) {
        size_type required = static_cast<size_type>(static_cast<float>(list.size()) / threshold) + 1;
        size_type num = next_pow2(required);
        if(num < 16) num = 16;
        _buckets = sp::array<Entry, 0, Allocator>(num);
        _states = sp::array<ull, 0, Allocator>(p_state_array_size(num));
        init_buckets();
        for(auto& i : list){
            insert_as<Key,0>(i.first, i.second);
        }
    }

    /// Copy constructor.
    SP_FLATTEN constexpr hash_map(const hash_map& other){
        *this = other;
    }
    /// Move constructor.
    SP_FLATTEN constexpr hash_map(hash_map&& other) noexcept{
        *this = sp::move(other);
    }

    /// Range constructor — inserts all `[first, last)` key-value pairs.
    template <class InputIt>
    SP_FLATTEN constexpr hash_map(InputIt first, InputIt last){
        size_type required = static_cast<size_type>(static_cast<float>(std::distance(first, last)) / threshold) + 1;
        size_type num = next_pow2(required);
        if(num < 16) num = 16;
        _buckets = sp::array<Entry, 0, Allocator>(num);
        _states = sp::array<ull, 0, Allocator>(p_state_array_size(num));
        init_buckets();
        for(auto it = first; it != last; ++it){
            insert<0>((*it).first, (*it).second);
        }
    }

    /// Initializer-list assignment — clears and re-inserts all pairs.
    constexpr hash_map& operator=(std::initializer_list<sp::pair<Key, Value>> ilist) {
        clear();                                    // safe reset first
        reserve(static_cast<size_type>(ilist.size() / threshold) + 16);
        for (const auto& p : ilist) {
            insert<0>(p.first, p.second);           // use safe insert
        }
        return *this;
    }

    /// Copy assignment.
    constexpr hash_map& operator=(const hash_map& other) {
        if (this != &other) {
            _buckets = other._buckets;
            _states  = other._states;
            _count   = other._count;
            _max_count = other._max_count;
        }
        return *this;
    }

    /// Move assignment.
    constexpr hash_map& operator=(hash_map&& other) noexcept {
        if (this != &other) {
            _buckets   = sp::move(other._buckets);
            _states    = sp::move(other._states);
            _count     = other._count;
            _max_count = other._max_count;
            /*other._buckets = array<Entry, 0>(16);
            other._states = array<ull, 0>(1);
            other.init_buckets();*/
            other._count = 0;
            other._max_count = 0;
        }
        return *this;
    }


// ========== // ========== // ========== // ========== // ========== // ========== // ========== // ==========
// ========== // ========== // ========== // ========== // ========== // ========== // ========== // ==========
                                            // public functions
// ========== // ========== // ========== // ========== // ========== // ========== // ========== // ==========
// ========== // ========== // ========== // ========== // ========== // ========== // ========== // ==========

    /// Prints all key→value pairs to stdout with colour formatting.
    SP_FORCEINLINE hash_map& print() {
        for(const_entry_loop_type e : *this) {   // use const version
            std::cout << e.first << console::FG_BRIGHT_GREEN << " -> " 
                    << console::RESET_EFFECTS << e.second << std::endl;
        }
        return *this;
    }

    /// Prints detailed diagnostics (capacity, load factor, probe stats, memory) to stdout.
    SP_FORCEINLINE hash_map& print_stats(){
        std::cout << console::FG_BRIGHT_RED << "-----------------------------------------" << console::FG_BRIGHT_YELLOW << std::endl;
        const size_type cap = capacity();
        const size_type cnt = size();

        std::cout << "Bucket count (capacity): " << cap << "\n";
        std::cout << "Size (used slots): " << cnt << "\n";
        std::cout << "Empty slots: " << empty_slots();
        if(cap) std::cout << " (" << (static_cast<double>(empty_slots())/static_cast<double>(cap))*100.0 << "%)";
        std::cout << "\n";

        std::cout << "Load factor: " << load_factor() << "\n";
        std::cout << "Max load factor (threshold): " << max_load_factor() << "\n";
        std::cout << "Max tolerated elements: " << max_size() << "\n";

        std::cout << "States array size (#chunks): " << states_array_size() << "\n";
        std::cout << "Occupied state chunks: " << occupied_state_chunks();
        if(states_array_size()) std::cout << " (" << (static_cast<double>(occupied_state_chunks())/static_cast<double>(states_array_size()))*100.0 << "%)";
        std::cout << "\n";

        std::cout << "Total collisions: " << total_collisions() << "\n";
        std::cout << "Average probe length: " << average_probe_length() << "\n";
        std::cout << "Max probe length: " << max_probe_length() << "\n";

        decltype(sizeof(0)) mem = memory_usage();
        // print anywhere from bytes to tb
        if(mem < 1024) std::cout << "Memory usage: " << mem << " bytes\n";
        else if(mem < 1024*1024) std::cout << "Memory usage: " << mem/1024.0 << " KB\n";
        else if(mem < 1024*1024*1024) std::cout << "Memory usage: " << mem/(1024.0*1024.0) << " MB\n";
        else if(mem < 1024ULL*1024ULL*1024ULL*1024ULL) std::cout << "Memory usage: " << mem/(1024.0*1024.0*1024.0) << " GB\n";
        else std::cout << "Memory usage: " << mem/(1024.0*1024.0*1024.0*1024.0) << " TB\n";

        /*sp::array<size_type> hist = bucket_occupancy_histogram();
        size_type max_len = max_probe_length();
        if(max_len > 0){
            std::cout << "Probe length distribution (length->count):\n";
            for(size_type i = 0; i <= max_len; ++i){
                if(hist[i]) std::cout << "  " << i << "->" << hist[i] << "\n";
            }
        }*/

        std::cout << console::FG_BRIGHT_RED << "-----------------------------------------" << console::RESET_EFFECTS << std::endl;
        return *this;
    }



    // ─────────────────────────────────────────────────────────────────────────────
    // Capacity & basic state
    // ─────────────────────────────────────────────────────────────────────────────
    _SP_SAFETY_TEMPLATE_ _SP_FUNC_NIP_ constexpr bool is_empty() const noexcept { return !((bool)(_count)); }          ///< Returns `true` if the map has no entries.
    _SP_SAFETY_TEMPLATE_ _SP_FUNC_NIP_ constexpr size_type size() const noexcept { return _count; }                    ///< Number of entries currently stored.
    _SP_SAFETY_TEMPLATE_ _SP_FUNC_NIP_ constexpr size_type max_size() const noexcept { return _max_count; }            ///< Maximum entries before the next automatic rehash.
    _SP_SAFETY_TEMPLATE_ _SP_FUNC_NIP_ constexpr size_type capacity() const noexcept { return _buckets.size(); }       ///< Total number of allocated bucket slots.
    _SP_SAFETY_TEMPLATE_ _SP_FUNC_NIP_ constexpr float load_factor() const noexcept { return (double)_count / _buckets.size(); }  ///< Current load factor (size / capacity).
    _SP_SAFETY_TEMPLATE_ _SP_FUNC_NI_ SP_CONST constexpr double max_load_factor() const noexcept { return threshold; } ///< Compile-time load-factor threshold (0–1).
    /// Ensures capacity for at least @p n elements, rehashing if necessary.
    _SP_SAFETY_TEMPLATE_
    SP_FLATTEN SP_COLD constexpr void reserve(size_type n){
        SP_IF_NOT_EXPECT(n<=_count) return;
        SP_IF_NOT_EXPECT(n<=_buckets.size()) return;
        SP_IF_NOT_EXPECT(_count==0){
            size_type new_cap = next_pow2((size_type)(n/threshold));
            _buckets = array<Entry, 0>(new_cap);
            _states = array<ull, 0, Allocator>(p_state_array_size(new_cap));
            _max_count = (size_type)(_buckets.size() * threshold);
        }else{
            rehash(n);
        }
    }
    /// Shrinks the bucket array to the smallest power-of-two that fits the current entries.
    _SP_SAFETY_TEMPLATE_
    SP_FLATTEN constexpr void shrink_to_fit(){
        SP_IF_NOT_EXPECT(_count==0){
            _buckets = array<Entry, 0>(16);
            _states = array<ull, 0, Allocator>(1);
            init_buckets();
        }else if(_count < _buckets.size()){
            size_type count = next_pow2(_count / threshold);
            sp::array<Entry, 0, Allocator> temp(count);
            sp::array<ull, 0, Allocator> temp_states(p_state_array_size(count));
            _max_count = static_cast<size_type>(count * threshold);
            for(entry_loop_type e : *this){
                size_type mask = count - 1;
                size_type probe = Hash()(e.first) & mask;
                while((temp_states[probe >> 6] & (1ULL << (probe & 63)))){
                    probe = (probe + 1) & mask;
                    prefetch_info(probe, mask);
                }
                SP_IF_CONSTEXPR(key_trivially_copyable) temp[probe].first = sp::move(e.first);
                else temp[probe].first = e.first;
                SP_IF_CONSTEXPR(value_trivially_copyable) temp[probe].second = sp::move(e.second);
                else temp[probe].second = e.second;

                temp_states[probe >> 6] |= (1ULL << (probe & 63));
            }
            _buckets = sp::move(temp);
            _states = sp::move(temp_states);
        }
    }

    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NIP_ constexpr size_type bucket_count() const noexcept { return _buckets.size(); } ///< Alias for `capacity()`.

    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NIP_ constexpr size_type empty_slots() const{ return _buckets.size() - _count; }  ///< Number of unoccupied bucket slots.
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NIP_ constexpr size_type used_slots() const{ return _count; }                      ///< Alias for `size()`.
    /// Returns the sum of probe displacements across all occupied buckets.
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NIP_ constexpr size_type total_collisions() const{
        size_type cap = _buckets.size();
        SP_IF_NOT_EXPECT(cap == 0) return 0;
        const size_type mask = cap - 1;
        size_type sum = 0;
        for(size_type i = 0; i < cap; ++i){
            if(!get_state(i)) continue;
            const Entry& e = _buckets[i];
            size_type ideal = (Hash()(e.first, cap) & mask);
            sum += (i + cap - ideal) & mask;
        }
        return sum;
    }
    /// Returns the longest probe chain length across all occupied buckets.
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NIP_ constexpr size_type max_probe_length() const{
        size_type cap = _buckets.size();
        SP_IF_NOT_EXPECT(cap == 0) return 0;
        const size_type mask = cap - 1;
        size_type max_length = 0;
        for(size_type i = 0; i < cap; ++i){
            if(!get_state(i)) continue;
            const Entry& e = _buckets[i];
            size_type ideal = (Hash()(e.first, cap) & mask);
            size_type probe_length = (i + cap - ideal) & mask;
            max_length = std::max(max_length, probe_length);
        }
        return max_length;
    }
    /// Returns the mean probe chain length across all occupied buckets.
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NIP_ constexpr double average_probe_length() const{
        size_type cap = _buckets.size();
        SP_IF_NOT_EXPECT(cap == 0) return 0;
        const size_type mask = cap - 1;
        size_type total_length = 0;
        size_type count = 0;
        for(size_type i = 0; i < cap; ++i){
            if(!get_state(i)) continue;
            const Entry& e = _buckets[i];
            size_type ideal = (Hash()(e.first, cap) & mask);
            size_type probe_length = (i + cap - ideal) & mask;
            total_length += probe_length;
            count++;
        }
        return static_cast<double>(total_length) / count;
    }
    /// Returns the number of 64-bit state chunks that contain at least one occupied bit.
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NIP_ constexpr size_type occupied_state_chunks() const{
        size_type cap = _states.size();
        SP_IF_NOT_EXPECT(cap == 0) return 0;
        size_type count = 0;
        for(size_type i = 0; i < cap; ++i){
            if(get_state(i)) count++;
        }
        return count;
    }
    /// Returns the total number of 64-bit state chunks (= `ceil(capacity / 64)`).
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NIP_ constexpr size_type states_array_size() const{ return _states.size(); }

    /// Returns a histogram mapping probe-length → bucket count.
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NIP_ sp::array<size_type> bucket_occupancy_histogram() const{
        size_type cap = _buckets.size();
        sp::array<size_type> histogram(cap, 0);
        for(size_type i = 0; i < cap; ++i){
            if(get_state(i)){
                const Entry& e = _buckets[i];
                size_type ideal = (Hash()(e.first, cap) & (cap - 1));
                histogram[(i + cap - ideal) & (cap - 1)]++;
            }
        }
        return histogram;
    }
    /// Returns the number of probes needed to reach @p key from its ideal bucket.
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NIP_ constexpr size_type probe_count(const Key& key) const{
        size_type cap = _buckets.size();
        SP_IF_NOT_EXPECT(cap == 0) return 0;
        const size_type mask = cap - 1;
        size_type count = 0;
        for(size_type i = 0; i < cap; ++i){
            if(!get_state(i)) continue;
            const Entry& e = _buckets[i];
            size_type ideal = (Hash()(e.first, cap) & mask);
            if(ideal == (i & mask)) break;
            count++;
        }
        return count;
    }

    /// Returns the total heap memory used by this map in bytes.
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NIP_ constexpr size_type memory_usage() const{
        return sizeof(*this) + _buckets.capacity() * sizeof(Entry) + _states.capacity() * sizeof(ull);
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // Iterators (only the main forward iterators you already have)
    // ─────────────────────────────────────────────────────────────────────────────
    SP_FORCEINLINE constexpr iterator begin() noexcept {
        iterator it(_buckets.data(), _states.data(), 0, _buckets.size(), (load_factor() < 0.3));
        if (_count > 0 && !get_state(0)) ++it;
        return it;
    }

    SP_FORCEINLINE constexpr const_iterator begin() const noexcept {
        const_iterator it(_buckets.data(), _states.data(), 0, _buckets.size(), (load_factor() < 0.3));
        if (_count > 0 && !get_state(0)) ++it;
        return it;
    }
    SP_FORCEINLINE constexpr const_iterator cbegin() const noexcept{
        const_iterator it(_buckets.data(), _states.data(), 0, _buckets.size(), (load_factor() < 0.3));
        if (_count > 0 && !get_state(0)) ++it;
        return it;
    }

    SP_FORCEINLINE constexpr iterator end() noexcept{
        return iterator(_buckets.data(), _states.data(), _buckets.size(), _buckets.size(), ((load_factor()<0.3)?true:false));
    }
    SP_FORCEINLINE constexpr const_iterator end() const noexcept{
        return const_iterator(_buckets.data(), _states.data(), _buckets.size(), _buckets.size(), ((load_factor()<0.3)?true:false));
    }
    SP_FORCEINLINE constexpr const_iterator cend() const noexcept{
        return const_iterator(_buckets.data(), _states.data(), _buckets.size(), _buckets.size(), ((load_factor()<0.3)?true:false));
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // Modifiers
    // ─────────────────────────────────────────────────────────────────────────────
    /// Resets the map to an empty 16-bucket state.
    SP_COLD SP_FORCEINLINE constexpr void clear() noexcept{
        _buckets = sp::array<Entry,0,Allocator>(16);
        _states = sp::array<ull,0,Allocator>(1);
        init_buckets();
    }

    /// Inserts all entries in the range `[first, last)`.
    template <short safety = _safety_level, class InputIt>
    SP_FLATTEN
    constexpr void insert(InputIt first, InputIt last){
        for(; first != last; ++first){
            auto&& kv = *first;
            insert<safety>(kv.first, kv.second);
        }
    }

    /// Inserts all entries from an initializer list.
    template <short safety = _safety_level, class P = sp::pair<Key,Value>>
    SP_FLATTEN
    constexpr void insert(std::initializer_list<P> ilist){
        for(auto& i : ilist){
            insert<safety>(i.first,i.second);
        }
    }

    /// Fast insert — skips the duplicate-key return value; no iterator is returned.
    _SP_SAFETY_TEMPLATE_
    SP_FLATTEN SP_CONSTEXPR23 void insert_fast(const Key& key, const Value& value){
        __sp_start:
        _SP_PROBE_LOOP_(
            return;
        );
        _SP_SIZE_REALLOC_CHECK_(insert_fast, key, value);
        _count++;
        set_state(probe);
    }

    /// Inserts @p key / @p value. Returns `{iterator, true}` on insertion, `{iterator, false}` if key already existed.
    _SP_SAFETY_TEMPLATE_
    SP_FLATTEN SP_CONSTEXPR23 pair<iterator, bool> insert(const Key& key, const Value& value){
        return insert_as<Key, safety>(key, value);
    }

    _SP_SAFETY_TEMPLATE_
    SP_FLATTEN constexpr pair<iterator, bool> insert(const Key& key, Value&& value){
        return insert_as<Key, safety>(key, sp::forward<Value>(value));
    }

    /// Heterogeneous insert — accepts any key type @p K comparable to `Key`.
    template <typename K, short safety=_safety_level>
    SP_CONSTEXPR23 pair<iterator, bool> insert_as(const K& key, const Value& value){
        __sp_start:
        _SP_PROBE_LOOP_(
            return {iterator(_buckets.data(), _states.data(), probe, _buckets.size(), (load_factor()<0.3)?true:false), 0};
        );
        _SP_SIZE_REALLOC_CHECK_(insert_as, key, value);
        _count++;
        set_state(probe);
        return {iterator(_buckets.data(), _states.data(), probe, _buckets.size(), (load_factor()<0.3)?true:false), 1};
    }

    template <typename K, short safety=_safety_level>
    SP_CONSTEXPR23 pair<iterator, bool> insert_as(const K& key, Value&& value){
        __sp_start:
        _SP_PROBE_LOOP_(
            return {iterator(_buckets.data(), _states.data(), probe, _buckets.size(), (load_factor()<0.3)?true:false), 0};
        )
        _SP_SIZE_REALLOC_CHECK_(insert_as, key, sp::move(value));
        _count++;
        set_state(probe);
        return {iterator(_buckets.data(), _states.data(), probe, _buckets.size(), (load_factor()<0.3)?true:false), 1};
    }

    /// Inserts or overwrites the value for @p key. Returns `{iterator, true}` on insert, `{iterator, false}` on assign.
    _SP_SAFETY_TEMPLATE_
    SP_FLATTEN constexpr pair<iterator, bool> insert_or_assign(const Key& key, const Value& obj) {
        return insert_or_assign_as<Key, safety>(key, obj);
    }

    _SP_SAFETY_TEMPLATE_
    SP_FLATTEN constexpr pair<iterator, bool> insert_or_assign(Key&& key, const Value& obj) {
        return insert_or_assign_as<Key, safety>(sp::forward<Key>(key), obj);
    }

    _SP_SAFETY_TEMPLATE_
    SP_FLATTEN constexpr pair<iterator, bool> insert_or_assign(const Key& key, Value&& obj) {
        return insert_or_assign_as<Key, safety>(key, sp::forward<Value>(obj));
    }

    _SP_SAFETY_TEMPLATE_
    SP_FLATTEN constexpr pair<iterator, bool> insert_or_assign(Key&& key, Value&& obj) {
        return insert_or_assign_as<Key, safety>(sp::forward<Key>(key), sp::forward<Value>(obj));
    }

    template <class K, short safety = _safety_level>
    SP_CONSTEXPR23 pair<iterator, bool> insert_or_assign_as(const K& key, const Value& value){
        __sp_start:
        _SP_PROBE_LOOP_(
            _buckets[probe].second = value;
            return {iterator(_buckets.data(), _states.data(), probe, cap, (load_factor()<0.3)?true:false), 0};
        );
        _SP_SIZE_REALLOC_CHECK_(insert_or_assign_as, key, value);
        _count++;
        set_state(probe);
        return {iterator(_buckets.data(), _states.data(), probe, cap, (load_factor()<0.3)?true:false), 1};
    }

    template <class K, short safety = _safety_level>
    SP_CONSTEXPR23 pair<iterator, bool> insert_or_assign_as(K&& key, const Value& value){
        __sp_start:
        _SP_PROBE_LOOP_(
            _buckets[probe].second = value;
            return {iterator(_buckets.data(), _states.data(), probe, cap, (load_factor()<0.3)?true:false), 0};
        );
        _SP_SIZE_REALLOC_CHECK_(insert_or_assign_as, sp::move(key), value);
        _count++;
        set_state(probe);
        return {iterator(_buckets.data(), _states.data(), probe, cap, (load_factor()<0.3)?true:false), 1};
    }

    template <class K, short safety = _safety_level>
    SP_CONSTEXPR23 pair<iterator, bool> insert_or_assign_as(const K& key, Value&& value){
        __sp_start:
        _SP_PROBE_LOOP_(
            _buckets[probe].second = value;
            return {iterator(_buckets.data(), _states.data(), probe, cap, (load_factor()<0.3)?true:false), 0};
        );
        _SP_SIZE_REALLOC_CHECK_(insert_or_assign_as, key, sp::move(value));
        _count++;
        set_state(probe);
        return {iterator(_buckets.data(), _states.data(), probe, cap, (load_factor()<0.3)?true:false), 1};
    }

    template <class K, short safety = _safety_level>
    SP_CONSTEXPR23 pair<iterator, bool> insert_or_assign_as(K&& key, Value&& value){
        __sp_start:
        _SP_PROBE_LOOP_(
            _buckets[probe].second = value;
            return {iterator(_buckets.data(), _states.data(), probe, cap, (load_factor()<0.3)?true:false), 0};
        );
        _SP_SIZE_REALLOC_CHECK_(insert_or_assign_as, sp::move(key), sp::move(value));
        _count++;
        set_state(probe);
        return {iterator(_buckets.data(), _states.data(), probe, cap, (load_factor()<0.3)?true:false), 1};
    }

    /// Constructs the value in-place from @p args if @p key is absent.
    template <short safety=_safety_level, class... Args>
    constexpr pair<iterator, bool> emplace(const Key& key, Args&&... args){
        return emplace_as<Key, safety, Args...>(key, sp::forward<Args>(args)...);
    }

    template <short safety = _safety_level, class... Args>
    constexpr pair<iterator, bool> emplace(Key&& key, Args&&... args){
        return emplace_as<Key, safety, Args...>(sp::forward<Key>(key), sp::forward<Args>(args)...);
    }

    template <typename K, short safety=_safety_level, class... Args>
    SP_CONSTEXPR23 pair<iterator, bool> emplace_as(const K& key, Args&&... args){
        __sp_start:
        _SP_PROBE_LOOP_(
            return { iterator(_buckets.data(), _states.data(), probe, _buckets.size(), (load_factor()<0.3)?true:false), false };
        );
        _SP_SIZE_REALLOC_CHECK_(emplace_as, key, Value(sp::forward<Args>(args)...));
        _count++;
        set_state(probe);
        return { iterator(_buckets.data(), _states.data(), probe, _buckets.size(), (load_factor()<0.3)?true:false), true };
    }

    template <typename K, short safety=_safety_level, class... Args>
    SP_CONSTEXPR23 pair<iterator, bool> emplace_as(K&& key, Args&&... args){
        __sp_start:
        _SP_PROBE_LOOP_(
            return { iterator(_buckets.data(), _states.data(), probe, _buckets.size(), (load_factor()<0.3)?true:false), false };
        );
        _SP_SIZE_REALLOC_CHECK_(emplace_as, sp::move(key), Value(sp::forward<Args>(args)...));
        _count++;
        set_state(probe);
        return { iterator(_buckets.data(), _states.data(), probe, _buckets.size(), (load_factor()<0.3)?true:false), true };
    }

    /// Erases the entry pointed to by @p pos and returns an iterator to the next occupied bucket.
    SP_FLATTEN constexpr iterator erase(const_iterator pos){
        size_type idx = pos._index;
        erase(_buckets[idx].first);
        iterator it(_buckets.data(), _states.data(), idx, _buckets.size(), (load_factor()<0.3));
        if(!get_state(idx)) ++it;
        return it;
    }

    /// Erases the entry with @p key. Returns `true` if an entry was removed.
    _SP_SAFETY_TEMPLATE_
    SP_HOT _SP_FUNC_FI_ SP_CONSTEXPR23 bool erase(const Key& key){
        const size_type cap = _buckets.size();
        const size_type mask = cap - 1;
        size_type hole=0;
        SP_IF_CONSTEXPR(hash_with_cap) hole = (Hash()(key, cap) & mask);
        else hole = (Hash()(key) & mask);
        while(get_state(hole)){
            if(_buckets[hole].first==key) goto __found;
            hole = (hole + 1) & mask;
            prefetch_info(hole, mask);
        }
        return 0;
        __found:
        // Find last valid element in the subcluster, swap with the hole; keeps chain intact and prioritizes newly inserted elements
        _count--;
        size_type probe = hole;
        clear_state(hole);
        for (;;) {
            size_type best_candidate = hole;
            size_type probe = hole;
            for (;;) { // Scan the entire cluster to find the LAST valid element for this hole
                probe = (probe + 1) & mask;
                prefetch_info(probe, mask);
                SP_IF_NOT_EXPECT(!get_state(probe)) goto __done;
                size_type ideal = (Hash()(_buckets[probe].first, cap) & mask);
                if (((probe - ideal + cap) & mask) >= ((probe - hole + cap) & mask)) { // is at or before the hole?
                    best_candidate = probe;
                }
            }
            __done:
            SP_IF_NOT_EXPECT(best_candidate == hole) return 1; // No candidate found, all done
            SP_IF_CONSTEXPR(spt::is_trivially_copyable_v<Key>&&spt::is_trivially_copyable_v<Value>) _buckets[hole] = _buckets[best_candidate];
            else _buckets[hole] = sp::move(_buckets[best_candidate]);
            clear_state(best_candidate);
            set_state(hole);

            // The old position of the candidate is the new hole; repeat
            hole = best_candidate;
        }
    }

    /// Erases all entries in `[first, last)`.
    _SP_SAFETY_TEMPLATE_
    SP_FLATTEN constexpr iterator erase(const_iterator first, const_iterator last){
        while(first!=last){
            erase(_buckets[first._index].first);
            if(!get_state(first._index)) ++first;
        }
    }

    /// Swaps contents with @p other in O(1).
    _SP_SAFETY_TEMPLATE_
    constexpr void swap(hash_map& other){
        using std::swap;
        swap(_buckets, other._buckets);
        swap(_states, other._states);
        swap(_count, other._count);
        swap(_max_count, other._max_count);
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // Lookup
    // ─────────────────────────────────────────────────────────────────────────────
    /// Returns a reference to the value for @p key; throws `MapException` if not found.
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NI_ SP_HOT constexpr Value& at(const Key& key){
        _SP_PROBE_LOOP_(
            return _buckets[probe].second;
        );
        throw exceptions::MapException("No value found in function: at()");
    }
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NIFH_ constexpr const Value& at(const Key& key) const{
        return const_cast<Value&>(at<safety>(key));
    }

    /// Returns a reference to the value for @p key, inserting a default-constructed value if absent.
    template <typename K>
    SP_CONSTEXPR23 Value& operator[](const K& key){
        __sp_start:
        constexpr short safety = _safety_level;
        _SP_PROBE_LOOP_(
            return _buckets[probe].second;
        );
        _SP_SIZE_REALLOC_CHECK_(operator[], key, Value());
        _count++;
        set_state(probe);
        return _buckets[probe].second;
    }
    template <typename K>
    SP_NODISCARD SP_HOT SP_CONSTEXPR23 Value& operator[](K&& key){
        __sp_start:
        constexpr short safety = _safety_level;
        _SP_PROBE_LOOP_(
            return _buckets[probe].second;
        );
        _SP_SIZE_REALLOC_CHECK_(operator[], sp::move(key), Value());
        _count++;
        set_state(probe);
        return _buckets[probe].second;
    }

    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NIP_ constexpr size_type count(const Key& key) const{
        _SP_PROBE_LOOP_(return 1;);
        return 0;
    }
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NI_ constexpr iterator find(const Key& key){
        _SP_PROBE_LOOP_(
            return iterator(_buckets.data(), _states.data(), probe, _buckets.size(), (load_factor()<0.3)?true:false);
        );
        return end();
    }
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NIP_ constexpr const_iterator find(const Key& key) const{
        _SP_PROBE_LOOP_(
            return const_iterator(_buckets.data(), _states.data(), probe, _buckets.size(), (load_factor()<0.3)?true:false);
        );
        return cend();
    }

    template <typename K, short safety = _safety_level>
    _SP_FUNC_NI_ constexpr iterator find(const K& key){
        _SP_PROBE_LOOP_(
            return iterator(_buckets.data(), _states.data(), probe, _buckets.size(), (load_factor()<0.3)?true:false);
        );
        return end();
    }
    template <typename K, short safety = _safety_level>
    _SP_FUNC_NIP_ constexpr const_iterator find(const K& key) const{
        _SP_PROBE_LOOP_(
            return const_iterator(_buckets.data(), _states.data(), probe, _buckets.size(), (load_factor()<0.3)?true:false);
        );
        return cend();
    }
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_NIP_ constexpr bool contains(const Key& key) const{
        return count(key) != 0;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // Safe & convenient access (highly requested)
    // ─────────────────────────────────────────────────────────────────────────────
    _SP_SAFETY_TEMPLATE_
    SP_NODISCARD _SP_FUNC_FI_ constexpr Value get(const Key& key, const Value& default_value = {}) const{
        return get_as<Key, safety>(key, default_value);
    }
    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_FI_ constexpr bool try_get(const Key& key, Value& out) const{
        return try_get_as<Key, safety>(key, out);
    }

    template <typename K, short safety = _safety_level>
    SP_NODISCARD SP_FORCEINLINE SP_CONSTEXPR23 Value get_as(const K& key, const Value& default_value = {}) const{
        __sp_start:
        _SP_PROBE_LOOP_(return _buckets[probe].second;);
        return default_value;
    }

    template <typename K, short safety = _safety_level>
    SP_NODISCARD SP_FORCEINLINE constexpr bool try_get_as(const K& key, Value& out) const{
        size_type cap = _buckets.size();
        _SP_CHECK_SAFETY_(1) SP_IF_NOT_EXPECT(cap==0) return false;
        const size_type mask = cap - 1;
        size_type probe=0;
        SP_IF_CONSTEXPR(hash_with_cap) probe = (Hash()(key, cap) & mask);
        else probe = (Hash()(key) & mask);
        prefetch_info(probe, mask);
        while(get_state(probe)){
            if(_buckets[probe].first==key){
                out = _buckets[probe].second;
                return true;
            }
            probe = (probe + 1) & mask;
            prefetch_info(probe, mask);
        }
        return false;
    }

    _SP_SAFETY_TEMPLATE_
    _SP_FUNC_FI_ constexpr iterator find_or_insert(const Key& key, const Value& default_val){
        return insert_as<Key, safety>(key, default_val).first;
    }
    template <short safety = _safety_level, typename... Args>
    _SP_FUNC_FI_ constexpr iterator find_or_emplace(const Key& key, Args&&... args){
        return emplace_as<Key, safety>(key, sp::forward<Args>(args)...).first;
    }

    template <typename K, short safety = _safety_level>
    _SP_FUNC_FI_ constexpr iterator find_or_insert_as(const K& key, const Value& default_val){
        return insert_as<K, safety>(key, default_val).first;
    }
    template <typename K, short safety = _safety_level, typename... Args>
    _SP_FUNC_FI_ constexpr iterator find_or_emplace_as(const K& key, Args&&... args){
        return emplace_as<K, safety>(sp::forward<K>(key), sp::forward<Args>(args)...).first;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // Collection views / copies (very common requests)
    // ─────────────────────────────────────────────────────────────────────────────
    SP_FORCEINLINE constexpr sp::array<Key> keys() const{
        sp::array<Key> result;
        result.reserve(_count);
        for(const_entry_loop_type e : *this){
            result.template push_back<0>(e.first);
        }
        return result;
    }
    SP_FORCEINLINE constexpr sp::array<Value> values() const{
        sp::array<Value> result;
        result.reserve(_count);
        for(const_entry_loop_type e : *this){
            result.template push_back<0>(e.second);
        }
        return result;
    }
    SP_FORCEINLINE constexpr sp::array<sp::pair<const Key, Value>> items() const{
        sp::array<sp::pair<const Key, Value>> result;
        result.reserve(_count);
        for(const_entry_loop_type e : *this){
            result.template push_back<0>(sp::pair<const Key, Value>(e.first, e.second));
        }
        return result;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // Range-based modifiers & algorithms
    // ─────────────────────────────────────────────────────────────────────────────
    template <class Pred>
    SP_FORCEINLINE constexpr size_type erase_if(Pred&& pred) {
        size_type sum = 0;
        auto it = begin();
        while (it != end()) {
            if (pred(*it)) {                    // *it gives Entry&
                it = erase(it);                 // erase returns the next valid iterator
                ++sum;
            } else {
                ++it;
            }
        }
        return sum;
    }

    template <class UnaryFunc> 
    SP_FORCEINLINE constexpr void for_each(UnaryFunc&& func){
        for(entry_loop_type e : *this){
            func(e);
        }
    }

    template <class UnaryFunc> 
    SP_FORCEINLINE constexpr void for_each_const(UnaryFunc&& func) const{
        for(const_entry_loop_type e : *this){
            func(e);
        }
    }

    template <class UnaryFunc>
    SP_FORCEINLINE constexpr void for_each_key(UnaryFunc&& func){
        for(entry_loop_type e : *this){
            func(e.first);
        }
    }

    template <class UnaryFunc>
    SP_FORCEINLINE constexpr void for_each_value(UnaryFunc&& func){
        for(entry_loop_type e : *this){
            func(e.second);
        }
    }

    template <class UnaryFunc>
    SP_FORCEINLINE constexpr void for_each_key_const(UnaryFunc&& func) const{
        for(const_entry_loop_type e : *this){
            func(e.first);
        }
    }

    template <class UnaryFunc>
    SP_FORCEINLINE constexpr void for_each_value_const(UnaryFunc&& func) const{
        for(const_entry_loop_type e : *this){
            func(e.second);
        }
    }

    template <typename Pred>
    SP_FORCEINLINE constexpr hash_map filter(Pred pred) const{
        hash_map<Key, Value, _safety_level, _threshold, Hash, Allocator> result;
        for(const_entry_loop_type e : *this){
            if(pred(e)) result.insert(e.first,e.second);
        }
        return result;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // Miscellaneous / nice-to-have
    // ─────────────────────────────────────────────────────────────────────────────
        // ─────────────────────────────────────────────────────────────────────────────
    // Comparison
    // ─────────────────────────────────────────────────────────────────────────────
    SP_FORCEINLINE constexpr bool operator==(const hash_map& other) const {
        if (this == &other) return true;
        if (_count != other._count) return false;
        if (_buckets.size() != other._buckets.size()) return false;

        return (_buckets.template equals<0>(other._buckets) &&
                _states == other._states &&
                _count == other._count &&
                _max_count == other._max_count);
    }

    SP_FORCEINLINE constexpr bool operator!=(const hash_map& other) const {
        return !(*this == other);
    }

    constexpr hash_map clone() const{
        hash_map result;
        result._buckets = _buckets.clone();
        result._count = _count;
        result._max_count = _max_count;
        result._states = _states.clone();
        return result;
    }


    // ─────────────────────────────────────────────────────────────────────────────
    // Additional lookup / access variations
    // ─────────────────────────────────────────────────────────────────────────────
    constexpr Value& value_or(const Key& key, Value& fallback){
        const size_type cap = _buckets.size();
        const size_type mask = cap - 1;
        size_type probe=0;
        SP_IF_CONSTEXPR(hash_with_cap) probe = (Hash()(key, cap) & mask);
        else probe = (Hash()(key) & mask);
        prefetch_info(probe, mask);
        while(get_state(probe)){
            if(_buckets[probe].first==key){
                return _buckets[probe].second;
            }
            probe = (probe + 1) & mask;
            prefetch_info(probe, mask);
        }
        return fallback;
    }
    constexpr const Value& value_or(const Key& key, const Value& fallback) const{
        const size_type cap = _buckets.size();
        const size_type mask = cap - 1;
        size_type probe=0;
        SP_IF_CONSTEXPR(hash_with_cap) probe = (Hash()(key, cap) & mask);
        else probe = (Hash()(key) & mask);
        prefetch_info(probe, mask);
        while(get_state(probe)){
            if(_buckets[probe].first==key){
                return _buckets[probe].second;
            }
            probe = (probe + 1) & mask;
            prefetch_info(probe, mask);
        }
        return fallback;
    }

    constexpr Value value_or(Key&& key, Value&& fallback){
        const size_type cap = _buckets.size();
        const size_type mask = cap - 1;
        size_type probe=0;
        SP_IF_CONSTEXPR(hash_with_cap) probe = (Hash()(key, cap) & mask);
        else probe = (Hash()(key) & mask);
        prefetch_info(probe, mask);
        while(get_state(probe)){
            if(_buckets[probe].first==key){
                return _buckets[probe].second;
            }
            probe = (probe + 1) & mask;
            prefetch_info(probe, mask);
        }
        return fallback;
    }

    template <typename K, short safety = _safety_level>
    SP_CONSTEXPR23 Value& at_or_insert_as(const K& key, const Value& value){
        __sp_start:
        _SP_PROBE_LOOP_(
            return _buckets[probe].second;
        );
        _SP_SIZE_REALLOC_CHECK_(at_or_insert, key, value);
        _count++;
        set_state(probe);
        return _buckets[probe].second;
    }
    template <typename K, short safety = _safety_level>
    SP_CONSTEXPR23 Value& at_or_insert_as(K&& key, Value&& value){
        __sp_start:
        _SP_PROBE_LOOP_(
            return _buckets[probe].second;
        );
        _SP_SIZE_REALLOC_CHECK_(at_or_insert, sp::move(key), sp::move(value));
        _count++;
        set_state(probe);
        return _buckets[probe].second;
    }
    template <typename K, short safety = _safety_level>
    SP_CONSTEXPR23 Value& at_or_insert_as(const K& key, Value&& value){
        __sp_start:
        _SP_PROBE_LOOP_(
            return _buckets[probe].second;
        );
        _SP_SIZE_REALLOC_CHECK_(at_or_insert, key, sp::move(value));
        _count++;
        set_state(probe);
        return _buckets[probe].second;
    }
    template <typename K, short safety = _safety_level>
    SP_CONSTEXPR23 Value& at_or_insert_as(K&& key, const Value& value){
        __sp_start:
        _SP_PROBE_LOOP_(
            return _buckets[probe].second;
        );
        _SP_SIZE_REALLOC_CHECK_(at_or_insert, sp::move(key), value);
        _count++;
        set_state(probe);
        return _buckets[probe].second;
    }

    _SP_SAFETY_TEMPLATE_
    SP_FLATTEN constexpr Value& at_or_insert(const Key& key, const Value& value){
        return at_or_insert_as<Key, safety>(key, value);
    }
    _SP_SAFETY_TEMPLATE_
    SP_FLATTEN constexpr Value& at_or_insert(Key&& key, Value&& value){
        return at_or_insert_as<Key, safety>(sp::forward<Key>(key), sp::forward<Value>(value));
    }
    _SP_SAFETY_TEMPLATE_
    SP_FLATTEN constexpr Value& at_or_insert(const Key& key, Value&& value){
        return at_or_insert_as<Key, safety>(key, sp::forward<Value>(value));
    }
    _SP_SAFETY_TEMPLATE_
    SP_FLATTEN constexpr Value& at_or_insert(Key&& key, const Value& value){
        return at_or_insert_as<Key, safety>(sp::forward<Key>(key), value);
    }


    template <typename K, short safety = _safety_level, class... Args>
    SP_FORCEINLINE SP_CONSTEXPR23 pair<iterator, bool> try_emplace_as(const K& key, Args&&... args){
        __sp_start:
        _SP_PROBE_LOOP_(
            return {iterator(_buckets.data(), _states.data(), probe, _buckets.size(), (load_factor()<0.3)?true:false), false};
        );
        SP_IF_NOT_EXPECT(!would_fit_without_rehash(1)) return {end(), false};
        _count++;
        set_state(probe);
        _buckets[probe] = Entry(key, Value(sp::forward<Args>(args)...));
        return {iterator(_buckets.data(), _states.data(), probe, _buckets.size(), (load_factor()<0.3)?true:false), true};
    }

    template <typename K, short safety = _safety_level, class... Args>
    SP_FORCEINLINE SP_CONSTEXPR23 pair<iterator, bool> try_emplace_as(K&& key, Args&&... args){
        __sp_start:
        _SP_PROBE_LOOP_(
            return {iterator(_buckets.data(), _states.data(), probe, _buckets.size(), (load_factor()<0.3)?true:false), false};
        );
        
        SP_IF_NOT_EXPECT(!would_fit_without_rehash(1)) {
            rehash(_buckets.size() * 2); // (Or whatever your growth factor logic is)
            goto __sp_start; 
        }
        
        _count++;
        set_state(probe);
        _buckets[probe] = Entry(sp::move(key), Value(sp::forward<Args>(args)...));
        return {iterator(_buckets.data(), _states.data(), probe, _buckets.size(), (load_factor()<0.3)?true:false), true};
    }

    template <short safety = _safety_level, class... Args>
    _SP_FUNC_FI_ constexpr pair<iterator, bool> try_emplace(const Key& key, Args&&... args){
        return try_emplace_as<Key, safety, Args...>(key, sp::forward<Args>(args)...);
    }

    template <short safety = _safety_level, class... Args>
    _SP_FUNC_FI_ constexpr pair<iterator, bool> try_emplace(Key&& key, Args&&... args){
        return try_emplace_as<Key, safety, Args...>(sp::forward<Key>(key), sp::forward<Args>(args)...);
    }

    template <typename K, short safety = _safety_level>
    SP_FORCEINLINE SP_CONSTEXPR23 Value& lazy_emplace_as(const K& key){
        __sp_start:
        _SP_PROBE_LOOP_(
            return _buckets[probe].second;
        );
        SP_IF_NOT_EXPECT(!would_fit_without_rehash(1)) throw exceptions::MapException("Insertion failed in lazy_emplace_as: insufficient capacity and reallocation is disabled");
        _count++;
        set_state(probe);
        new(&_buckets[probe].second) Value(); // default-construct in-place
        _buckets[probe].first = key; // assign key after value is safely constructed
        return _buckets[probe].second;
    }   // default-construct if absent
    template <typename K, short safety = _safety_level>
    SP_FORCEINLINE SP_CONSTEXPR23 Value& lazy_emplace_as(K&& key){
        __sp_start:
        _SP_PROBE_LOOP_(
            return _buckets[probe].second;
        );
        SP_IF_NOT_EXPECT(!would_fit_without_rehash(1)) throw exceptions::MapException("Insertion failed in lazy_emplace_as: insufficient capacity and reallocation is disabled");
        _count++;
        set_state(probe);
        new(&_buckets[probe].second) Value(); // default-construct in-place
        _buckets[probe].first = sp::move(key); // assign key after value is safely constructed
        return _buckets[probe].second;
    }

    template <typename K, short safety = _safety_level>
    SP_FORCEINLINE constexpr Value& lazy_emplace(const K& key){
        return lazy_emplace_as<K, safety>(key);
    }
    template <typename K, short safety = _safety_level>
    SP_FORCEINLINE constexpr Value& lazy_emplace(K&& key){
        return lazy_emplace_as<K, safety>(sp::forward<K>(key));
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // More modifier / update variations
    // ─────────────────────────────────────────────────────────────────────────────
    template <typename K, short safety = _safety_level>
    SP_FORCEINLINE SP_FLATTEN constexpr pair<iterator, bool> insert_default_as(const K& key){
        return insert_as<K, safety>(key, Value());
    }
    template <typename K, short safety = _safety_level>
    SP_FORCEINLINE SP_FLATTEN constexpr pair<iterator, bool> insert_default_as(K&& key){
        return insert_as<K, safety>(sp::forward<Key>(key), Value());
    }


    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE SP_FLATTEN constexpr pair<iterator, bool> insert_default(const Key& key){
        return insert_as<Key, safety>(key, Value());
    }
    _SP_SAFETY_TEMPLATE_
    SP_FORCEINLINE SP_FLATTEN constexpr pair<iterator, bool> insert_default(Key&& key){
        return insert_as<Key, safety>(sp::forward<Key>(key), Value());
    }

    SP_FORCEINLINE constexpr bool replace(const Key& key, const Value& new_value){
        auto it = find(key);
        SP_IF_EXPECT(it != end()){
            it->second = new_value;
            return true;
        }
        else{
            return false;
        }
    }
    SP_FORCEINLINE constexpr bool replace(const Key& key, Value&& new_value){
        auto it = find(key);
        SP_IF_EXPECT(it != end()){
            it->second = sp::move(new_value);
            return true;
        }
        else{
            return false;
        }
    }

    constexpr size_type erase_if_value_matches(const Value& value_to_match){
        size_type sum = 0;
        for(entry_loop_type e : *this){
            if(e.second == value_to_match){
                erase(e.first);
                sum++;
                }
        }
        return sum;
    }

    constexpr iterator erase_and_get_next(const_iterator pos){
        size_type idx = pos._index;
        erase(idx);
        iterator it(_buckets.data(), _states.data(), idx, _buckets.size(), (load_factor()<0.3)?true:false);
        ++it;
        return it;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // Bulk / batch operations
    // ─────────────────────────────────────────────────────────────────────────────

    SP_FORCEINLINE constexpr size_type insert_many(const Key* keys, const Value* values, size_type n){
        size_type inserted = 0;
        for(size_type i=0; i<n; i++){
            auto result = insert_as(keys[i], values[i]);
            if(result.second) inserted++;
        }
        return inserted;
    }
    SP_FORCEINLINE constexpr size_type insert_many(std::initializer_list<Key> keys, std::initializer_list<Value> values){
        SP_IF_NOT_EXPECT(keys.size() == values.size()) throw exceptions::MapException("Mismatched initializer list sizes in insert_many");
        size_type inserted = 0;
        auto key_it = keys.begin();
        auto val_it = values.begin();
        for(; key_it != keys.end() && val_it != values.end(); ++key_it, ++val_it){
            auto result = insert_as(*key_it, *val_it);
            if(result.second) inserted++;
        }
        return inserted;
    }

    _SP_FUNC_FI_ constexpr void reserve_at_least_for_ratio(size_type expected_elements, float desired_load_factor){
        size_type required_capacity = static_cast<size_type>(std::ceil(expected_elements / desired_load_factor));
        reserve(required_capacity);
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // Hash policy & probing introspection
    // ─────────────────────────────────────────────────────────────────────────────
    // ─────────────────────────────────────────────────────────────────────────────
    // Lazy insertion & map merging
    // ─────────────────────────────────────────────────────────────────────────────

    /// Returns the value for @p key if present; otherwise calls @p callable() to
    /// construct the value, inserts it, and returns a reference to the new entry.
    /// The callable is invoked at most once and only on a cache miss.
    /// @tparam K        Key type (may differ from `Key` for heterogeneous lookup).
    /// @tparam Callable Nullary callable returning `Value`.
    /// @param key      Key to look up or insert.
    /// @param callable Factory invoked as `callable()` to produce the value on miss.
    /// @return Reference to the existing or newly inserted value.
    template <typename K, short safety = _safety_level, typename Callable>
    SP_CONSTEXPR23 Value& get_or_insert_with_as(const K& key, Callable&& callable) {
        __sp_start:
        _SP_PROBE_LOOP_(
            return _buckets[probe].second;
        );
        _SP_SIZE_REALLOC_CHECK_(get_or_insert_with_as, key, callable());
        _count++;
        set_state(probe);
        return _buckets[probe].second;
    }

    /// @copydoc get_or_insert_with_as(const K&, Callable&&)
    template <typename K, short safety = _safety_level, typename Callable>
    SP_CONSTEXPR23 Value& get_or_insert_with_as(K&& key, Callable&& callable) {
        __sp_start:
        _SP_PROBE_LOOP_(
            return _buckets[probe].second;
        );
        _SP_SIZE_REALLOC_CHECK_(get_or_insert_with_as, sp::move(key), callable());
        _count++;
        set_state(probe);
        return _buckets[probe].second;
    }

    /// Thin wrapper over `get_or_insert_with_as` for the map's own `Key` type.
    /// @param key      Key to look up or insert.
    /// @param callable Factory invoked as `callable()` to produce the value on miss.
    /// @return Reference to the existing or newly inserted value.
    template <short safety = _safety_level, typename Callable>
    SP_FORCEINLINE constexpr Value& get_or_insert_with(const Key& key, Callable&& callable) {
        return get_or_insert_with_as<Key, safety>(key, sp::forward<Callable>(callable));
    }
    /// @copydoc get_or_insert_with(const Key&, Callable&&)
    template <short safety = _safety_level, typename Callable>
    SP_FORCEINLINE constexpr Value& get_or_insert_with(Key&& key, Callable&& callable) {
        return get_or_insert_with_as<Key, safety>(sp::forward<Key>(key), sp::forward<Callable>(callable));
    }

    /// Inserts all entries from @p other into this map, skipping keys that already exist.
    /// Equivalent to calling `insert` for every entry in @p other.
    /// @param other Source map to merge from; left unmodified.
    /// @return Number of entries actually inserted (duplicates are skipped).
    template <short safety = _safety_level>
    SP_FORCEINLINE constexpr size_type merge(const hash_map& other) {
        size_type inserted = 0;
        for (const_entry_loop_type e : other) {
            if (insert_as<Key, safety>(e.first, e.second).second) ++inserted;
        }
        return inserted;
    }

    /// Moves all entries from `*this` into @p other, skipping keys already present in @p other.
    /// Trivially-copyable entries are copied; non-trivial entries are move-assigned.
    /// `*this` is cleared after the operation regardless of how many entries were transferred.
    /// @param other Destination map to receive the entries.
    /// @return Number of entries actually moved (duplicates are skipped).
    template <short safety = _safety_level>
    SP_FORCEINLINE constexpr size_type merge_into(hash_map& other) {
        size_type inserted = 0;
        for (entry_loop_type e : *this) {
            SP_IF_CONSTEXPR(key_trivially_copyable && value_trivially_copyable) {
                if (other.template insert_as<Key, safety>(e.first, e.second).second) ++inserted;
            } else {
                if (other.template insert_as<Key, safety>(sp::move(e.first), sp::move(e.second)).second) ++inserted;
            }
        }
        clear();
        return inserted;
    }

    _SP_FUNC_FI_ constexpr bool would_fit_without_rehash(size_type additional_elements) const{
        return (_count + additional_elements) <= _max_count;
    }

    _SP_FUNC_FI_ constexpr ull hash_of(const Key& key) const{
        SP_IF_CONSTEXPR(hash_with_cap) return Hash()(key, _buckets.size());
        else return Hash()(key);
    }
    _SP_FUNC_FI_ constexpr size_type ideal_bucket_for_hash(ull h) const{
        const size_type cap = _buckets.size();
        const size_type mask = cap - 1;
        return h & mask;
    }
    _SP_FUNC_FI_ constexpr size_type ideal_bucket(const Key& key) const{
        return ideal_bucket_for_hash(hash_of(key));
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // Debugging / validation helpers
    // ─────────────────────────────────────────────────────────────────────────────
    constexpr bool states_match_occupied_count() const{
        size_type occupied = 0;
        for(size_type i=0; i<_states.size(); i++){
            if(get_state(i)) occupied++;
        }
        return occupied == _count;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // View / snapshot helpers
    // ─────────────────────────────────────────────────────────────────────────────
    constexpr sp::vector<sp::pair<Key, Value>> to_vector() const{
        sp::vector<sp::pair<Key, Value>> result;
        result.reserve(_count);
        for(const_entry_loop_type e : *this){
            result.emplace_back(e.first, e.second);
        }
        return result;
    }
    constexpr sp::array<sp::pair<Key,Value>> to_array() const{
        sp::array<sp::pair<Key, Value>> result;
        result.reserve(_count);
        for(const_entry_loop_type e : *this){
            result.template push_back<0>(sp::pair<Key, Value>(e.first, e.second));
        }
        return result;
    }
    constexpr sp::vector<sp::pair<Key,Value>> to_sp_vector() const{
        sp::vector<sp::pair<Key, Value>> result;
        result.reserve(_count);
        for(const_entry_loop_type e : *this){
            result.template push_back<0>(sp::pair<Key, Value>(e.first, e.second));
        }
        return result;
    }
    constexpr sp::array<sp::pair<const Key*, const Value*>> view_pairs() const{
        sp::array<sp::pair<const Key*, const Value*>> result;
        result.reserve(_count);
        for(const_entry_loop_type e : *this){
            result.template push_back<0>(sp::pair<const Key*, const Value*>(std::addressof(e.first), std::addressof(e.second)));
        }
        return result;

    }
}; /// class hash_map
namespace maps{
    //-Hash map with safety level 2
    //-Strong exception gurantees: catches failed ops and re-throws sp::exceptions::MapConstructException
    //-Slowest version
    template <typename Key, typename Value, int _threshold = 70, typename Hash = sp::basic_hash>
    using safe_map = hash_map<Key, Value, 2, _threshold, Hash>;

    //-Default hash map
    //-Safety level 1
    //-Normal safety
    //-Good balance between safety and speed
    template <typename Key, typename Value, int _threshold = 70, typename Hash = sp::basic_hash>
    using def_map = hash_map<Key,Value,1,_threshold,Hash>;

    // WARNING: Requires preconditionss such as reserving; reallocations are NOT automatic
    //-No reallocation checks: very fast
    //-Fails silently if space is insufficient
    template <typename Key, typename Value, int _threshold = 70, typename Hash = sp::basic_hash>
    using fast_map = hash_map<Key, Value, 0, _threshold, Hash>;

    // Customized before include or at build time by the user
    // Every template parameter is customized using the __SP_DEFAULT_MAP_TRAITS__ flag
    template <typename Key, typename Value>
    using hmap = hash_map<Key, Value, __SP_DEFAULT_MAP_TRAITS__>;

};

//-Hash map with safety level 2
//-Strong exception gurantees: catches failed ops and re-throws sp::exceptions::MapConstructException
//-Slowest version
template <typename Key, typename Value, int _threshold = 70, typename Hash = sp::basic_hash>
using safe_map = maps::safe_map<Key,Value,_threshold,Hash>;

//-Default hash map
//-Safety level 1
//-Normal safety
//-Good balance between safety and speed
template <typename Key, typename Value, int _threshold = 70, typename Hash = sp::basic_hash>
using def_map = maps::def_map<Key,Value,_threshold,Hash>;
// Alias for def_map
template <typename Key, typename Value, int _threshold = 70, typename Hash = sp::basic_hash>
using default_map = maps::def_map<Key,Value,_threshold,Hash>;

// WARNING: Requires preconditions such as reserving; reallocations are NOT automatic
//-No reallocation checks: very fast
//-Fails silently if space is insufficient
template <typename Key, typename Value, int _threshold = 70, typename Hash = sp::basic_hash>
using fast_map = maps::fast_map<Key, Value, _threshold, Hash>;

// Customized before include or at build time by the user
// Every template parameter is customized using the __SP_DEFAULT_MAP_TRAITS__ flag
template <typename Key, typename Value>
using hmap = hash_map<Key, Value, __SP_DEFAULT_MAP_TRAITS__>;



template <typename Key, typename Value, size_type N=1024, int _threshold=70, typename Hash = sp::basic_hash>
using stack_map = hash_map<Key,Value,1,_threshold,Hash,sp::bind_alloc<sp::unbinded_stack_allocator,N>::template r>;
template <typename Key, typename Value, size_type N=1024, int _threshold=70, typename Hash = sp::basic_hash>
using stack_umap = hash_map<Key,Value,0,_threshold,Hash,sp::bind_alloc<sp::unbinded_stack_allocator,N>::template r>;
} // namespace sp

#undef _SP_SAFETY_TEMPLATE_
#undef _SP_CHECK_SAFETY_
#undef _SP_SIZE_REALLOC_CHECK_
#undef _SP_PROBE_LOOP_

#endif