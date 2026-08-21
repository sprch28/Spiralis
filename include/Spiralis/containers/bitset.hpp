#ifndef ____SP_BITSET____
#define ____SP_BITSET____
#pragma once

#include "../setup/init.hpp"
#include "../containers/string.hpp"
#include "../math/bit_manip.hpp"

namespace sp {

template <ull num_bits = 64>
class bitset {
private:
    static constexpr ull bits_per_chunk = sizeof(ull) * 8;
    static constexpr ull num_chunks = (num_bits + bits_per_chunk - 1) / bits_per_chunk;
    ull chunks[num_chunks > 0 ? num_chunks : 1]{};

    constexpr void sanitize() noexcept {
        constexpr ull unused = (num_chunks * bits_per_chunk) - num_bits;
        if constexpr (unused > 0 && num_chunks > 0) {
            chunks[num_chunks - 1] &= (~0ULL >> unused);
        }
    }

public:
    class reference {
        friend class bitset;
        ull* chunk;
        ull mask;
        constexpr reference(ull* c, ull m) noexcept : chunk(c), mask(m) {}
    public:
        constexpr reference& operator=(bool val) noexcept {
            if (val) *chunk |= mask;
            else     *chunk &= ~mask;
            return *this;
        }

        constexpr reference& operator=(const reference& rhs) noexcept {
            return *this = static_cast<bool>(rhs);
        }

        constexpr operator bool() const noexcept {
            return (*chunk & mask) != 0;
        }

        constexpr bool operator~() const noexcept {
            return !static_cast<bool>(*this);
        }

        constexpr reference& flip() noexcept {
            *chunk ^= mask;
            return *this;
        }
    };

    constexpr bitset() noexcept = default;

    constexpr bitset(ull val) noexcept {
        if constexpr (num_chunks > 0) {
            chunks[0] = val;
            sanitize();
        }
    }

    explicit bitset(const sp::string& str) noexcept {
        ull limit = str.size() < num_bits ? str.size() : num_bits;
        for (ull i = 0; i < limit; ++i) {
            if (str[str.size() - 1 - i] == '1') {
                set(i);
            }
        }
    }

    // --- Capacity & Inspection ---
    constexpr ull size() const noexcept {
        return num_bits;
    }

    constexpr ull count() const noexcept {
        ull total = 0;
        for (ull i = 0; i < num_chunks; ++i) {
            total += sp::popcount(chunks[i]);
        }
        return total;
    }

    constexpr bool all() const noexcept {
        return count() == num_bits;
    }

    constexpr bool any() const noexcept {
        for (ull i = 0; i < num_chunks; ++i) {
            if (chunks[i] != 0) return true;
        }
        return false;
    }

    constexpr bool none() const noexcept {
        return !any();
    }

    constexpr bool test(ull pos) const noexcept {
        ull chunk_idx = pos / bits_per_chunk;
        ull bit_offset = pos % bits_per_chunk;
        return (chunks[chunk_idx] & (1ULL << bit_offset)) != 0;
    }

    // --- Element Access ---
    constexpr bool operator[](ull pos) const noexcept {
        return test(pos);
    }

    constexpr reference operator[](ull pos) noexcept {
        ull chunk_idx = pos / bits_per_chunk;
        ull bit_offset = pos % bits_per_chunk;
        return reference(&chunks[chunk_idx], 1ULL << bit_offset);
    }

    // --- Modifiers ---
    constexpr bitset& set() noexcept {
        for (ull i = 0; i < num_chunks; ++i) {
            chunks[i] = ~0ULL;
        }
        sanitize();
        return *this;
    }

    constexpr bitset& set(ull pos, bool val = true) noexcept {
        ull chunk_idx = pos / bits_per_chunk;
        ull bit_offset = pos % bits_per_chunk;
        if (val) {
            chunks[chunk_idx] |= (1ULL << bit_offset);
        } else {
            chunks[chunk_idx] &= ~(1ULL << bit_offset);
        }
        return *this;
    }

    constexpr bitset& reset() noexcept {
        for (ull i = 0; i < num_chunks; ++i) {
            chunks[i] = 0;
        }
        return *this;
    }

    constexpr bitset& reset(ull pos) noexcept {
        return set(pos, false);
    }

    constexpr bitset& flip() noexcept {
        for (ull i = 0; i < num_chunks; ++i) {
            chunks[i] = ~chunks[i];
        }
        sanitize();
        return *this;
    }

    constexpr bitset& flip(ull pos) noexcept {
        ull chunk_idx = pos / bits_per_chunk;
        ull bit_offset = pos % bits_per_chunk;
        chunks[chunk_idx] ^= (1ULL << bit_offset);
        return *this;
    }

    // --- Conversions ---
    constexpr ull to_ullong() const noexcept {
        return num_chunks > 0 ? chunks[0] : 0;
    }

    sp::string to_string(char zero = '0', char one = '1') const {
        sp::string result;
        result.reserve_exact(num_bits);
        for (ull i = num_bits; i > 0; --i) {
            result.push_back(test(i - 1) ? one : zero);
        }
        return result;
    }

    // --- Bitwise Operators & Assignment ---
    constexpr bitset operator~() const noexcept {
        bitset res = *this;
        res.flip();
        return res;
    }

    constexpr bitset& operator&=(const bitset& rhs) noexcept {
        for (ull i = 0; i < num_chunks; ++i) {
            chunks[i] &= rhs.chunks[i];
        }
        sanitize();
        return *this;
    }

    constexpr bitset& operator|=(const bitset& rhs) noexcept {
        for (ull i = 0; i < num_chunks; ++i) {
            chunks[i] |= rhs.chunks[i];
        }
        sanitize();
        return *this;
    }

    constexpr bitset& operator^=(const bitset& rhs) noexcept {
        for (ull i = 0; i < num_chunks; ++i) {
            chunks[i] ^= rhs.chunks[i];
        }
        sanitize();
        return *this;
    }

    constexpr bitset operator&(const bitset& rhs) const noexcept {
        bitset res = *this;
        return res &= rhs;
    }

    constexpr bitset operator|(const bitset& rhs) const noexcept {
        bitset res = *this;
        return res |= rhs;
    }

    constexpr bitset operator^(const bitset& rhs) const noexcept {
        bitset res = *this;
        return res ^= rhs;
    }

    // --- Shift Operators ---
    constexpr bitset& operator<<=(ull shift) noexcept {
        if (shift >= num_bits) return reset();
        ull chunk_shift = shift / bits_per_chunk;
        ull bit_shift = shift % bits_per_chunk;

        if (bit_shift == 0) {
            for (ull i = num_chunks; i-- > 0; ) {
                chunks[i] = (i >= chunk_shift) ? chunks[i - chunk_shift] : 0;
            }
        } else {
            for (ull i = num_chunks; i-- > 0; ) {
                ull val = (i >= chunk_shift) ? chunks[i - chunk_shift] : 0;
                ull prev = (i >= chunk_shift + 1) ? chunks[i - chunk_shift - 1] : 0;
                chunks[i] = (val << bit_shift) | (prev >> (bits_per_chunk - bit_shift));
            }
        }
        sanitize();
        return *this;
    }

    constexpr bitset& operator>>=(ull shift) noexcept {
        if (shift >= num_bits) return reset();
        ull chunk_shift = shift / bits_per_chunk;
        ull bit_shift = shift % bits_per_chunk;

        if (bit_shift == 0) {
            for (ull i = 0; i < num_chunks; ++i) {
                chunks[i] = (i + chunk_shift < num_chunks) ? chunks[i + chunk_shift] : 0;
            }
        } else {
            for (ull i = 0; i < num_chunks; ++i) {
                ull val = (i + chunk_shift < num_chunks) ? chunks[i + chunk_shift] : 0;
                ull next = (i + chunk_shift + 1 < num_chunks) ? chunks[i + chunk_shift + 1] : 0;
                chunks[i] = (val >> bit_shift) | (next << (bits_per_chunk - bit_shift));
            }
        }
        sanitize();
        return *this;
    }

    constexpr bitset operator<<(ull shift) const noexcept {
        bitset res = *this;
        return res <<= shift;
    }

    constexpr bitset operator>>(ull shift) const noexcept {
        bitset res = *this;
        return res >>= shift;
    }

    constexpr bool operator==(const bitset& rhs) const noexcept {
        for (ull i = 0; i < num_chunks; ++i) {
            if (chunks[i] != rhs.chunks[i]) return false;
        }
        return true;
    }

    constexpr bool operator!=(const bitset& rhs) const noexcept {
        return !(*this == rhs);
    }
};

} // namespace sp

#endif // ____SP_BITSET____