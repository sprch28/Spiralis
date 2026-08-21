#ifndef ____SP_TOKENIZER_HPP____
#define ____SP_TOKENIZER_HPP____
#pragma once
#include "policies.hpp"
#include <cstdint>
#include <cstring>
namespace sp{
template <ull vocab_size = 25'000, class policy = sp_pol::greedy_subword_tokenizer<2,4,uint32_t>>
class tokenizer{
private:
    policy p;
public:
    tokenizer() : p(policy()) {}
    SP_FORCEINLINE void build_mapping(const sp::string& text) { p.build_mapping(text, vocab_size); }
    SP_FORCEINLINE void build_mapping_debug(const sp::string& text) { p.build_mapping_debug(text, vocab_size); }
    SP_FORCEINLINE const auto& get_mapping() { return p.mapping(); }
    SP_FORCEINLINE sp::vector<typename policy::s_t> tokenize(const sp::string& text) const {  return p.tokenize(text); }
    SP_FORCEINLINE sp::vector<sp::string> reconstructed(const sp::vector<typename policy::s_t> tokens) const { return p.reconstructed(tokens); }
    SP_FORCEINLINE sp::string reconstructed_string(const sp::vector<typename policy::s_t> tokens) const { return ""_sp.join(reconstructed(tokens)); }
    SP_FORCEINLINE void to_file(const sp::string& filename) const { p.to_file(filename); }
    SP_FORCEINLINE void from_file(const sp::string& filename) { p.from_file(filename); }
    SP_FORCEINLINE sp::vector<sp::vector<typename policy::s_t>> tokenize_batch(const sp::vector<sp::string>& texts, sp::thread_pool& pool) const { return p.tokenize_batch(texts, pool); }
};


}
#endif
