#ifndef ____SP_ALGORITHM____
#define ____SP_ALGORITHM____
#pragma once
#include "../setup/init.hpp"
#include "../collections/array.hpp"
#include "../collections/string.hpp"
#include <string>

namespace sp {

template <typename T>
sp::string to_string(T&& value) {
    // temporary until implemented
    return sp::string(std::to_string(value).c_str());
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

// Expects input to be valid shape
template <typename T, bool one_indexed = true>
SP_FORCEINLINE sp::vector<sp::vector<T>> prefix_vector(const sp::vector<sp::vector<T>>& arr) {
    SP_IF_NOT_EXPECT(arr.is_empty() || arr[0].is_empty()) return {};
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