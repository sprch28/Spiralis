// Only to be included in tensor.hpp

// to shut up the IDE while working
//#include "../containers/tensor.hpp"

// zeros
class spml{ // for friend access
public:
template <typename T, typename... Args>
static sp::tensor<T> zeros(Args&&... args){
    return sp::tensor<T>(args...);
}

template <typename T, typename... Args>
static sp::tensor<T> full(T val, Args&&... args){
    sp::tensor<T> result(args...);
    T* SP_RESTRICT dta = result.data();
    _SP_APPLY_UNROLLED_(result.size(),dta[i] = val);
    return result;
}

template <typename T, typename... Args>
static sp::tensor<T> ones(Args&&... args){
    return full(1, args...);
}
template <typename T>
static sp::tensor<T> arange(T start, T stop, T step = 1){
    sp::tensor<T> result((stop-start+step-1)/step);
    size_type idx = 0;
    for(size_type i = start; i < stop; i += step) result._data[idx++] = i; 
    return result;
}

template <typename T>
static sp::tensor<T> arange(T stop){
    sp::tensor<T> result(static_cast<size_type>(stop));
    _SP_APPLY_UNROLLED_(stop, result._data[i] = i);
    return result;
}

template <typename T=double, bool inclusive=false>
static sp::tensor<T> linspace(T start, T stop, size_type steps) {
    SP_IF_NOT_EXPECT(steps == 0) return sp::tensor<T>();
    sp::tensor<T> result(steps);
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

template <typename T=double, typename... Args>
static spt::enable_if_t<spt::is_trivially_copyable_v<T>,sp::tensor<T>> randoms(Args&&... args){
    sp::tensor<T> result(args...);
    const ull sz = result.size();
    for(ull i = 0; i < sz; ++i){
        ull raw_rand = sp::get_psrand64();
        std::memcpy(&result._data[i],&raw_rand,sizeof(T));
    }
    return result;
}
};

