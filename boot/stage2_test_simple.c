// MiniSecureOS Stage 2 Bootloader - Simple Test
// Just prints to verify it's working

#include <stdint.h>

// Simple BIOS print function for 16-bit mode
static void print_string_bios(const char* str) {
    __asm__ volatile(
        "mov $0x0E, %%ah\n"
        "1:\n"
        "movb (%0), %%al\n"
        "cmpb $0, %%al\n"
        "je 2f\n"
        "int $0x10\n"
        "inc %0\n"
        "jmp 1b\n"
        "2:\n"
        :
        : "r"(str)
        : "eax", "ebx", "ecx", "edx"
    );
}

// Stage 2 main function - 16-bit real mode
void stage2_main(void) {
    // Clear screen and set video mode
    __asm__ volatile(
        "mov $0x0003, %%ax\n"
        "int $0x10\n"
        :
        :
        : "eax"
    );
    
    print_string_bios("=== STAGE 2 WORKING! ===\r\n");
    print_string_bios("Stage 2: 16-bit mode OK!\r\n");
    print_string_bios("BIOS printing works!\r\n");
    print_string_bios("Halting system...\r\n");
    
    // Halt the system
    while (1) {
        __asm__ volatile("hlt");
    }
}
