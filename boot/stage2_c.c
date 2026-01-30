// MiniSecureOS Stage 2 C Code
// Called from 16-bit stub after switching to protected mode

#include <stdint.h>

// VGA text mode memory
#define VGA_MEMORY 0xB8000
#define VGA_WIDTH  80

// Prototypes
static inline void outb(uint16_t port, uint8_t val);
static void serial_putc(char c);
static void vga_debug(char c, int pos);
static void vga_print_debug(const char* str, int line);
void* memcpy(void* dest, const void* src, uint32_t n);
void* memset(void* s, int c, uint32_t n);

// Main C function called from assembly stub
// PLACED FIRST TO ENSURE OFFSET 0 RELATIVE TO C BINARY (Loaded at 0x10200)
// This is critical because stage2_c.ld does not enforce entry point order in binary output
__attribute__((section(".text.entry")))
void stage2_c_main(void) {
    // Debug output - show we're in C code
    vga_debug('C', 0);  // Position 0
    vga_print_debug("Stage 2 C Code Running", 1);
    
    // Copy kernel from 0x20000 (temporary buffer) to 0x100000 (1MB)
    vga_print_debug("Copying kernel...", 2);
    memcpy((void*)0x100000, (void*)0x20000, 64 * 512);  // Copy 64 sectors
    
    // Copy Userspace Init + Drivers from 0x28000 (0x20000 + 32KB) to 0x400000 (4MB)
    // We loaded ~255 sectors in stub, but let's copy a generous amount to 4MB region
    vga_print_debug("Copying userspace...", 3);
    memset((void*)0x400000, 0, 1024 * 1024); // Zero 1MB first
    memcpy((void*)0x400000, (void*)(0x20000 + (64 * 512)), 512 * 512); // Copy 512 sectors (256KB)
    
    
    // Verify kernel was loaded
    uint32_t* kernel_ptr = (uint32_t*)0x100000;
    if (*kernel_ptr == 0) {
        vga_print_debug("ERROR: Kernel not loaded!", 3);
        while (1) { __asm__ volatile("hlt"); }
    }
    
    vga_print_debug("Kernel copied successfully", 3);
    vga_print_debug("Jumping to kernel...", 4);
    
    // Jump to 0x100000 using inline assembly to ensure no return address is pushed
    // and to have precise control. clear registers for clean state.
    // ebx = 0, ecx = 0, edx = 0, esi = 0, edi = 0, ebp = 0
    // esp is set to 0x90000 (Top of bootloader stack, safe reuse) to ensure alignment and validity
    __asm__ volatile (
        "mov $0x90000, %%esp\n"
        "mov $0, %%ebx\n"
        "mov $0, %%ecx\n"
        "mov $0, %%edx\n"
        "mov $0, %%esi\n"
        "mov $0, %%edi\n"
        "mov $0, %%ebp\n"
        "jmp *%0\n"
        : : "r"((uint32_t)0x100000) : "memory"
    );
    
    // Should never return
    while(1) {}
}

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

// Simple VGA debug output
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static void serial_putc(char c) {
    outb(0x3F8, c);
}

static void vga_debug(char c, int pos) {
    volatile uint16_t* vga = (volatile uint16_t*)VGA_MEMORY;
    vga[pos] = (uint16_t)(c | 0x0F00);  // White on black
    serial_putc(c);
}

static void vga_print_debug(const char* str, int line) {
    volatile uint16_t* vga = (volatile uint16_t*)VGA_MEMORY;
    int pos = line * VGA_WIDTH;
    
    while (*str) {
        vga[pos++] = (uint16_t)(*str | 0x0F00);
        serial_putc(*str);
        str++;
    }
    serial_putc('\n');
}
