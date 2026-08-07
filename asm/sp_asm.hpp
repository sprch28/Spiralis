#ifndef ____SP_ASM____
#define ____SP_ASM____
#pragma once

#include "../setup/init.hpp"

namespace sp {
extern "C" {
    // Basic Process & System Management
    [[noreturn]] void sp_exit(int status);
    ull sp_fork(void);
    ull sp_getpid(void);
    ull sp_kill(ull pid, ull sig);

    // File I/O & Descriptor Operations
    ull sp_read(ull fd, void* buf, ull count);
    ull sp_write(ull fd, const void* buf, ull count);
    ull sp_open(const char* path, ull flags, ull mode);
    ull sp_close(ull fd);
    ull sp_unlink(const char* path);
    ull sp_dup(ull fd);
    ull sp_pipe(int* fds);

    // Memory Management
    void* sp_mmap(void* addr, ull length, ull prot, ull flags, ull fd, ull offset);
    ull sp_munmap(void* addr, ull length);
} asm(".include \"Spiral/asm/spiral-asm.s\"");
}

#endif