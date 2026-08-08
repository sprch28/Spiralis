#ifndef ____SP_ALGORITHM____
#define ____SP_ALGORITHM____
#pragma once
#include "../setup/init.hpp"
#include "../collections/array.hpp"
#include "../collections/string.hpp"

namespace sp {

template <typename T>
sp::string to_string(T&& value) {
    using CleanT = spt::decay_t<T>;
    if constexpr (spt::is_same_v<CleanT, const char*> || spt::is_same_v<CleanT, char*>) {
        return sp::string(value);
    } else if constexpr (spt::is_same_v<CleanT, char>) {
        return sp::string(1, value);
    } else if constexpr (spt::is_arithmetic_v<CleanT>) {
        if constexpr (spt::is_integral_v<CleanT>) {
            static constexpr char digits[] =
                "0001020304050607080910111213141516171819"
                "2021222324252627282930313233343536373839"
                "4041424344454647484950515253545556575859"
                "6061626364656667686970717273747576777879"
                "8081828384858687888990919293949596979899";

            char buf[32];
            char* ptr = buf + sizeof(buf);

            using UnsignedT = spt::make_unsigned_t<CleanT>;
            bool is_negative = false;
            UnsignedT uval;

            if constexpr (spt::is_signed_v<CleanT>) {
                if (value < 0) {
                    is_negative = true;
                    uval = static_cast<UnsignedT>(0) - static_cast<UnsignedT>(value);
                } else {
                    uval = static_cast<UnsignedT>(value);
                }
            } else {
                uval = value;
            }

            while (uval >= 100) {
                auto idx = (uval % 100) * 2;
                uval /= 100;
                *--ptr = digits[idx + 1];
                *--ptr = digits[idx];
            }

            if (uval >= 10) {
                auto idx = uval * 2;
                *--ptr = digits[idx + 1];
                *--ptr = digits[idx];
            } else {
                *--ptr = static_cast<char>('0' + uval);
            }

            if (is_negative) {
                *--ptr = '-';
            }

            size_t len = static_cast<size_t>((buf + sizeof(buf)) - ptr);
            return sp::string(ptr, len);
        }
        
        else if constexpr (spt::is_floating_point_v<CleanT>) {
            char buf[64];
            char* ptr = buf;

            CleanT val = value;
            if (val < 0) {
                *ptr++ = '-';
                val = -val;
            }

            // Extract integral and fractional parts
            auto int_part = static_cast<unsigned long long>(val);
            CleanT frac_part = val - static_cast<CleanT>(int_part);

            // Print integer portion recursively using our integral logic
            sp::string int_str = to_string(int_part);
            for (size_t i = 0; i < int_str.size(); ++i) {
                *ptr++ = int_str[i];
            }

            *ptr++ = '.';

            // Extract 6 decimal places of precision
            for (int i = 0; i < 6; ++i) {
                frac_part *= 10.0;
                int digit = static_cast<int>(frac_part);
                *ptr++ = static_cast<char>('0' + digit);
                frac_part -= digit;
            }

            return sp::string(buf, static_cast<size_t>(ptr - buf));
        }
    } else {
        static_assert(!sizeof(T*), "Unsupported type conversion");
    }
}

template <typename T, bool one_indexed = true>
SP_FORCEINLINE sp::vector<T> prefix_vector(const sp::vector<T>& arr) {
    SP_IF_NOT_EXPECT(arr.is_empty()) return {};
    if constexpr (one_indexed) {
        sp::vector<T> result(arr.size() + 1, T());
        for (ull i = 1; i < result.size(); i++) result[i] = result[i - 1] + arr[i - 1];
        return result;
    } else {
        sp::vector<T> result(arr.size());
        result[0] = arr[0];
        for (ull i = 1; i < result.size(); i++) result[i] = result[i - 1] + arr[i];
        return result;
    }
}

template <typename T, bool one_indexed = true>
SP_FORCEINLINE sp::vector<sp::vector<T>> prefix_vector(const sp::vector<sp::vector<T>>& arr) {
    SP_IF_NOT_EXPECT(arr.empty() || arr[0].empty()) return {};
    ull rows = arr.size();
    ull cols = arr[0].size();

    if constexpr (one_indexed) {
        sp::vector<sp::vector<T>> result(rows + 1, sp::vector<T>(cols + 1, T()));
        for (ull i = 1; i <= rows; i++) {
            for (ull j = 1; j <= cols; j++) {
                result[i][j] = arr[i - 1][j - 1] + result[i - 1][j] + result[i][j - 1] - result[i - 1][j - 1];
            }
        }
        return result;
    } else {
        sp::vector<sp::vector<T>> result(rows, sp::vector<T>(cols));
        result[0][0] = arr[0][0];
        for (ull i = 1; i < rows; i++) result[i][0] = result[i - 1][0] + arr[i][0];
        for (ull j = 1; j < cols; j++) result[0][j] = result[0][j - 1] + arr[0][j];
        for (ull i = 1; i < rows; i++) {
            for (ull j = 1; j < cols; j++) {
                result[i][j] = arr[i][j] + result[i - 1][j] + result[i][j - 1] - result[i - 1][j - 1];
            }
        }
        return result;
    }
}

} // namespace sp
#endif // ____SP_ALGORITHM____