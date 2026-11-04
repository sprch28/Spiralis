#ifndef ____SP_IO_CONSTANTS____
#define ____SP_IO_CONSTANTS____
#pragma once
#include "../../setup/init.hpp"
#include "../../core/type_traits.hpp"
#include "../../math/int128.hpp"
#include "../../containers/pair.hpp"
#include "../../containers/string.hpp"

//#include <unistd.h>
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

namespace detail{
const char digit_pairs[201] = 
    "0001020304050607080910111213141516171819"
    "2021222324252627282930313233343536373839"
    "4041424344454647484950515253545556575859"
    "6061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";
}


} // namespace sp
#endif // ____SP_IO_CONSTANTS____