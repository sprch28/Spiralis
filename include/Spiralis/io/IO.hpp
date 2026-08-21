#ifndef ____SP_IO_SETUP____
#define ____SP_IO_SETUP____
#pragma once
#include "../setup/init.hpp"
#include "../core/type_traits.hpp"
#include "../math/int128.hpp"
#include "../containers/pair.hpp"
#include "../containers/string.hpp"

#include <unistd.h>
#include <cstdio>
#include <fcntl.h>
#include <stdexcept>
#include <cstring>

namespace sp{
enum /*class*/ io{ 
    flush, // Flush the buffer
    endl, // End line and flush buffer
    nl, // End line without flushing
    boolalpha, // Boolalpha: changes bool 1 and 0 to true and false
    nboolalpha, // Disable boolalpha
    hrule, // Horizontal rule
    hline, // Horizontal line
    sep // ", "
};

enum /*class*/ file_mode {
    write,
    append,
    read, 
    rw
};

const char digit_pairs[201] = 
    "0001020304050607080910111213141516171819"
    "2021222324252627282930313233343536373839"
    "4041424344454647484950515253545556575859"
    "6061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";


class file{
private:
    int _fd;
    const char* _filename;
    file_mode _mode = file_mode::read;
public:
    file(const char* filename, file_mode mode = file_mode::read, mode_t m = 0644) : _mode(mode), _filename(filename){
        int flags = 0;
        switch (_mode) {
            case file_mode::write:
                flags = O_WRONLY | O_CREAT | O_TRUNC;
                break;
            case file_mode::append:
                flags = O_WRONLY | O_CREAT | O_APPEND;
                break;
            case file_mode::read:
                flags = O_RDONLY;
                break;
            case file_mode::rw:
                flags = O_RDWR | O_CREAT;
                break;
        }
        _fd = ::open(filename, flags, m);
        SP_IF_NOT_EXPECT(_fd == -1) throw std::runtime_error("Failed to open file");
    }
    file(const file&) = delete;
    file& operator=(const file&) = delete;
    file(file&& other) noexcept : _fd(other._fd) { other._fd = -1; }
    file& operator=(file&& other) noexcept {
        SP_IF_EXPECT(this != &other){ 
            if(_fd != -1) ::close(_fd); 
            _fd = other._fd; 
            other._fd = -1; 
        }
        return *this;
    }

    SP_FORCEINLINE file& change_mode(file_mode mode, mode_t m = 0644) {
        _mode = mode;
        SP_IF_NOT_EXPECT(_fd != -1) ::close(_fd);
        int flags = 0;
        switch (mode) {
            case file_mode::write:
                flags = O_WRONLY | O_CREAT | O_TRUNC;
                break;
            case file_mode::append:
                flags = O_WRONLY | O_CREAT | O_APPEND;
                break;
            case file_mode::read:
                flags = O_RDONLY;
                break;
            case file_mode::rw:
                flags = O_RDWR | O_CREAT;
                break;
        }
        _fd = ::open(_filename, flags, m);
        SP_IF_NOT_EXPECT(_fd == -1) throw std::runtime_error("Failed to open file");
        return *this;
    }

    SP_FORCEINLINE file& close(){
        SP_IF_EXPECT(_fd != -1) {
            ::close(_fd);
            _fd = -1;
        }
        return *this;
    }

    SP_FORCEINLINE file_mode get_mode() const { return _mode; }
    SP_FORCEINLINE file& rewind() { ::lseek(_fd, 0, SEEK_SET); return *this; }
    SP_FORCEINLINE file& rewind_to(ull idx=0) { ::lseek(_fd, idx, SEEK_SET); return *this; }
    SP_FORCEINLINE int get_fd() const { return _fd; }

    ~file() { close(); }
};

class IO{
private:
class __I_Backend{
private:
    static constexpr int _SIZE = __SP_IO_BUFFER_SIZE__;
    
    alignas(sp_cache_line_size) char _buf[_SIZE];
    
    ull _head = 0; // Current position reading out of the buffer
    ull _tail = 0; // End of valid data currently populated in the buffer
public:
    int _fd = 0;

    SP_FORCEINLINE void setFd(int fd) { _fd = fd; }
    
    SP_COLD bool refillBuffer() {
        ssize_t n = ::read(_fd, _buf, _SIZE);
        if (n <= 0) return false;
        _head = 0;
        _tail = static_cast<ull>(n);
        return true;
    }

    SP_FORCEINLINE int readChar(char* dest) {
        SP_IF_NOT_EXPECT(_head == _tail) {
            SP_IF_NOT_EXPECT(!refillBuffer()) return -1;
        }
        *dest = _buf[_head++];
        return 1;
    }
    SP_FORCEINLINE ull readLine(char* dest, ull max_size = 2048) {
        ull i = 0;
        char c;
        while (i < max_size - 1) {
            SP_IF_NOT_EXPECT(readChar(&c) == -1) break; 
            else SP_IF_NOT_EXPECT(c == '\n') break;
            dest[i++] = c;
        }
        
        dest[i] = '\0';
        return i; // Return the length of the string read (excluding \0)
    }

    SP_FORCEINLINE ull readBinary(void* dest, ull bytes) {
        char* dst = static_cast<char*>(dest);
        ull total_read = 0;

        while (bytes > 0) {
            // If the buffer is empty, refill it
            if (_head == _tail) {
                ssize_t n = ::read(_fd, _buf, _SIZE);
                SP_IF_NOT_EXPECT(n <= 0) break; // EOF or error
                _head = 0;
                _tail = static_cast<ull>(n);
            }

            ull available = _tail - _head;
            ull to_read = (bytes < available) ? bytes : available;

            memcpy(dst + total_read, _buf + _head, to_read);
            _head += to_read;
            total_read += to_read;
            bytes -= to_read;
        }

        return total_read; // Return actual bytes read (useful for EOF tracking)
    }
    ~__I_Backend() {  }
} __sp_backend_i; // class __I_Backend


class __O_Backend{
private:
    static constexpr int _SIZE = __SP_IO_BUFFER_SIZE__;
    alignas(sp_cache_line_size) char _buf[_SIZE];
    ull _idx = 0;
public:
int _fd = 1;
bool _boolalpha=false;

    ~__O_Backend() { flush();  }
    SP_FORCEINLINE void flush(){
        SP_IF_EXPECT(_idx){
            ::write(_fd, _buf, _idx);
            _idx = 0;
        }
    }

    SP_FORCEINLINE void setFd(int fd) { flush(); _fd = fd; }

    SP_FORCEINLINE void writeChar(char c){
        SP_IF_NOT_EXPECT(_idx == _SIZE) flush();
        _buf[_idx++] = c;
    }

    SP_FORCEINLINE void writeBinary(const void* data, ull bytes) {
        SP_IF_NOT_EXPECT(bytes >= _SIZE) {
            flush();
            ::write(_fd, data, bytes);
            return;
        }

        const char* src = static_cast<const char*>(data);
        while (bytes > 0) {
            ull space_left = _SIZE - _idx;
            SP_IF_NOT_EXPECT(space_left == 0) {
                flush();
                space_left = _SIZE;
            }
            ull to_write = (bytes < space_left) ? bytes : space_left;
            memcpy(_buf + _idx, src, to_write);
            _idx += to_write;
            src += to_write;
            bytes -= to_write;
        }
    }

    void writeLL(ll n){
        SP_IF_NOT_EXPECT(n == 0) { writeChar('0'); return; }
        if (n < 0) { writeChar('-'); n = -n; }
        char local_buf[24];
        int local_idx = 24; // Start from the end of the buffer
        while (n >= 100) {
            int r = n % 100;
            n /= 100;
            local_buf[--local_idx] = digit_pairs[(r * 2) + 1];
            local_buf[--local_idx] = digit_pairs[r * 2];
        }
        SP_IF_EXPECT(n >= 10) {
            local_buf[--local_idx] = digit_pairs[(n * 2) + 1];
            local_buf[--local_idx] = digit_pairs[n * 2];
        }else{
            local_buf[--local_idx] = n + '0';
        }
        while(local_idx < 24) {
            writeChar(local_buf[local_idx++]);
        }
    }

    void writeULL(ull n){
        SP_IF_NOT_EXPECT(n == 0) { writeChar('0'); return; }
        char local_buf[24];
        int local_idx = 24; // Start from the end of the buffer
        while (n >= 100) {
            int r = n % 100;
            n /= 100;
            local_buf[--local_idx] = digit_pairs[(r * 2) + 1];
            local_buf[--local_idx] = digit_pairs[r * 2];
        }
        SP_IF_EXPECT(n >= 10) {
            local_buf[--local_idx] = digit_pairs[(n * 2) + 1];
            local_buf[--local_idx] = digit_pairs[n * 2];
        }else{
            local_buf[--local_idx] = n + '0';
        }
        while(local_idx < 24) {
            writeChar(local_buf[local_idx++]);
        }
    }

    void writeDouble(double d){
        char local_buf[32];
        int len = std::snprintf(local_buf, 32, "%lf", d); // placeholder until i implement a better function
        for(int i = 0; i < len; ++i){
            writeChar(local_buf[i]);
        }
    }

    SP_FORCEINLINE SP_FLATTEN void writeString(const char* s){
        ull len = sp::strlen(s);
        SP_IF_NOT_EXPECT(len+_idx >= _SIZE) {
            flush();
            ::write(_fd,s,len);
            return;
        }
        memcpy(_buf+_idx, s, len);
        _idx += len;
    }

    void writeU128(uint128 n) {
        SP_IF_NOT_EXPECT(n == 0) { writeChar('0'); return; }
        char local_buf[48];
        int local_idx = 48;
        while (n >= 100) {
            int r = static_cast<int>(n % 100);
            n /= 100;
            local_buf[--local_idx] = digit_pairs[(r * 2) + 1];
            local_buf[--local_idx] = digit_pairs[r * 2];
        }
        
        if(n >= 10){
            int r = static_cast<int>(n);
            local_buf[--local_idx] = digit_pairs[(r * 2) + 1];
            local_buf[--local_idx] = digit_pairs[r * 2];
        }else if(n > 0){
            local_buf[--local_idx] = static_cast<char>(n) + '0';
        }

        while(local_idx < 48){
            writeChar(local_buf[local_idx++]);
        }
    }

    void write128(int128 n) {
        if(n < 0){
            writeChar('-');
            writeU128(static_cast<uint128>(-n)); 
        }else{
            writeU128(static_cast<uint128>(n));
        }
    }
} __sp_backend_o; // class __O_Backend
public:
    IO(){}
    IO(const sp::file& f){
        if(f.get_mode()==file_mode::read) __sp_backend_o.setFd(1);
        else __sp_backend_o.setFd(f.get_fd());
        __sp_backend_i.setFd(f.get_fd());
    }
    ~IO(){
        __sp_backend_o.flush();
    }
    SP_FORCEINLINE IO& rewind_input() { if(__sp_backend_i._fd!=0&&__sp_backend_i._fd!=1&&__sp_backend_i._fd!=2) ::lseek(__sp_backend_i._fd, 0, SEEK_SET); return *this; }
    SP_FORCEINLINE IO& rewind_output() { if(__sp_backend_o._fd!=0&&__sp_backend_o._fd!=1&&__sp_backend_o._fd!=2) ::lseek(__sp_backend_o._fd, 0, SEEK_SET); return *this; }
    SP_FORCEINLINE IO& rewind() { rewind_input(); rewind_output(); return *this;}
    SP_FORCEINLINE IO& flush() { __sp_backend_o.flush(); return *this; }
    SP_FORCEINLINE IO& print() { return *this; }
    SP_FORCEINLINE IO& print(char msg) { __sp_backend_o.writeChar(msg); return *this; }
    SP_FORCEINLINE IO& print(const char* msg){ __sp_backend_o.writeString(msg); return *this; }
    SP_FORCEINLINE IO& print(int msg) { __sp_backend_o.writeLL(msg); return *this; }
    SP_FORCEINLINE IO& print(ull msg) { __sp_backend_o.writeULL(msg); return *this; }
    SP_FORCEINLINE IO& print(long long msg) { __sp_backend_o.writeLL(msg); return *this; }
    SP_FORCEINLINE IO& print(double msg) { __sp_backend_o.writeDouble(msg); return *this; }
    SP_FORCEINLINE IO& print(float msg) { __sp_backend_o.writeDouble(static_cast<double>(msg)); return *this; }
    SP_FORCEINLINE IO& print(bool msg) {
        if (__sp_backend_o._boolalpha) __sp_backend_o.writeString(msg ? "true" : "false");
        else __sp_backend_o.writeChar(msg ? '1' : '0');
        return *this;
    }
    SP_FORCEINLINE IO& print(uint128 msg) { __sp_backend_o.writeU128(msg); return *this; }
    SP_FORCEINLINE IO& print(int128 msg) { __sp_backend_o.write128(msg); return *this; }
    template <typename T> SP_FORCEINLINE spt::enable_if_t<spt::has_getSpiralMessage_v<T>, IO&> print(const T& msg) { this->print(msg.__getSpiralMessage()); return *this;}
    template <typename T> SP_FORCEINLINE spt::enable_if_t<spt::has_getSpiralMessage_v<T>, IO&> println(const T& msg) { this->print(msg.__getSpiralMessage()); this->print(sp::io::endl); return *this;}
    template <typename T> SP_FORCEINLINE spt::enable_if_t<!spt::has_getSpiralMessage_v<T>&&SP_HAS_METHOD(T, c_str), IO&> print(const T& msg) { this->print(msg.c_str()); return *this; }
    template <typename T> SP_FORCEINLINE spt::enable_if_t<!spt::has_getSpiralMessage_v<T>&&SP_HAS_METHOD(T, c_str), IO&> println(const T& msg) { this->print(msg.c_str()); this->print(sp::io::endl); return *this; }
    template <typename T> SP_FORCEINLINE spt::enable_if_t<!spt::has_getSpiralMessage_v<T>&&!SP_HAS_METHOD(T,c_str)&&SP_HAS_METHOD(T,to_string), IO&> print(const T& msg) { sp::string temp = msg.to_string(); this->print(temp.c_str()); return *this;}
    template <typename T> SP_FORCEINLINE spt::enable_if_t<!spt::has_getSpiralMessage_v<T>&&!SP_HAS_METHOD(T,c_str)&&SP_HAS_METHOD(T,to_string), IO&> println(const T& msg) { sp::string temp = msg.to_string(); this->print(temp.c_str()); this->print(sp::io::endl); return *this;}
    template <typename T> SP_FORCEINLINE spt::enable_if_t<!spt::has_getSpiralMessage_v<T>&&!SP_HAS_METHOD(T, c_str)&&!SP_HAS_METHOD(T,to_string), IO&> print(T&& msg) { __sp_backend_o.writeString(msg); return *this; }

    template <typename Arg1, typename Arg2, typename... Args> 
    SP_FORCEINLINE IO& print(Arg1&& arg1, Arg2&& arg2, Args&&... args){
        this->print(sp::forward<Arg1>(arg1));
        this->print(sp::forward<Arg2>(arg2));
        (this->print(sp::forward<Args>(args)), ...);
        return *this;
    }

    template <typename... Args> 
    SP_FORCEINLINE IO& println(Args&&... args){
        this->print(sp::forward<Args>(args)...);
        this->print(sp::io::endl);
        return *this;
    }

    SP_FORCEINLINE IO& print(enum io trait){
        if(trait==io::flush) {  flush(); } 
        else if(trait==io::endl) { __sp_backend_o.writeChar('\n'); flush(); }
        else if(trait==io::nl) { __sp_backend_o.writeChar('\n'); }
        else if(trait==io::boolalpha) { __sp_backend_o._boolalpha = true; }
        else if(trait==io::nboolalpha) { __sp_backend_o._boolalpha = false; }
        else if(trait==io::hrule) { __sp_backend_o.writeString("--------------------\n"); }
        else if(trait==io::hline) { __sp_backend_o.writeString("--------------------"); }
        else if(trait==io::sep) { __sp_backend_o.writeChar(','); __sp_backend_o.writeChar(' '); }
        return *this;
    }

    SP_FORCEINLINE IO& output_to_console(){ __sp_backend_o.flush(); __sp_backend_o._fd = 1; return *this;}
    SP_FORCEINLINE IO& output_to_cerr(){ __sp_backend_o.flush(); __sp_backend_o._fd = 2; return *this; }
    SP_FORCEINLINE IO& output_to_file(const sp::file& f){ if(f.get_mode()!=file_mode::read) { __sp_backend_o.flush(); __sp_backend_o._fd = f.get_fd(); } return *this; }

    SP_FORCEINLINE IO& input_to_console(){ __sp_backend_i._fd = 0; return *this;}
    SP_FORCEINLINE IO& input_to_cerr(){ __sp_backend_i._fd = 2; return *this; }
    SP_FORCEINLINE IO& input_to_file(const sp::file& f){ __sp_backend_i._fd = f.get_fd(); return *this; }

    SP_FORCEINLINE IO& to_console(){ return output_to_console().input_to_console(); }
    SP_FORCEINLINE IO& to_cerr(){ return output_to_cerr().input_to_cerr(); }
    SP_FORCEINLINE IO& to_file(const sp::file& f){ return output_to_file(f).input_to_file(f); }
    SP_FORCEINLINE sp::string /*sp::pair<sp::string, bool>*/ getLine() {
        sp::string result;
        char c;
        //bool seen_input = false;
        while(__sp_backend_i.readChar(&c) != -1) {
            //seen_input = true;
            SP_IF_NOT_EXPECT(c==(char)13||c==(char)10) break;
            result += c;
        }
        return result; //{result, seen_input};
    }

    template <bool include_nl=false>
    SP_FORCEINLINE bool getLine(sp::string& str){
        char c;
        bool seen_input = false;
        while(__sp_backend_i.readChar(&c)!=-1){
            seen_input = true;
            if constexpr(include_nl){
                str.push_back(c);
                SP_IF_NOT_EXPECT(c==(char)13||c==(char)10) break;
            }else{
                SP_IF_NOT_EXPECT(c==(char)13||c==(char)10) break;
                str.push_back(c);
            }
        }
        return seen_input;
    }

    // ==================================================== BINARY I/O =====================================================

    SP_FORCEINLINE IO& write(const void* ptr, ull bytes) {
        __sp_backend_o.writeBinary(ptr, bytes);
        return *this;
    }
    template <typename T>
    SP_FORCEINLINE IO& write(const T& value) {
        if constexpr(spt::is_trivially_copyable_v<T>){
            __sp_backend_o.writeBinary(sp::addressof(value), sizeof(T));
        }else if constexpr(spt::has_getSpiralBinary_v<T>){
            auto [data, size] = value.__getSpiralBinary();
            __sp_backend_o.writeBinary(data, size);
        }else if constexpr(spt::has_contiguous_storage_v<T>){
            auto dta = value.data();
            ull size = sizeof(spt::remove_reference_t<decltype(*dta)>) * value.size();
            __sp_backend_o.writeBinary(dta, size);
        }else{
            static_assert(spt::is_trivially_copyable_v<T> || spt::has_getSpiralBinary_v<T>, 
                        "Type T is unsupported: must be trivially copyable or define __getSpiralBinary()");
        }
        return *this;
    }
    template <typename... Args>
    SP_FORCEINLINE IO& write(const Args&... args) {
        (write(args), ...);
        return *this;
    }

    SP_FORCEINLINE ull read(void* ptr, ull bytes) {
        return __sp_backend_i.readBinary(ptr, bytes);
    }
    template <typename T>
    SP_FORCEINLINE bool read(T& value) {
        if constexpr(spt::is_trivially_copyable_v<T>){
            ull bytes_read = __sp_backend_i.readBinary(sp::addressof(value), sizeof(T));
            return bytes_read == sizeof(T);
        }else if constexpr(spt::has_getSpiralBinary_v<T>){
            auto [data, size] = value.__getSpiralBinary();
            ull bytes_read = __sp_backend_i.readBinary((void*)data, size);
            return bytes_read == size; 
        }else if constexpr(SP_HAS_METHOD(T, data)) {
            if constexpr(SP_HAS_METHOD(T, size)) {
                auto dta = value.data();
                ull size = sizeof(spt::remove_reference_t<decltype(*dta)>) * value.size();
                ull bytes_read = __sp_backend_i.readBinary((void*)dta, size);
                return bytes_read == size;
            } else {
                static_assert(false, "Target data type for read() does not have required method size().");
                return false;
            }
        }else{
            static_assert(spt::is_trivially_copyable_v<T> || spt::has_getSpiralBinary_v<T>, 
            "Type T is unsupported: must be trivially copyable or define __getSpiralBinary()");
            return false;
        }
    }
    template <typename T>
    SP_FORCEINLINE sp::pair<T, bool> read(){
        T value;
        SP_IF_EXPECT(read(value)) return {value, true};
        else return {T(), false};
    }
    template <typename T, typename... Args>
    SP_FORCEINLINE bool read(T& first, Args&... args){
        return (read(first)) && (read(args) && ...);
    }
}; // class IO
inline IO io;

template <typename... Args>
SP_FORCEINLINE void print(Args&&... args){
    io.print(sp::forward<Args>(args)...);
}
template <typename... Args>
SP_FORCEINLINE void println(Args&&... args){
    io.print(sp::forward<Args>(args)...);
    io.print(sp::io::endl);
}
SP_FORCEINLINE void flush_io() { io.flush(); }

};
#endif