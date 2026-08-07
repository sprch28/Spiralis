#ifndef ____SP_ASM_GEN____
#define ____SP_ASM_GEN____
#pragma once
#include <sys/types.h>
#include <sys/sysctl.h>
#include <string>
#include "../core/IO.hpp"
#include "../collections/string.hpp"
#include "../collections/hash_map.hpp"
namespace sp{
sp::string get_kernel_version() {
    char buffer[256];
    size_t size = sizeof(buffer);
    
    // "kern.osrelease" returns strings like "23.5.0"
    if (sysctlbyname("kern.osrelease", buffer, &size, NULL, 0) == 0) {
        return sp::string(buffer);
    }
    return "";
}

int get_darwin_major_version() {
    std::string ver = get_kernel_version().c_str();
    SP_IF_NOT_EXPECT(ver.empty()) return -1;
    // Parse the major release number before the first '.'
    return std::stoi(ver.substr(0, ver.find('.')));
}

struct SyscallTable {
    int exit_num;      // SYS_exit      (1)
    int fork_num;      // SYS_fork      (2)
    int read_num;      // SYS_read      (3)
    int write_num;     // SYS_write     (4)
    int open_num;      // SYS_open      (5)
    int close_num;     // SYS_close     (6)
    int unlink_num;    // SYS_unlink    (10)
    int getpid_num;    // SYS_getpid    (20)
    int kill_num;      // SYS_kill      (37)
    int dup_num;       // SYS_dup       (41)
    int pipe_num;      // SYS_pipe      (42)
    int mmap_num;      // SYS_mmap      (197)
    int munmap_num;    // SYS_munmap    (191)
};

// Standard BSD system call mappings for Darwin / XNU
constexpr SyscallTable default_sys_call_table = {
    1,   // exit
    2,   // fork
    3,   // read
    4,   // write
    5,   // open
    6,   // close
    10,  // unlink
    20,  // getpid
    37,  // kill
    41,  // dup
    42,  // pipe
    197, // mmap
    191  // munmap
};


// USING sp::hash_map UNTIL sp::hash_map IS FIXED

// Map Darwin Major Version -> Syscall Table
// Core BSD system call numbers remain ABI-stable across macOS releases.
const sp::hash_map<int, SyscallTable> KERNEL_TABLES = {
    // Darwin 18 (macOS 10.14 Mojave)
    {18, default_sys_call_table},
    // Darwin 19 (macOS 10.15 Catalina)
    {19, default_sys_call_table},
    // Darwin 20 (macOS 11 Big Sur - First ARM64 macOS)
    {20, default_sys_call_table},
    // Darwin 21 (macOS 12 Monterey)
    {21, default_sys_call_table},
    // Darwin 22 (macOS 13 Ventura)
    {22, default_sys_call_table},
    // Darwin 23 (macOS 14 Sonoma)
    {23, default_sys_call_table},
    // Darwin 24 (macOS 15 Sequoia)
    {24, default_sys_call_table},
    // Darwin 25 (macOS 16 Tahoe)
    {25, default_sys_call_table}
};

#define _SP_GENERATE_STANDARD_ASM_(syscall_name) \
scanner.println( \
    "_sp_", #syscall_name, ":\n" \
    "\tmov x16, #", table.syscall_name##_num, "\n" \
    "\tsvc #0x80\n" \
    "\tret\n" \
)

void generate_asm_file(const sp::string& filename, const SyscallTable& table) {
    sp::file asm_file(filename.c_str(),sp::write);
    sp::IO scanner(asm_file);
    scanner.println(
        "; Generated ARM64 Assembly Library for host macOS\n"
        ".global _sp_exit\n"
        ".global _sp_fork\n"
        ".global _sp_read\n"
        ".global _sp_write\n"
        ".global _sp_open\n"
        ".global _sp_close\n"
        ".global _sp_unlink\n"
        ".global _sp_getpid\n"
        ".global _sp_kill\n"
        ".global _sp_dup\n"
        ".global _sp_pipe\n"
        ".global _sp_mmap\n"
        ".global _sp_munmap\n"
        ".align 4\n"
    );

    _SP_GENERATE_STANDARD_ASM_(exit);
    _SP_GENERATE_STANDARD_ASM_(fork);
    _SP_GENERATE_STANDARD_ASM_(read);
    _SP_GENERATE_STANDARD_ASM_(write);
    _SP_GENERATE_STANDARD_ASM_(open);
    _SP_GENERATE_STANDARD_ASM_(close);
    _SP_GENERATE_STANDARD_ASM_(unlink);
    _SP_GENERATE_STANDARD_ASM_(getpid);
    _SP_GENERATE_STANDARD_ASM_(kill);
    _SP_GENERATE_STANDARD_ASM_(dup);
    _SP_GENERATE_STANDARD_ASM_(pipe);
    _SP_GENERATE_STANDARD_ASM_(mmap);
    _SP_GENERATE_STANDARD_ASM_(munmap);
    sp::println("Successfully generated ",filename, " using syscall table!");
}
} // namespace sp
#endif // ____SP_ASM_GEN____