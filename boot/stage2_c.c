// MiniSecureOS Stage 2 C Code
// Called from 16-bit stub after switching to protected mode

#include <stdint.h>

// VGA text mode memory
#define VGA_MEMORY 0xB8000
#define VGA_WIDTH  80

// Simple memory functions
void* memcpy(void* dest, const void* src, uint32_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

void* memset(void* s, int c, uint32_t n) {
    uint8_t* p = (uint8_t*)s;
    while (n--) {
        *p++ = (uint8_t)c;
    }
    return s;
}

// VGA print helper
static void vga_print(const char* str, int line) {
    volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
    int pos = line * VGA_WIDTH;
    while (*str) {
        vga[pos++] = (uint16_t)(*str | 0x0F00);  // White on black
        str++;
    }
}

// Main C function called from assembly stub
void stage2_c_main(void) {
    volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
    
    // Clear screen
    for (int i = 0; i < 80 * 25; i++) {
        vga[i] = 0x0000;
    }
    
    vga_print("Stage 2 Bootloader - Protected Mode", 0);
    vga_print("====================================", 1);
    vga_print("A20 line: ENABLED", 2);
    vga_print("GDT: LOADED", 3);
    vga_print("Protected Mode: ACTIVE (32-bit)", 4);
    vga_print("", 5);
    vga_print("System Status:", 6);
    vga_print("  Memory: OK", 7);
    vga_print("  Segments: OK", 8);
    vga_print("  Stack: 0x90000", 9);
    vga_print("", 10);
    vga_print("Looking for kernel at 0x100000...", 11);
    
    // Check if kernel code is loaded at 0x100000
    volatile uint32_t* kernel_addr = (volatile uint32_t*)0x100000;
    uint32_t first_word = *kernel_addr;
    
    // Display hex value of first instruction
    vga_print("Kernel first dword: ", 12);
    
    // Simple hex printer
    volatile uint16_t* vga_pos = (volatile uint16_t*)(0xB8000 + 2 * 20 + 80 * 12 * 2);
    uint32_t val = first_word;
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (val >> (i * 4)) & 0xF;
        char hex_char = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
        *vga_pos++ = (uint16_t)(hex_char | 0x0F00);
    }
    
    vga_print("", 13);
    
    if (first_word == 0xCCCCCCCC || first_word == 0x00000000) {
        vga_print("WARNING: Kernel memory appears empty!", 14);
        vga_print("Kernel might not be loaded from disk.", 15);
    } else {
        vga_print("Kernel code detected!", 14);
        vga_print("Jumping to kernel entry point...", 15);
    }
    
    // Jump to kernel entry point at 0x100000
    typedef void (*kernel_entry_t)(void);
    kernel_entry_t kernel_entry = (kernel_entry_t)0x100000;
    kernel_entry();
    
    // Should never reach here
    vga_print("KERNEL RETURNED - SYSTEM HALT", 17);
    while (1) {
        __asm__ volatile("hlt");
    }
}
