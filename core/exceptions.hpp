#ifndef ____SP_EXCEPTIONS____
#define ____SP_EXCEPTIONS____
#pragma once
#include "../system/console.hpp"
#include "../setup/init.hpp"
#include <exception>
#include <string>

namespace sp{

namespace exceptions{
class FileException : public std::exception{
    public:
    std::string message;
    int errorCode;

    FileException(const char* msg, int code = 0){
        message = std::string(sp::console::FG_BRIGHT_RED) + 
                    std::string(msg)+
                    sp::console::RESET_EFFECTS;
        errorCode = code;
    }

    FileException(int code = 0) : message("File Error"), errorCode(code){}

    const char* what() const noexcept override{
        return message.c_str();
    }
};

class StringException : public std::exception{
    public:
    std::string message;
    int errorCode;

    StringException(const char* msg, int code = 0){
        message = std::string(sp::console::FG_BRIGHT_RED) + 
                    std::string(msg)+
                    sp::console::RESET_EFFECTS;
        errorCode = code;
    }

    const char* what() const noexcept override{
        return message.c_str();
    }
};

class StringAccessException : public StringException{
    using StringException::StringException;
};

class StringViewException : public StringException{
    using StringException::StringException;
};

class MapException : public std::exception{
    public:
    std::string message;
    int errorCode;

    MapException(const char* msg, int code = 0){
        message = std::string(sp::console::FG_BRIGHT_RED) + 
                    std::string(msg)+
                    sp::console::RESET_EFFECTS;
        errorCode = code;
    }

    const char* what() const noexcept override{
        return message.c_str();
    }
};

class MapConstructException : public MapException{
    // inherit constructor and functions
    using MapException::MapException;
};

class ArrayException : public std::exception{
    public:
    std::string message;
    int errorCode;

    ArrayException(const char* msg, int code = 0){
        message = std::string(sp::console::FG_BRIGHT_RED) + 
                    std::string(msg)+
                    sp::console::RESET_EFFECTS;
        errorCode = code;
    }

    const char* what() const noexcept override{
        return message.c_str();
    }
};

class TensorException : public std::exception {
public:
    std::string message;
    int errorCode;

    TensorException(const char* msg, int code = 0) {
        message = std::string(sp::console::FG_BRIGHT_RED) +
                  std::string(msg) + 
                  sp::console::RESET_EFFECTS;
        errorCode = code;
    }

    const char* what() const noexcept override {
        return message.c_str();
    }
};



class TensorSizeError : public std::exception {
private:
    std::string message;
    
public:
    TensorSizeError(const char* msg = nullptr) {
        if (msg) {
            message = msg;
        } else {
            message = std::string(sp::console::FG_BRIGHT_RED) + 
                      "Tensor size mismatch." + 
                      sp::console::RESET_EFFECTS;
        }
    }
    
    const char* what() const noexcept override {
        return message.c_str();
    }
};


} // namespace exceptions
} // namespace sp

#endif // ____SP_EXCEPTIONS____