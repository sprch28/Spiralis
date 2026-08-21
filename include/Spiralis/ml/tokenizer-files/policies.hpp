#ifndef ____SP_POLICIES_HPP____
#define ____SP_POLICIES_HPP____
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


// GREEDY SUBWORD TOKENIZER
namespace sp_pol{
template <ull min_len, ull max_len, typename T = uint32_t>
class greedy_subword_tokenizer : public TokenizerPolicy<T>{
private:
    sp::hash_map<sp::string, T> stoi_mapping{};
    sp::vector<sp::compressed_string_view> itos_mapping{}; // points to stoi mapping keys
    bool mapping_built = false;
    static const inline sp::vector<sp::string> color_vec = {
        sp::console::FG_RED, sp::console::FG_BRIGHT_MAGENTA, sp::console::FG_BRIGHT_YELLOW,
        sp::console::FG_BLUE, sp::console::FG_GREEN,sp::console::FG_WHITE,sp::console::FG_BRIGHT_BLUE,
        sp::console::FG_BRIGHT_CYAN, sp::console::FG_BLACK
    };

    SP_FORCEINLINE static sp::string unpack_chars(ull packed, size_type n) {
        sp::string result(n, '\0');
        std::memcpy(result.data(), &packed, n);
        return result;
    }

    template <bool debug>
    void priv_build_mapping(const sp::string& text, ull vocab_size) {
        sp::thread_pool pool;
        ull idx = 0;

        ull num_threads = pool.size();
        
        // Grid of maps: partition_maps[worker_id][target_partition]
        sp::vector<sp::vector<sp::hash_map<ull, ull>>> partition_maps(num_threads);
        for (auto& worker_partitions : partition_maps) {
            worker_partitions.resize(num_threads);
        }

        //for(ull sp_i = min_len; sp_i <= max_len; ++sp_i){
        for (ull sp_i = max_len; sp_i >= min_len; --sp_i) {
            if constexpr(debug) sp::println("At char count: ", sp_i);
            ull size = text.size();
            const char* ptr = text.c_str();

            SP_IF_NOT_EXPECT(size < sp_i) continue;
            
            ull total_work = size - sp_i + 1;
            ull chunk_size = total_work / num_threads;

            // PHASE 1: Parallel Scan & Partitioning
            for (ull t = 0; t < num_threads; ++t) {
                ull start_idx = t * chunk_size;
                ull end_idx = (t == num_threads - 1) ? total_work : (start_idx + chunk_size);
                ull chunk_work = end_idx - start_idx;

                ull estimated_uniques;
                SP_IF_EXPECT(sp_i <= 4) estimated_uniques = (sp_i == 2) ? 65536 : 200'000;
                else if (sp_i <= 5) estimated_uniques = sp::min<ull>(chunk_work * 0.15, 1'000'000);
                else estimated_uniques = sp::min<ull>(chunk_work * 0.40, 2'500'000);

                ull per_partition_est = (estimated_uniques / num_threads) + 1;
                for (ull p = 0; p < num_threads; ++p) {
                    if (partition_maps[t][p].capacity() < per_partition_est){
                        partition_maps[t][p].reserve(per_partition_est);
                    }
                }

                pool.enqueue([t, ptr, sp_i, start_idx, end_idx, num_threads, &partition_maps]() {
                    const uint64_t mask = (sp_i == 8) ? ~0ULL : ((1ULL << (sp_i * 8)) - 1);
                    auto& my_partitions = partition_maps[t];

                    if constexpr (debug) {
                        ull idx = 0;
                        if (t == num_threads - 1) {
                            uint64_t total = end_idx - start_idx;
                            uint64_t step = total / 100;

                            for (ull i = start_idx; i < end_idx; ++i) {
                                uint64_t packed = 0;
                                std::memcpy(&packed, ptr + i, sp::min<size_type>(sp_i, 8));
                                packed &= mask;

                                ull target_partition = ((packed * 0x9e3779b97f4a7c15ULL) >> 32) % num_threads;
                                my_partitions[target_partition][packed]++;

                                if(step && (i - start_idx + 1) % step == 0) sp::print(color_vec[idx],'=', sp::console::RESET_EFFECTS, sp::flush);
                                idx = (idx + 1) & 7;
                            }
                            sp::println();
                        } else {
                            for (ull i = start_idx; i < end_idx; ++i) {
                                uint64_t packed = 0;
                                std::memcpy(&packed, ptr + i, sp::min<size_type>(sp_i, 8));
                                packed &= mask;

                                ull target_partition = ((packed * 0x9e3779b97f4a7c15ULL) >> 32) % num_threads;
                                my_partitions[target_partition][packed]++;
                            }
                        }
                    } else {
                        for (ull i = start_idx; i < end_idx; ++i) {
                            uint64_t packed = 0;
                            std::memcpy(&packed, ptr + i, sp::min<size_type>(sp_i, 8));
                            packed &= mask;

                            ull target_partition = ((packed * 0x9e3779b97f4a7c15ULL) >> 32) % num_threads;
                            my_partitions[target_partition][packed]++;
                        }
                    }
                });
            }
            pool.wait_all();

            // PHASE 2: Parallel In-Partition Aggregation & Top-K Trimming
            ull potential = vocab_size / (max_len - min_len + 1);
            
            // Use auto decltype to dynamically match the exact array element type returned by hash_map::to_array()
            using ArrayType = decltype(spt::declval<sp::hash_map<ull, ull>>().to_array());
            sp::vector<ArrayType> partition_top_k(num_threads);

            for (ull p = 0; p < num_threads; ++p) {
                pool.enqueue([p, num_threads, potential, &partition_maps, &partition_top_k]() {
                    sp::hash_map<ull, ull> final_partition_map;

                    ull total_elements = 0;
                    for (ull worker = 0; worker < num_threads; ++worker) {
                        total_elements += partition_maps[worker][p].size();
                    }
                    final_partition_map.reserve(total_elements);

                    for (ull worker = 0; worker < num_threads; ++worker) {
                        auto& src_map = partition_maps[worker][p];
                        for (const auto& [key, count] : src_map) {
                            final_partition_map[key] += count;
                        }
                        src_map.clear();
                    }

                    auto local_arr = final_partition_map.to_array();
                    final_partition_map.clear();

                    if (local_arr.size() > potential) {
                        std::nth_element(local_arr.begin(), local_arr.begin() + potential, local_arr.end(),
                            [](const auto& a, const auto& b) {
                                return a.second > b.second;
                            });
                        local_arr.resize(potential);
                    }

                    partition_top_k[p] = sp::move(local_arr);
                });
            }
            pool.wait_all();

            // PHASE 3: Flatten candidate arrays using push_back instead of vector range insert
            using ElementType = sp::pair<ull,ull>;
            sp::vector<ElementType> global_candidates;

            ull total_candidates = 0;
            for (const auto& top_k : partition_top_k) {
                total_candidates += top_k.size();
            }
            global_candidates.reserve(total_candidates);

            for (auto& top_k : partition_top_k) {
                for (const auto& item : top_k) {
                    global_candidates.push_back(item);
                }
                top_k.clear();
            }

            if (global_candidates.size() > potential) {
                std::nth_element(global_candidates.begin(), global_candidates.begin() + potential, global_candidates.end(),
                    [](const auto& a, const auto& b) {
                        return a.second > b.second;
                    });
                global_candidates.resize(potential);
            }

            for (const auto& p : global_candidates) {
                sp::string str;
                str.resize(sp_i);
                for (ull b = 0; b < sp_i; ++b) {
                    str[b] = static_cast<char>((p.first >> (b * 8)) & 0xFF);
                }

                auto [it, inserted] = stoi_mapping.insert_or_assign(sp::move(str), idx);
                if (inserted) ++idx;
            }
        }

        for (int i = 0; i < 256; ++i) {
            sp::string c(1, (char)i);
            if (stoi_mapping.find(c) == stoi_mapping.end()) stoi_mapping.insert(c, idx++);
        }

        itos_mapping = sp::vector<sp::compressed_string_view>(stoi_mapping.size());
        for (auto it = stoi_mapping.begin(); it != stoi_mapping.end(); ++it)
            itos_mapping[it->second] = sp::compressed_string_view(it->first.c_str(), it->first.size());
        mapping_built = true;
    }
public:
    SP_FORCEINLINE void build_mapping(const sp::string& text, ull vocab_size) override { return priv_build_mapping<false>(text, vocab_size); }
    SP_FORCEINLINE void build_mapping_debug(const sp::string& text, ull vocab_size) override { return priv_build_mapping<true>(text, vocab_size); }
    SP_FORCEINLINE const sp::hash_map<sp::string, T>& mapping() { return stoi_mapping; }
    sp::vector<T> tokenize(const sp::string& text) const override {
        SP_IF_NOT_EXPECT(!mapping_built) throw sp::exceptions::spiral_exception("Error on tokenize()...");

        const ull n = text.size();
        SP_IF_NOT_EXPECT(n == 0) { sp::println("Empty input."); return {}; }

        const char* str = text.c_str();

        sp::vector<ull> min_costs(n + 1, sp::npos);
        sp::vector<T> token_ids(n + 1, -1);
        sp::vector<ll> parent_indices(n + 1, -1);

        min_costs[0] = 0;

        const double cost_scale = 10.0 / static_cast<double>(stoi_mapping.size());
        for (ull i = 0; i < n; ++i) {
            if(min_costs[i] == sp::npos) continue;

            const ull limit = sp::min(i + max_len, n);
            for (ull j = i + 1; j <= limit; ++j){
                sp::compressed_string_view p(str + i, j - i);
                auto it = stoi_mapping.find(p);
                if (it != stoi_mapping.end()) {
                    ull token_cost = 1 + static_cast<ull>(it->second * cost_scale);
                    ull cost = min_costs[i] + token_cost;
                    if (min_costs[j] > cost) {
                        min_costs[j] = cost;
                        token_ids[j] = it->second;
                        parent_indices[j] = i;
                    }
                }
            }
        }

        SP_IF_NOT_EXPECT(min_costs[n] == sp::npos) { sp::println("No way to tokenize text."); return {}; }

        sp::vector<T> final_tokens;
        final_tokens.reserve(n/min_len); // should almost never take a reallocation, especially since reserve rounds to power of 2

        ull curr = n;
        while (curr > 0) {
            final_tokens.push_back(token_ids[curr]);
            curr = parent_indices[curr];
        }

        return final_tokens.reverse();
    }

    sp::vector<sp::string> reconstructed(const sp::vector<T> tokens) const override{
        sp::vector<sp::string> result;
        result.reserve_exact(tokens.size());
        for(T tok : tokens) result.push_back(sp::string(itos_mapping.at(tok).data(),itos_mapping[tok].size()));
        return result;
    }

    void to_file(const sp::string& filename) const override{
        sp::file f(filename.c_str(),sp::write); sp::IO scanner(f);
        scanner.write((ull)stoi_mapping.size());
        for(const auto& entry : stoi_mapping){
            scanner.write((ull)entry.first.size());
            scanner.write(entry.first);
            scanner.write((T)entry.second);
        }
        scanner.flush();
    }

    void from_file(const sp::string& filename) override{
        sp::file f(filename.c_str(),sp::read); sp::IO scanner(f);
        ull sz;
        scanner.read<ull>(sz);
        itos_mapping = sp::vector<sp::compressed_string_view>(sz);
        stoi_mapping.reserve(sz);
        for(ull i = 0; i < sz; ++i){
            ull string_sz;
            scanner.read<ull>(string_sz);
            sp::string str(string_sz,' ');
            scanner.read<sp::string>(str);
            T occ; scanner.read<T>(occ);
            auto [it, inserted] = stoi_mapping.insert(sp::move(str),occ);
        }
        for(const auto& i : stoi_mapping){
            itos_mapping[i.second] = sp::compressed_string_view(i.first.c_str(),i.first.size());
        }
        mapping_built = true;
    }

    sp::vector<sp::vector<T>> tokenize_batch(
        const sp::vector<sp::string>& texts, 
        sp::thread_pool& pool
    ) const override {
        const size_type num_texts = texts.size();
        SP_IF_NOT_EXPECT(num_texts == 0) return {};
        sp::vector<sp::vector<T>> results(num_texts);

        for (size_type i = 0; i < num_texts; ++i){
            const auto& input_text = texts[i];
            auto* output_slot = &results[i];

            pool.enqueue([this, &input_text, output_slot](){
                //sp::vector<T> local_tokens = this->tokenize(input_text);
                //*output_slot = sp::move(local_tokens);
                *output_slot = this->tokenize(input_text);
            });
        }

        pool.wait_all();
        return results;
    }
};


}

#endif