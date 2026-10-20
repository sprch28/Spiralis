// Only to be included in tensor.hpp

// to shut up the IDE while working
//#include "../containers/tensor.hpp"

/// @brief Factory class for constructing `sp::tensor` instances.
/// @note Declared as `friend` inside `sp::tensor` to allow direct access to private members (e.g. `_data`, `_uninit_tag`).
class spml{ // for friend access
public:
/// @brief Returns a zero-initialized tensor with the given shape.
/// @tparam T Element type.
/// @param args Dimension sizes.
template <typename T, typename... Args>
static sp::tensor<T> zeros(Args&&... args){
    return sp::tensor<T>(args...);
}

/// @brief Returns a tensor filled with `val` and the given shape.
/// @tparam T Element type.
/// @param val Value to fill every element with.
/// @param args Dimension sizes.
template <typename T, typename... Args>
static sp::tensor<T> full(T val, Args&&... args){
    sp::tensor<T> result(args...);
    T* SP_RESTRICT dta = result.data();
    _SP_APPLY_UNROLLED_(result.size(),dta[i] = val);
    return result;
}

/// @brief Returns a tensor filled with `1` and the given shape.
/// @tparam T Element type.
/// @param args Dimension sizes.
template <typename T, typename... Args>
static sp::tensor<T> ones(Args&&... args){
    return full(1, args...);
}

/// @brief Returns a 1D tensor of evenly spaced values in `[start, stop)` with the given `step`.
/// @tparam T Element type.
/// @param start First value.
/// @param stop Upper bound (exclusive).
/// @param step Spacing between values; must not be zero.
/// @throws sp::exceptions::TensorException if `step == 0`.
template <typename T>
static sp::tensor<T> arange(T start, T stop, T step = 1) {
    if(step == 0) throw sp::exceptions::TensorException("Step cannot be zero.");

    auto count = std::floor((stop - start) / step);
    size_type size = (count > 0) ? static_cast<size_type>(count) : 0;

    sp::tensor<T> result(typename sp::tensor<T>::_uninit_tag{}, size);
    
    for(size_type idx = 0; idx < size; ++idx) result._data[idx] = start + static_cast<T>(idx) * step;

    return result;
}

/// @brief Returns a 1D tensor of values `[0, stop)` with step 1.
/// @tparam T Element type.
/// @param stop Number of elements; values are `0, 1, ..., stop-1`.
template <typename T>
static sp::tensor<T> arange(T stop){
    sp::tensor<T> result(typename sp::tensor<T>::_uninit_tag{}, static_cast<size_type>(stop));
    _SP_APPLY_UNROLLED_(stop, result._data[i] = i);
    return result;
}

/// @brief Returns a 1D tensor of `steps` evenly spaced values between `start` and `stop`.
/// @tparam T Element type; defaults to `double`.
/// @tparam inclusive If `true`, the last element is exactly `stop`; if `false` (default), the interval is half-open `[start, stop)`.
/// @param start First value.
/// @param stop End of the interval.
/// @param steps Number of values to generate; returns an empty tensor if `0`.
template <typename T=double, bool inclusive=false>
static sp::tensor<T> linspace(T start, T stop, size_type steps) {
    SP_IF_NOT_EXPECT(steps == 0) return sp::tensor<T>();
    sp::tensor<T> result(typename sp::tensor<T>::_uninit_tag{}, steps);
    if(steps == 1){
        result.data()[0] = start;
        return result;
    }

    SP_IF_CONSTEXPR(inclusive){
        const T step_size = (stop - start) / static_cast<T>(steps - 1);
        for(size_type i = 0; i < steps - 1; ++i) result.data()[i] = start + static_cast<T>(i) * step_size;
        result.data()[steps - 1] = stop; // Force exact end point
    }else{
        const T step_size = (stop - start) / static_cast<T>(steps);
        for(size_type i = 0; i < steps; ++i) result.data()[i] = start + static_cast<T>(i) * step_size;
    }
    
    return result;
}

/// @brief Returns a tensor filled with pseudo-random bit patterns reinterpreted as `T`.
/// @tparam T Trivially copyable element type; defaults to `double`.
/// @param args Dimension sizes.
/// @note Values are raw 64-bit PRNG output truncated to `sizeof(T)` bytes — not a uniform distribution.
template <typename T=double, typename... Args>
static spt::enable_if_t<spt::is_trivially_copyable_v<T>,sp::tensor<T>> randoms(Args&&... args){
    sp::tensor<T> result(args...);
    const sp::ull sz = result.size();
    for(sp::ull i = 0; i < sz; ++i){
        sp::ull raw_rand = sp::get_psrand64();
        std::memcpy(&result._data[i],&raw_rand,sizeof(T));
    }
    return result;
}
};

