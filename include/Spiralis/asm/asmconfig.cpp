#include "asmconfig.hpp"

int main() {
    int darwin_ver = sp::get_darwin_major_version();
    sp::println("Detected Darwin Kernel Major Version: ",darwin_ver);

    sp::SyscallTable table;
    if(sp::KERNEL_TABLES.find(darwin_ver) != sp::KERNEL_TABLES.end()) {
        table = sp::KERNEL_TABLES.at(darwin_ver);
    }else{
        sp::println("Unknown Darwin version! Falling back to default BSD syscall numbers.");
        table = sp::default_sys_call_table;
    }
    generate_asm_file("Spiral/asm/spiral-asm.s", table);
    return 0;
}