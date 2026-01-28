// MiniSecureOS// Kernel Main Entry Point
// Initializes all kernel subsystems and starts user space

#include "kernel.h"
#include "hal.h"
#include <stddef.h>

// VGA text mode memory
#define VGA_MEMORY 0xB8000
#define VGA_WIDTH  80

// Simple VGA print function
static void vga_print(const char* str, int line) {
    volatile uint16_t* vga = (volatile uint16_t*)VGA_MEMORY;
    int pos = line * VGA_WIDTH;
    
    while (*str) {
        vga[pos++] = (uint16_t)(*str | 0x0F00);  // White on black
        str++;
    }
}

// Simple memory functions (since we don't have libc)
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

// External symbols from linker
extern uint32_t __bss_start;
extern uint32_t __bss_end;

// Kernel state
static bool kernel_initialized = false;

// Forward declarations
static void clear_bss(void);
static void init_interrupts(void);
// static void start_init_process(void);

// Kernel main entry point
void kernel_main(void) {
    // Clear BSS section
    clear_bss();
    
    // Clear screen and show we're in protected mode
    vga_print("Cat-OS Microkernel v1.0", 0);
    vga_print("========================", 1);
    vga_print("Initializing kernel subsystems...", 2);
    
    // Initialize HAL first
    hal_cpu_init();
    hal_io_init();
    hal_timer_init(100);  // 100 Hz timer
    hal_pic_init();
    
    // Disable timer interrupt initially to avoid spam
    // hal_timer_disable_irq();
    
    vga_print("HAL initialized", 3);
    
    // Initialize kernel subsystems
    memory_init();
    vga_print("Memory manager initialized", 4);
    
    scheduler_init();
    vga_print("Scheduler initialized", 5);
    
    ipc_init();
    vga_print("IPC system initialized", 6);
    
    capability_init();
    vga_print("Capability system initialized", 7);
    
    syscall_init();
    vga_print("System calls initialized", 8);
    
    init_interrupts();
    vga_print("Interrupts initialized", 9);
    
    // Mark kernel as initialized
    kernel_initialized = true;
    vga_print("Kernel initialization complete!", 10);
    vga_print("Cat-OS Microkernel is RUNNING!", 11);
    vga_print("Press Ctrl+C to exit QEMU", 12);
    
    // Halt and show success message
    while (1) {
        __asm__ volatile("hlt");
    }
}

// Clear BSS section
static void clear_bss(void) {
    uint8_t* bss = (uint8_t*)&__bss_start;
    uint32_t size = &__bss_end - &__bss_start;
    
    for (uint32_t i = 0; i < size; i++) {
        bss[i] = 0;
    }
}

// Initialize interrupt handlers
static void init_interrupts(void) {
    // Set up interrupt descriptor table (IDT)
    // This will be implemented in interrupt_init()
    interrupt_init();
}

// Kernel panic handler
void kernel_panic(const char* message) {
    vga_print("KERNEL PANIC: ", 20);
    vga_print(message, 21);
    
    while (1) {
        __asm__ volatile("hlt");
    }
}

// Simple kernel print function
void kernel_print(const char* str) {
    // For now, we'll use a simple VGA text mode output
    static uint16_t* vga_memory = (uint16_t*)0xB8000;
    static uint32_t position = 0;
    
    while (*str) {
        if (*str == '\r') {
            // Carriage return - move to beginning of line
            position = (position / 80) * 80;
        } else if (*str == '\n') {
            // New line - move to next line
            position += 80;
            if (position >= 80 * 25) {
                // Scroll screen
                for (uint32_t i = 0; i < 80 * 24; i++) {
                    vga_memory[i] = vga_memory[i + 80];
                }
                for (uint32_t i = 80 * 24; i < 80 * 25; i++) {
                    vga_memory[i] = 0x0720;  // Light gray on black, space
                }
                position -= 80;
            }
        } else {
            // Regular character
            vga_memory[position++] = (0x07 << 8) | *str;
            if (position >= 80 * 25) {
                // Scroll screen
                for (uint32_t i = 0; i < 80 * 24; i++) {
                    vga_memory[i] = vga_memory[i + 80];
                }
                for (uint32_t i = 80 * 24; i < 80 * 25; i++) {
                    vga_memory[i] = 0x0720;  // Light gray on black, space
                }
                position -= 80;
            }
        }
        str++;
    }
}

// Print hexadecimal value
void kernel_print_hex(uint32_t value) {
    const char hex_chars[] = "0123456789ABCDEF";
    char buffer[11] = "0x00000000";
    
    for (int i = 0; i < 8; i++) {
        buffer[9 - i] = hex_chars[value & 0xF];
        value >>= 4;
    }
    
    kernel_print(buffer);
}

// Check if kernel is initialized
bool kernel_is_initialized(void) {
    return kernel_initialized;
}

// Get kernel uptime in ticks
uint32_t kernel_get_uptime(void) {
    return hal_timer_get_ticks();
}

// Get kernel version string
const char* kernel_get_version(void) {
    return "MiniSecureOS v1.0";
}
