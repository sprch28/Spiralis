#if !defined(____SP_MALLOC____) && __has_include(<sys/mman.h>)
#define ____SP_MALLOC____
#include "../setup/init.hpp"
#include "../math/bit_manip.hpp"
#include "../math/math.hpp"
#include "../core/exceptions.hpp"


#include <sys/mman.h>
#include <cstddef>
#include <cstdint>

namespace sp {


#define _SP_MIN_BYTE_ALIGNMENT_ 8
#define SP_ALIGN_UP(val) (((val) + (_SP_MIN_BYTE_ALIGNMENT_ - 1)) & ~(_SP_MIN_BYTE_ALIGNMENT_ - 1))

constexpr size_type ARENA_SIZE = 4 * 1024 * 1024; // 4 MB chunk size
constexpr size_type NUM_BUCKETS = 12;             // Sizes up to ~16 KB (8, 16, 32... 16384)

struct FreeNode {
    FreeNode* next;
};

inline thread_local char* arena_start = nullptr;
inline thread_local size_type arena_remaining = 0;
inline thread_local FreeNode* free_buckets[NUM_BUCKETS] = { nullptr };

inline int get_bucket_index(size_type size) {
    size_type alloc_size = SP_ALIGN_UP(size);
    int index = 0;
    size_type bucket_size = _SP_MIN_BYTE_ALIGNMENT_;
    while (bucket_size < alloc_size && index < NUM_BUCKETS - 1) {
        bucket_size <<= 1;
        index++;
    }
    return index;
}

inline size_type get_bucket_size(int index) {
    return _SP_MIN_BYTE_ALIGNMENT_ << index;
}

inline char* allocate_arena(size_type size){
    void* ptr = mmap(
        nullptr,
        size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );
    SP_IF_NOT_EXPECT(ptr == MAP_FAILED) return nullptr;
    return static_cast<char*>(ptr);
}

void* malloc(size_type num_bytes){
    SP_IF_NOT_EXPECT(num_bytes == 0) return nullptr;

    size_type total_bytes = SP_ALIGN_UP(num_bytes + sizeof(size_type));

    // Bypass arena for large allocations
    if(total_bytes > get_bucket_size(NUM_BUCKETS - 1)){
        char* raw = allocate_arena(total_bytes);
        SP_IF_NOT_EXPECT(!raw) return nullptr;
        *reinterpret_cast<size_type*>(raw) = total_bytes;
        return raw + sizeof(size_type);
    }

    int bucket_idx = get_bucket_index(total_bytes);
    size_type slot_size = get_bucket_size(bucket_idx);

    if(free_buckets[bucket_idx] != nullptr){
        FreeNode* node = free_buckets[bucket_idx];
        free_buckets[bucket_idx] = node->next;
        
        char* raw = reinterpret_cast<char*>(node);
        *reinterpret_cast<size_type*>(raw) = slot_size;
        return raw + sizeof(size_type);
    }

    if(arena_remaining < slot_size){
        size_type new_arena_size = (slot_size > ARENA_SIZE) ? slot_size : ARENA_SIZE;
        arena_start = allocate_arena(new_arena_size);
        SP_IF_NOT_EXPECT(!arena_start) return nullptr;
        arena_remaining = new_arena_size;
    }

    char* raw = arena_start;
    arena_start += slot_size;
    arena_remaining -= slot_size;

    *reinterpret_cast<size_type*>(raw) = slot_size;
    return raw + sizeof(size_type);
}

void free(void* ptr){
    SP_IF_NOT_EXPECT(!ptr) return;

    char* raw = static_cast<char*>(ptr) - sizeof(size_type);
    size_type block_size = *reinterpret_cast<size_type*>(raw);

    if(block_size > get_bucket_size(NUM_BUCKETS - 1)){
        munmap(raw, block_size);
        return;
    }

    int bucket_idx = get_bucket_index(block_size);
    FreeNode* node = reinterpret_cast<FreeNode*>(raw);
    node->next = free_buckets[bucket_idx];
    free_buckets[bucket_idx] = node;
}

} // namespace sp

#endif