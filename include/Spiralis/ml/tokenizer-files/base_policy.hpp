#ifndef ____SP_BASE_POLICY____
#define ____SP_BASE_POLICY____
#pragma once
#include "../../io/IO.hpp"
#include "../../setup/init.hpp"
#include "../../containers/array.hpp"
#include "../../containers/hash_map.hpp"
#include "../../containers/pair.hpp"
#include "../../containers/string.hpp"
namespace sp_pol{
    template <typename T>
    class TokenizerPolicy{
    public:
        using s_t = T;
        SP_FORCEINLINE TokenizerPolicy() { }
        SP_FORCEINLINE virtual void build_mapping(const sp::string& text, ull vocab_size) {}
        SP_FORCEINLINE virtual void build_mapping_debug(const sp::string& text, ull vocab_size) { return build_mapping(text, vocab_size); }
        SP_FORCEINLINE virtual sp::vector<T> tokenize(const sp::string& text) const { return sp::vector<T>(); }
        SP_FORCEINLINE virtual sp::vector<sp::string> reconstructed(const sp::vector<T> tokens) const { return sp::vector<sp::string>(); }
        SP_FORCEINLINE virtual void to_file(const sp::string& filename) const{}
        SP_FORCEINLINE virtual void from_file(const sp::string& filename) {}
        SP_FORCEINLINE virtual sp::vector<sp::vector<T>> tokenize_batch(const sp::vector<sp::string>& texts, sp::thread_pool& pool) const{ return sp::vector<sp::vector<T>>(); }
    };
}
#endif