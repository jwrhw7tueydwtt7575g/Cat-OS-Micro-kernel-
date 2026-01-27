// MiniSecureOS// Kernel Main Entry Point
// Initializes all kernel subsystems and starts user space

#include "kernel.h"
#include "hal.h"
#include <stddef.h>

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
static void start_init_process(void);

// Kernel main entry point
void kernel_main(void) {
    // Clear BSS section
    clear_bss();
    
    // Initialize HAL first
    hal_cpu_init();
    hal_io_init();
    hal_timer_init(100);  // 100 Hz timer
    hal_pic_init();
    
    // Print welcome message
    kernel_print("MiniSecureOS Kernel v1.0\r\n");
    kernel_print("Initializing kernel subsystems...\r\n");
    
    // Initialize kernel subsystems
    memory_init();
    scheduler_init();
    ipc_init();
    capability_init();
    syscall_init();
    init_interrupts();
    
    kernel_print("Kernel initialization complete\r\n");
    
    // Mark kernel as initialized
    kernel_initialized = true;
    
    // Start first user process (init)
    start_init_process();
    
    // Enable interrupts and start scheduling
    hal_cpu_enable_interrupts();
    
    // Main kernel loop
    while (1) {
        hal_cpu_halt();
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
    
    kernel_print("Interrupt system initialized\r\n");
}

// Start init process (PID 1)
static void start_init_process(void) {
    kernel_print("Starting init process...\r\n");
    
    // Create init process
    pcb_t* init_process = process_create(0);  // Parent is kernel
    
    if (!init_process) {
        kernel_panic("Failed to create init process");
    }
    
    // Set up init process
    init_process->priority = 1;  // High priority
    init_process->state = PROCESS_READY;
    
    // Add to scheduler
    scheduler_add_process(init_process);
    
    kernel_print("Init process created with PID ");
    kernel_print_hex(init_process->pid);
    kernel_print("\r\n");
}

// Kernel panic handler
void kernel_panic(const char* message) {
    kernel_print("KERNEL PANIC: ");
    kernel_print(message);
    kernel_print("\r\n");
    
    // Disable interrupts
    hal_cpu_disable_interrupts();
    
    // Halt system
    while (1) {
        hal_cpu_halt();
    }
}

// Simple kernel print function
void kernel_print(const char* str) {
    // For now, we'll use a simple VGA text mode output
    // This will be enhanced with proper console driver later
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
            position = (position / 80) * 80;
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
