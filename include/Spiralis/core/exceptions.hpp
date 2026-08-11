#ifndef ____SP_EXCEPTIONS____
#define ____SP_EXCEPTIONS____
#pragma once
#include "../system/console.hpp"
#include "../setup/init.hpp"
#include <exception>
#include <string>
#include <iostream>

namespace sp{
namespace exceptions{

class spiral_exception : public std::exception{
protected:
    std::string message;
    int errorCode;

public:
    spiral_exception(const char* msg, int code = 0) 
        : message(msg ? msg : "Unknown Spiral Error"), errorCode(code) {}

    explicit spiral_exception(int code) 
        : message("Spiral Error"), errorCode(code) {}

    const char* what() const noexcept override {
        return message.c_str();
    }

    int code() const noexcept { 
        return errorCode; 
    }

    // Handle color printing at the display layer instead of polluting what()
    void print() const {
        std::cout << sp::console::FG_BRIGHT_RED << message 
                  << sp::console::RESET_EFFECTS << std::endl;
    }
};

// --- Specific Exception Domains ---

class FileException : public spiral_exception {
    using spiral_exception::spiral_exception;
};

class StringException : public spiral_exception {
    using spiral_exception::spiral_exception;
};

class StringAccessException : public StringException {
    using StringException::StringException;
};

class StringViewException : public StringException {
    using StringException::StringException;
};

class MapException : public spiral_exception {
    using spiral_exception::spiral_exception;
};

class MapConstructException : public MapException {
    using MapException::MapException;
};

class ArrayException : public spiral_exception {
    using spiral_exception::spiral_exception;
};

class TensorException : public spiral_exception {
    using spiral_exception::spiral_exception;
};

class TensorSizeError : public TensorException {
public:
    TensorSizeError(const char* msg = "Tensor size mismatch.", int code = 0)
        : TensorException(msg, code) {}
};

} // namespace exceptions
} // namespace sp

#endif // ____SP_EXCEPTIONS____