#ifndef ____SP_TENSOR____
#define ____SP_TENSOR____
#pragma once
#include "../setup/init.hpp"
#include "../core/allocators.hpp"
#include "../core/type_traits.hpp"
#include "../core/pointer.hpp"
#include "../io/IO.hpp"
#include "../math/math.hpp" 
#include "../containers/array.hpp"
#include "../parallel/SIMD.hpp"
class spml;
namespace sp{
template <typename T, template <typename> class Allocator = sp::aligned_allocator>
class alignas(spt::get_allocator_alignment<Allocator<T>>()
? sp_cache_line_size 
: max(alignof(Allocator<T>),max(alignof(T*),alignof(size_type)))) tensor{
private:
    friend class ::spml;
    alignas(spt::get_allocator_alignment<Allocator<T>>() ? sp_cache_line_size 
    : max(alignof(Allocator<T>),max(alignof(T*),alignof(size_type)))) 
    T* _data = nullptr;
    size_type* _shapes = nullptr;
    size_type* _strides = nullptr;
    size_type _size = 0;
    size_type _shape_size = 0;
    size_type _stride_size = 0;
    SP_NO_UNIQUE_ADDRESS Allocator<T> _data_alloc;
    SP_NO_UNIQUE_ADDRESS Allocator<size_type> _meta_alloc;
    static constexpr bool _trivially_copyable = spt::is_trivially_copyable_v<T>;
    SP_FORCEINLINE constexpr void destroy_data(){
        SP_IF_CONSTEXPR(!_trivially_copyable){
            for(size_type i = 0; i < _size; ++i){
                sp::allocator_traits<Allocator<T>>::destroy(_data_alloc, _data+i);
            }
        }
        sp::allocator_traits<Allocator<T>>::deallocate(_data_alloc,_data,_size);
    }
    template <typename U>
    SP_FORCEINLINE constexpr void _build_from_list(std::initializer_list<U> list, size_type depth, sp::vector<size_type>& shape, size_type& total){
        SP_IF_CONSTEXPR(spt::is_init_list_v<U>){
            if(shape.size() <= depth)
                shape.push_back(list.size());
            //else
                //assert(shape[depth] == list.size());
            for(auto& sub : list)
                _build_from_list(sub, depth + 1, shape, total);
        }else{
            if(shape.size() <= depth) shape.push_back(list.size());
            total += list.size();
        }
    }

    template <typename U>
    SP_FORCEINLINE constexpr void _fill_from_list(std::initializer_list<U> list, size_type& offset) {
        SP_IF_CONSTEXPR (spt::is_init_list_v<U>){
            for(auto& sub : list)
                _fill_from_list(sub, offset);
        }else{
            for(const auto& v : list){
                SP_IF_CONSTEXPR(_trivially_copyable){
                    _data[offset++] = static_cast<T>(v);
                }else{
                    sp::allocator_traits<Allocator<T>>::construct(_data_alloc, _data + offset, static_cast<T>(v));
                    ++offset;
                }
            }
        }
    }

    SP_FORCEINLINE constexpr void compute_strides() {
        SP_IF_NOT_EXPECT(_shape_size == 0) return;
        _strides[_shape_size - 1] = 1;
        for (int i = static_cast<int>(_shape_size) - 2; i >= 0; --i)
            _strides[i] = _strides[i + 1] * _shapes[i + 1];
    }

    template <typename... Idxs>
    SP_FORCEINLINE constexpr size_type _offset(Idxs... idxs) const {
        size_type offset = 0;
        size_type i = 0;
        ((offset += static_cast<size_type>(idxs) * _strides[i++]), ...); // C++17
        return offset;
    }
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
public:
    using type_param = spt::conditional_t<spt::is_trivially_copyable_v<T> || sizeof(T) <= 8, T, const T&>;
    using flat_iterator = flat_tensor_iterator<T>;
    using const_flat_iterator = const flat_tensor_iterator<T>;
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    constexpr tensor() {}
    template <typename U>
    /*constexpr*/ tensor(std::initializer_list<std::initializer_list<U>> list){
        sp::vector<size_type> shape; // sp::vector has no constexpr support yet
        size_type total = 0;
        _build_from_list(list, 0, shape, total);

        SP_IF_NOT_EXPECT(total == 0) return;

        _shape_size  = shape.size();
        _stride_size = _shape_size;
        _shapes  = sp::allocator_traits<Allocator<size_type>>::allocate(_meta_alloc, _shape_size);
        _strides = sp::allocator_traits<Allocator<size_type>>::allocate(_meta_alloc, _stride_size);

        memcpy(_shapes,shape.data(),shape.size()*sizeof(size_type));
        /*for(size_type i = 0; i < _shape_size; ++i)
            _shapes[i] = shape[i];*/

        compute_strides();

        _size = total;
        _data = sp::allocator_traits<Allocator<T>>::allocate(_data_alloc, _size);

        size_type offset = 0;
        _fill_from_list(list, offset);
    }
    constexpr tensor(std::initializer_list<T> list){
        _shape_size = 1;
        _stride_size = 1;
        _shapes = sp::allocator_traits<Allocator<size_type>>::allocate(_meta_alloc,1);
        _strides = sp::allocator_traits<Allocator<size_type>>::allocate(_meta_alloc,1);
        size_type total_sz = list.size();
        _shapes[0] = total_sz;
        _strides[0] = 1;
        _data = sp::allocator_traits<Allocator<T>>::allocate(_data_alloc,total_sz);
        for(const auto& v : list){
            SP_IF_CONSTEXPR(_trivially_copyable){
                _data[_size++] = static_cast<T>(v);
            }else{
                sp::allocator_traits<Allocator<T>>::construct(_data_alloc, _data + _size, static_cast<T>(v));
                ++_size;
            }
        }
    }
    tensor(sp::vector<size_type> shapes){
        _shape_size = shapes.size();
        _stride_size = _shape_size;
        _shapes = sp::allocator_traits<Allocator<size_type>>::allocate(_meta_alloc,_shape_size);
        _strides = sp::allocator_traits<Allocator<size_type>>::allocate(_meta_alloc,_stride_size);
        size_type sum = 1;
        _SP_APPLY_UNROLLED_(_shape_size, {
            _shapes[i] = shapes[i];
            sum *= shapes[i];
        });
        _strides[_stride_size-1] = 1;
        if(_stride_size>1){
            for(ull i = _stride_size-2; i != static_cast<ull>(-1); --i){
                _strides[i] = _strides[i+1] * _shapes[i+1];
            }
        }
        _size = sum;
        _data = sp::allocator_traits<Allocator<T>>::allocate(_data_alloc, _size);
        SP_IF_CONSTEXPR(_trivially_copyable){
            std::memset(_data,0,_size*sizeof(T));
        }else{
            _SP_APPLY_UNROLLED_(_size, sp::allocator_traits<Allocator<T>>::construct(_data_alloc,&_data+i,0));
        }
    }
    template <typename... Args, 
              typename = spt::enable_if_t<(spt::is_integral_v<spt::remove_reference_t<Args>> && ...)>>
    constexpr tensor(Args&&... args){
        size_type shapes[] = { static_cast<size_type>(args)... };
        _shape_size = sizeof...(args);
        _stride_size = _shape_size;
        _shapes = sp::allocator_traits<Allocator<size_type>>::allocate(_meta_alloc,_shape_size);
        _strides = sp::allocator_traits<Allocator<size_type>>::allocate(_meta_alloc,_stride_size);
        size_type sum = 1;
        _SP_APPLY_UNROLLED_(_shape_size, {
            _shapes[i] = shapes[i];
            sum *= shapes[i];
        });
        _strides[_stride_size-1] = 1;
        if(_stride_size>1){
            for(ull i = _stride_size-2; i != static_cast<ull>(-1); --i){
                _strides[i] = _strides[i+1] * _shapes[i+1];
            }
        }
        _size = sum;
        _data = sp::allocator_traits<Allocator<T>>::allocate(_data_alloc, _size);
        SP_IF_CONSTEXPR(_trivially_copyable){
            std::memset(_data,0,_size*sizeof(T));
        }else{
            _SP_APPLY_UNROLLED_(_size, sp::allocator_traits<Allocator<T>>::construct(_data_alloc,&_data+i,0));
        }
    }
    constexpr tensor(const tensor& other){
        *this = other;
    }
    constexpr tensor(tensor&& other){
        *this = sp::move(other);
    }
    #if ___SP_CPP_VER___ >= 20 
    constexpr 
    #endif 
    ~tensor(){
        if(_data){
            destroy_data();
            sp::allocator_traits<Allocator<size_type>>::deallocate(_meta_alloc, _shapes, _shape_size);
            sp::allocator_traits<Allocator<size_type>>::deallocate(_meta_alloc, _strides, _stride_size);
        }
    }

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    //=-=-=-=-=-=-=-=-=-=-=-s=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template <typename... Idxs>
    SP_FORCEINLINE constexpr T& operator()(Idxs... idxs) {
        static_assert(sizeof...(Idxs) > 0, "At least one index required to use operator()");
        return _data[_offset(idxs...)];
    }

    template <typename... Idxs>
    SP_FORCEINLINE constexpr const T& operator()(Idxs... idxs) const {
        return const_cast<tensor*>(this)->operator()(idxs...);
    }

    SP_FORCEINLINE constexpr T& operator[](size_type physical_idx){
        return _data[physical_idx];
    }

    SP_FORCEINLINE constexpr const T& operator[](size_type physical_idx) const{
        return _data[physical_idx];
    }

    SP_FORCEINLINE constexpr tensor& operator=(const tensor& other){
        if(this == &other) return *this;

        if(_data){
            destroy_data();
            sp::allocator_traits<Allocator<size_type>>::deallocate(_meta_alloc, _shapes, _shape_size);
            sp::allocator_traits<Allocator<size_type>>::deallocate(_meta_alloc, _strides, _stride_size);
        }

        _shape_size = other._shape_size;
        _stride_size = other._stride_size;
        _size = other._size;
        _data_alloc = other._data_alloc;
        _meta_alloc = other._meta_alloc;

        if(_size > 0){
            _data = sp::allocator_traits<Allocator<T>>::allocate(_data_alloc, _size);
            _shapes = sp::allocator_traits<Allocator<size_type>>::allocate(_meta_alloc, _shape_size);
            _strides = sp::allocator_traits<Allocator<size_type>>::allocate(_meta_alloc, _stride_size);
            
            std::memcpy(_shapes, other._shapes, _shape_size * sizeof(size_type));
            std::memcpy(_strides, other._strides, _stride_size * sizeof(size_type));
            SP_IF_CONSTEXPR(_trivially_copyable){
                std::memcpy(_data, other._data, _size * sizeof(T));
            }else _SP_APPLY_UNROLLED_(_size, _data[i] = other._data[i]);
        }else{
            _data = nullptr;
            _shapes = nullptr;
            _strides = nullptr;
        }
        return *this;
    }

    SP_FORCEINLINE constexpr tensor& operator=(tensor&& other) noexcept {
        if(this == &other) return *this;

        if(_data){
            destroy_data();
            sp::allocator_traits<Allocator<size_type>>::deallocate(_meta_alloc, _shapes, _shape_size);
            sp::allocator_traits<Allocator<size_type>>::deallocate(_meta_alloc, _strides, _stride_size);
        }

        _shape_size = other._shape_size;
        _stride_size = other._stride_size;
        _size = other._size;
        _data_alloc = sp::move(other._data_alloc);
        _meta_alloc = sp::move(other._meta_alloc);
        _data = other._data;
        _shapes = other._shapes;
        _strides = other._strides;
        
        other._shape_size = 0;
        other._stride_size = 0;
        other._size = 0;
        other._data = nullptr;
        other._shapes = nullptr;
        other._strides = nullptr;
        return *this;
    }

    SP_FORCEINLINE constexpr tensor& operator+=(const tensor& other) { return add(other); }
    template <typename Other> SP_FORCEINLINE constexpr tensor& operator+=(const Other& other) { return add(other); }
    SP_FORCEINLINE constexpr tensor& operator+=(type_param other) { return add(other); }

    SP_FORCEINLINE constexpr tensor& operator-=(const tensor& other) { return sub(other); }
    template <typename Other> SP_FORCEINLINE constexpr tensor& operator-=(const Other& other) { return sub(other); }
    SP_FORCEINLINE constexpr tensor& operator-=(type_param other) { return sub(other); }

    SP_FORCEINLINE constexpr tensor& operator*=(const tensor& other) { return mul(other); }
    template <typename Other> SP_FORCEINLINE constexpr tensor& operator*=(const Other& other) { return mul(other); }
    SP_FORCEINLINE constexpr tensor& operator*=(type_param other) { return mul(other); }

    SP_FORCEINLINE constexpr tensor& operator/=(const tensor& other) { return div(other); }
    template <typename Other> SP_FORCEINLINE constexpr tensor& operator/=(const Other& other) { return div(other); }
    SP_FORCEINLINE constexpr tensor& operator/=(type_param other) { return div(other); }

    // Will be used for matmul
    //SP_FORCEINLINE constexpr tensor& operator^=(const tensor& other) { matmul(other); }
    //template <typename Other> SP_FORCEINLINE constexpr tensor& operator^=(const Other& other) { matmul(other); }

    SP_FORCEINLINE constexpr tensor operator+(const tensor& other) { tensor result(*this); return result.add(other); }
    template <typename Other> SP_FORCEINLINE constexpr tensor operator+(const Other& other) { tensor result(*this); return result.add(other); }
    SP_FORCEINLINE constexpr tensor operator+(type_param other) { tensor result(*this); return result.add(other); }

    SP_FORCEINLINE constexpr tensor operator-(const tensor& other) { tensor result(*this); return result.sub(other); }
    template <typename Other> SP_FORCEINLINE constexpr tensor operator-(const Other& other) { tensor result(*this); return result.sub(other); }
    SP_FORCEINLINE constexpr tensor operator-(type_param other) { tensor result(*this); return result.sub(other); }

    SP_FORCEINLINE constexpr tensor operator*(const tensor& other) { tensor result(*this); return result.mul(other); }
    template <typename Other> SP_FORCEINLINE constexpr tensor operator*(const Other& other) { tensor result(*this); return result.mul(other); }
    SP_FORCEINLINE constexpr tensor operator*(type_param other) { tensor result(*this); return result.mul(other); }

    SP_FORCEINLINE constexpr tensor operator/(const tensor& other) { tensor result(*this); return result.div(other); }
    template <typename Other> SP_FORCEINLINE constexpr tensor operator/(const Other& other) { tensor result(*this); return result.div(other); }
    SP_FORCEINLINE constexpr tensor operator/(type_param other) { tensor result(*this); return result.div(other); }

    SP_FORCEINLINE constexpr tensor operator>(type_param other) { tensor result(*this); _SP_APPLY_UNROLLED_(_size, result._data[i] = (T)(_data[i] > other)); return result; }

    // SP_FORCEINLINE constexpr tensor operator^(const tensor& other) { tensor result(*this); return result.matmul(other); }
    // template <typename Other> SP_FORCEINLINE constexpr tensor operator^(const Other& other) { tensor result(*this); return result.matmul(other); }


    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Access
    SP_FORCEINLINE constexpr T* data() noexcept { return _data; }
    SP_FORCEINLINE constexpr const T* data() const noexcept { return _data; }
    SP_FORCEINLINE constexpr size_type size() const noexcept { return _size; }
    SP_FORCEINLINE constexpr size_type rank() const noexcept { return _shape_size; }
    SP_FORCEINLINE constexpr size_type shape(size_type i) const { return _shapes[i]; }
    SP_FORCEINLINE constexpr size_type stride(size_type i) const { return _strides[i]; }
    SP_FORCEINLINE constexpr bool empty() const noexcept { return _size == 0; }
    SP_FORCEINLINE constexpr bool is_empty() const noexcept { return _size == 0; }
    SP_FORCEINLINE tensor& print_flat(){
        sp::print("Tensor([");
        SP_IF_EXPECT(_size>0){
            for(ull i = 0; i < _size-1; ++i) sp::print(_data[i],", ");
            sp::print(_data[_size-1]);
        }
        sp::println("])");
        return *this;
    }
    SP_FORCEINLINE tensor& print(){
        return print_flat();
    }

    flat_iterator begin() { return flat_iterator(_data); }
    const flat_iterator begin() const { return const_flat_iterator(_data); }
    const flat_iterator cbegin() const { return const_flat_iterator(_data); }

    flat_iterator end() { return flat_iterator(_data+_size); }
    const flat_iterator end() const { return const_flat_iterator(_data+_size); }
    const flat_iterator cend() const { return const_flat_iterator(_data+_size); }

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Tensor data manipulation
        SP_FORCEINLINE tensor& t(){
            for(size_type i = 0, j = _stride_size-1; i < j; ++i, --j) sp::swap(_strides[i],_strides[j]);
            return *this;
        }
        SP_FORCEINLINE tensor c_t(){
            tensor result(*this);
            return result.t();
        }

        template <typename... Args>
        SP_FORCEINLINE tensor& reshape(Args&&... args){
            size_type new_size = sizeof...(args);
            SP_IF_NOT_EXPECT(new_size==0) throw sp::exceptions::spiral_exception("Reshape() requires arguments, zero were given.");
            size_type new_shape[] = { static_cast<size_type>(args)... };
            ull prod = new_shape[0];
            for(ull i = 1; i < new_size; ++i) prod *= new_shape[i];
            if(prod!=_size) throw sp::exceptions::spiral_exception("Error: reshape() provided mismatching dimensions.");
            if(_shapes) sp::allocator_traits<Allocator<size_type>>::deallocate(_meta_alloc, _shapes, _shape_size);
            if(_strides) sp::allocator_traits<Allocator<size_type>>::deallocate(_meta_alloc, _strides, _stride_size);
            _shapes = sp::allocator_traits<Allocator<size_type>>::allocate(_meta_alloc, new_size);
            _strides = sp::allocator_traits<Allocator<size_type>>::allocate(_meta_alloc, new_size);
            std::memcpy(_shapes,new_shape,new_size*sizeof(size_type));
            _shape_size = new_size;
            _stride_size = new_size;
            compute_strides();
            return *this;
        }
        template <typename... Args>
        SP_FORCEINLINE tensor c_reshape(Args&&... args) { tensor result(*this); return result.reshape(args...); }

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Macro Helper
#define _SP_DEF_TENSOR_MATH_OP_1_(name, other_path, scalar_path, has_simd, simd_name) \
    SP_FORCEINLINE tensor& name(const tensor& other){ \
        SP_IF_CONSTEXPR(sp::simd::supports<T>()&&has_simd&&(spt::get_allocator_alignment<Allocator<T>>()&(sp_cache_line_size-1))==0){ \
            sp::simd::simd_name(_data, other._data, _data, _size); \
        } \
        else \
            SP_PRAGMA_UNROLL \
            _SP_NP_APPLY_UNROLLED_(_size, other_path); \
        return *this; \
    } \
    template <typename Other> SP_FORCEINLINE spt::enable_if_t<SP_HAS_METHOD(Other,data), tensor&> name(const Other& other){ \
        SP_IF_CONSTEXPR ((sp::simd::supports<T>() && \
     spt::is_same_v<T, spt::remove_cvref_t<decltype(*(other.data()))>> && \
     (spt::get_allocator_alignment<Allocator<T>>()&(sp_cache_line_size-1))==0&& \
     has_simd)) \
     { \
            sp::simd::simd_name<T>(_data, other.data(), _data, _size); \
        } \
        else{ \
            SP_PRAGMA_UNROLL \
            _SP_NP_APPLY_UNROLLED_(_size, other_path); } \
        return *this; \
    } \
    SP_FORCEINLINE tensor& name(type_param other){ \
        SP_PRAGMA_UNROLL \
        _SP_NP_APPLY_UNROLLED_(_size, scalar_path); \
        return *this; \
    } \
    \
    SP_FORCEINLINE tensor c_##name(const tensor& other) const { tensor result(*this); return result.name(other); } \
    template <typename Other> SP_FORCEINLINE spt::enable_if_t<SP_HAS_METHOD(Other,data), tensor> c_##name(const Other& other) const { tensor result(*this); return result.name(other); } \
    SP_FORCEINLINE tensor c_##name(type_param other) const { tensor result(*this); return result.name(other); }

#define _SP_DEF_TENSOR_MATH_OP_0_(name, path) \
    SP_FORCEINLINE tensor& name(){ \
        SP_PRAGMA_UNROLL \
        _SP_NP_APPLY_UNROLLED_(_size, path); \
        return *this; \
    } \
    SP_FORCEINLINE tensor c_##name() const { tensor result(*this); return result.name(); }

#define _SP_DEF_SCALAR_REDUCTION_(name, start_val, path, ret) \
    SP_FORCEINLINE T name(){ \
        T result = start_val; \
        _SP_APPLY_UNROLLED_(_size, path); \
        ret; \
    }

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Math Ops

    // Base functions
    _SP_DEF_TENSOR_MATH_OP_1_(add, _data[i] += other.data()[i], _data[i] += other, true, add);
    _SP_DEF_TENSOR_MATH_OP_1_(sub, _data[i] -= other.data()[i], _data[i] -= other, true, sub);
    _SP_DEF_TENSOR_MATH_OP_1_(mul, _data[i] *= other.data()[i], _data[i] *= other, true, mul);
    _SP_DEF_TENSOR_MATH_OP_1_(div, _data[i] /= other.data()[i], _data[i] /= other, true, div);
    // Curved functions
    _SP_DEF_TENSOR_MATH_OP_1_(pow, _data[i] = std::pow(_data[i],other.data()[i]), _data[i] = std::pow(_data[i],other), false, add);
    _SP_DEF_TENSOR_MATH_OP_0_(sqrt, _data[i] = std::sqrt(_data[i]));
    _SP_DEF_TENSOR_MATH_OP_0_(exp, _data[i] = std::exp(_data[i]));
    _SP_DEF_TENSOR_MATH_OP_0_(log, _data[i] = std::log(_data[i]));
    _SP_DEF_TENSOR_MATH_OP_0_(log2, _data[i] = std::log2(_data[i]));
    _SP_DEF_TENSOR_MATH_OP_0_(log10, _data[i] = std::log10(_data[i]));
    // Limiting functions
    _SP_DEF_TENSOR_MATH_OP_0_(abs, _data[i] = sp::abs(_data[i]));
    _SP_DEF_TENSOR_MATH_OP_0_(neg, _data[i] *= -1);
    _SP_DEF_TENSOR_MATH_OP_0_(sign, _data[i] = (_data[i] > 0) - (_data[i] < 0));
    _SP_DEF_TENSOR_MATH_OP_0_(floor, _data[i] = std::floor(_data[i]));
    _SP_DEF_TENSOR_MATH_OP_0_(ceil, _data[i] = std::ceil(_data[i]));
    _SP_DEF_TENSOR_MATH_OP_0_(round, _data[i] = std::round(_data[i]));
    // Activation functions
    _SP_DEF_TENSOR_MATH_OP_0_(relu, _data[i] = sp::max(_data[i], 0));
    _SP_DEF_TENSOR_MATH_OP_0_(sigmoid, _data[i] = (1/(1+std::exp(-_data[i]))));
    _SP_DEF_TENSOR_MATH_OP_0_(tanh, _data[i] = std::tanh(_data[i]));
    _SP_DEF_TENSOR_MATH_OP_0_(gelu, _data[i] = 0.5 * _data[i] * (1.0 + std::erf(_data[i] / std::sqrt(2.0))));
    _SP_DEF_TENSOR_MATH_OP_0_(silu, _data[i] = _data[i] / (1.0 + std::exp(-_data[i])));
    _SP_DEF_TENSOR_MATH_OP_0_(mish, _data[i] = _data[i] * std::tanh(std::log1p(std::exp(_data[i]))));
    _SP_DEF_TENSOR_MATH_OP_0_(elu, _data[i] = _data[i] > 0 ? _data[i] : (std::exp(_data[i]) - 1.0));
    _SP_DEF_TENSOR_MATH_OP_0_(selu, _data[i] = 1.0507 * (_data[i] > 0 ? _data[i] : 1.67326 * (std::exp(_data[i]) - 1.0)));
    _SP_DEF_TENSOR_MATH_OP_0_(softplus, _data[i] = std::log1p(std::exp(_data[i])));
    _SP_DEF_TENSOR_MATH_OP_0_(softsign, _data[i] = _data[i] / (1.0 + std::abs(_data[i])));
    _SP_DEF_TENSOR_MATH_OP_0_(square, _data[i] = _data[i] * _data[i]);
    _SP_DEF_TENSOR_MATH_OP_0_(cube, _data[i] = _data[i] * _data[i] * _data[i]);
    _SP_DEF_TENSOR_MATH_OP_0_(rsqrt, _data[i] = 1.0 / std::sqrt(_data[i]));
    _SP_DEF_TENSOR_MATH_OP_0_(reciprocal, _data[i] = 1.0 / _data[i]);
    _SP_DEF_TENSOR_MATH_OP_0_(hard_sigmoid, _data[i] = sp::min(sp::max(_data[i] + static_cast<T>(3), static_cast<T>(0)), static_cast<T>(6)) / static_cast<T>(6));
    _SP_DEF_TENSOR_MATH_OP_0_(hard_swish, _data[i] = _data[i] * sp::min(sp::max(_data[i] + static_cast<T>(3), static_cast<T>(0)), static_cast<T>(6)) / static_cast<T>(6));
    _SP_DEF_TENSOR_MATH_OP_0_(relu6, _data[i] = sp::min(sp::max(_data[i], static_cast<T>(0)), static_cast<T>(6)));
    SP_FORCEINLINE tensor& leaky_relu(type_param alpha = static_cast<T>(0.01)) { _SP_APPLY_UNROLLED_(_size, _data[i] = _data[i] > 0 ? _data[i] : _data[i] * alpha); return *this; }
    SP_FORCEINLINE tensor c_leaky_relu(type_param alpha = static_cast<T>(0.01)) const { tensor result(*this); return result.leaky_relu(alpha);  }

    SP_FORCEINLINE tensor& clamp(type_param min_val, type_param max_val) { _SP_APPLY_UNROLLED_(_size, _data[i] = sp::min(sp::max(_data[i], min_val), max_val)); return *this; }
    SP_FORCEINLINE tensor c_clamp(type_param min_val, type_param max_val) const { tensor result(*this); return result.clamp(min_val, max_val); }

    SP_FORCEINLINE tensor& softmax(){
        SP_IF_NOT_EXPECT(_size == 0) return *this;
        T max_elem = _data[0];
        for(size_type i = 1; i < _size; ++i) if(_data[i] > max_elem) max_elem = _data[i];
        
        T sum = 0;
        for(size_type i = 0; i < _size; ++i){
            _data[i] = std::exp(_data[i] - max_elem);
            sum += _data[i];
        }
        for(size_type i = 0; i < _size; ++i) _data[i] /= sum;
        return *this;
    }
    SP_FORCEINLINE tensor c_softmax() const { tensor result(*this); return result.softmax(); }

    SP_FORCEINLINE tensor& log_softmax(){
        SP_IF_NOT_EXPECT(_size == 0) return *this;
        T max_elem = _data[0];
        for (size_type i = 1; i < _size; ++i) if (_data[i] > max_elem) max_elem = _data[i];
        
        T sum = 0;
        for (size_type i = 0; i < _size; ++i) sum += std::exp(_data[i] - max_elem);
        T log_sum = std::log(sum);
        
        for (size_type i = 0; i < _size; ++i) _data[i] = (_data[i] - max_elem) - log_sum;
        return *this;
    }
    SP_FORCEINLINE tensor c_log_softmax() const { tensor result(*this); return result.log_softmax(); }

    // Trigonometric functions
    _SP_DEF_TENSOR_MATH_OP_0_(sin, _data[i] = std::sin(_data[i]));
    _SP_DEF_TENSOR_MATH_OP_0_(cos, _data[i] = std::cos(_data[i]));
    _SP_DEF_TENSOR_MATH_OP_0_(tan, _data[i] = std::tan(_data[i]));
    _SP_DEF_TENSOR_MATH_OP_0_(asin, _data[i] = std::asin(_data[i]));
    _SP_DEF_TENSOR_MATH_OP_0_(acos, _data[i] = std::acos(_data[i]));
    _SP_DEF_TENSOR_MATH_OP_0_(atan, _data[i] = std::atan(_data[i]));

    // Hyperbolic functions & inverses
    _SP_DEF_TENSOR_MATH_OP_0_(sinh, _data[i] = std::sinh(_data[i]));
    _SP_DEF_TENSOR_MATH_OP_0_(cosh, _data[i] = std::cosh(_data[i]));
    _SP_DEF_TENSOR_MATH_OP_0_(asinh, _data[i] = std::asinh(_data[i]));
    _SP_DEF_TENSOR_MATH_OP_0_(acosh, _data[i] = std::acosh(_data[i]));
    _SP_DEF_TENSOR_MATH_OP_0_(atanh, _data[i] = std::atanh(_data[i]));

    // Common ML Activation Functions

    // Special Mathematical functions
    _SP_DEF_TENSOR_MATH_OP_0_(erf, _data[i] = std::erf(_data[i]));
    _SP_DEF_TENSOR_MATH_OP_0_(erfc, _data[i] = std::erfc(_data[i]));
    _SP_DEF_TENSOR_MATH_OP_0_(lgamma, _data[i] = std::lgamma(_data[i]));
    _SP_DEF_TENSOR_MATH_OP_0_(tgamma, _data[i] = std::tgamma(_data[i]));

    // Fractional / Floating point utilities
    _SP_DEF_TENSOR_MATH_OP_0_(trunc, _data[i] = std::trunc(_data[i]));
    _SP_DEF_TENSOR_MATH_OP_0_(frac, _data[i] = _data[i] - std::trunc(_data[i]));

    // Scalar reductionss
    _SP_DEF_SCALAR_REDUCTION_(sum, 0, result += _data[i], return result);
    _SP_DEF_SCALAR_REDUCTION_(mean, 0, result += _data[i], return result / _size);
    SP_FORCEINLINE T prod() { SP_IF_NOT_EXPECT(_size==0) return T(); T result = _data[0]; _SP_EXPLICIT_UNROLLED_(i, 1, _size, result *= _data[i]); return result; }
    SP_FORCEINLINE T min() { SP_IF_NOT_EXPECT(_size==0) return T(); T result = _data[0]; _SP_EXPLICIT_UNROLLED_(i, 1, _size, if(_data[i]<result) result = _data[i]); return result; }
    SP_FORCEINLINE T max() { SP_IF_NOT_EXPECT(_size==0) return T(); T result = _data[0]; _SP_EXPLICIT_UNROLLED_(i, 1, _size, if(_data[i]>result) result = _data[i]); return result; }
    SP_FORCEINLINE T argmin() { SP_IF_NOT_EXPECT(_size==0) return T(); T tracker = _data[0]; T result = 0; _SP_EXPLICIT_UNROLLED_(i, 1, _size, if(_data[i]<tracker) { tracker = _data[i]; result = i; }); return result; }
    SP_FORCEINLINE T argmax() { SP_IF_NOT_EXPECT(_size==0) return T(); T tracker = _data[0]; T result = 0; _SP_EXPLICIT_UNROLLED_(i, 1, _size, if(_data[i]>tracker) { tracker = _data[i]; result = i; }); return result; }

    template <typename Other>
    SP_FORCEINLINE tensor matmul(const Other& other) const {
        // 1. Basic Dimension Validation
        // Assuming your class has a way to access shape at a specific dimension
        // For now, we assume 2D tensors and use index 0 as rows, 1 as cols
        size_t M = _shapes[0]; 
        size_t K_this = _shapes[1];
        size_t K_other = other._shapes[0];
        size_t N = other._shapes[1];

        // In a real library, you'd throw an error or assert here:
        if(K_this != K_other) throw std::runtime_error("Dimension mismatch");

        // 2. Initialize result tensor with shape (M, N)
        tensor result(M, N); // Assumes constructor exists: tensor(dims...)

        // 3. The Computation (Naive Triple Loop)
        // We use strides to ensure this works even if the input tensors are views/slices
        for (size_t i = 0; i < M; ++i) {
            for (size_t j = 0; j < N; ++j) {
                T sum = 0;
                for (size_t k = 0; k < K_this; ++k) {
                    // Indexing via strides: base + (row * row_stride) + (col * col_stride)
                    T val_a = _data[i * _strides[0] + k * _strides[1]];
                    T val_b = other._data[k * other._strides[0] + j * other._strides[1]];
                    sum += val_a * val_b;
                }
                result._data[i * result._strides[0] + j * result._strides[1]] = sum;
            }
        }

        return result;
    }

}; // class tensor

}; // namespace sp

#include "../container-funcs/tensor-funcs.hpp"
#endif