#ifndef ____SP_IO_FILE____
#define ____SP_IO_FILE____
#pragma once
namespace sp{
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
}
#endif