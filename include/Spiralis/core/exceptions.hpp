#ifndef ____SP_EXCEPTIONS____
#define ____SP_EXCEPTIONS____
#pragma once
#include "../io/console.hpp"
#include "../setup/init.hpp"
#include <exception>
#include <string>
#include <iostream>

namespace sp{
namespace exceptions{

class spiralis_exception : public std::exception{
protected:
    std::string message;
    int errorCode;

public:
    spiralis_exception(const char* msg, int code = 0) 
        : message(msg ? msg : "Unknown Spiral Error"), errorCode(code) {}

    explicit spiralis_exception(int code) 
        : message("Spiral Error"), errorCode(code) {}

    const char* what() const noexcept override {
        return message.c_str();
    }

    int code() const noexcept { 
        return errorCode; 
    }

    void print() const {
        std::cout << sp::console::FG_BRIGHT_RED << message 
                  << sp::console::RESET_EFFECTS << std::endl;
    }
};

#if defined(__SP_BENCHMARK__)
    class TestAssertionException : public spiralis_exception{
        using spiralis_exception::spiralis_exception;
    };
    class TestFailException : public spiralis_exception{
        using spiralis_exception::spiralis_exception;
    };
#endif

class FileException : public spiralis_exception {
    using spiralis_exception::spiralis_exception;
};

class StringException : public spiralis_exception {
    using spiralis_exception::spiralis_exception;
};

class StringAccessException : public StringException {
    using StringException::StringException;
};

class StringViewException : public StringException {
    using StringException::StringException;
};

class MapException : public spiralis_exception {
    using spiralis_exception::spiralis_exception;
};

class MapConstructException : public MapException {
    using MapException::MapException;
};

class ArrayException : public spiralis_exception {
    using spiralis_exception::spiralis_exception;
};

class TensorException : public spiralis_exception {
    using spiralis_exception::spiralis_exception;
};

class TensorSizeError : public TensorException {
public:
    TensorSizeError(const char* msg = "Tensor size mismatch.", int code = 0)
        : TensorException(msg, code) {}
};

} // namespace exceptions
} // namespace sp

#endif // ____SP_EXCEPTIONS____