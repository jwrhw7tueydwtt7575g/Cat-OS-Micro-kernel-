// MiniSecureOS// Kernel Main Entry Point
// Initializes all kernel subsystems and starts user space

#include "kernel.h"
#include "hal.h"
#include <stddef.h>

// Helper to expose private process functions for main.c
// In a cleaner design, these would be in a header, but for now we redeclare
void process_init(void);
void process_setup_stack(pcb_t* process, uint32_t entry_point);

// VGA text mode memory
#define VGA_MEMORY 0xB8000
#define VGA_WIDTH  80

// Simple VGA print function
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "d"(port) );
}

static void vga_print(const char* str, int line) {
    volatile uint16_t* vga = (volatile uint16_t*)VGA_MEMORY;
    int pos = line * VGA_WIDTH;
    
    while (*str) {
        vga[pos++] = (uint16_t)(*str | 0x0F00);  // White on black
        outb(0x3F8, *str);
        str++;
    }
    outb(0x3F8, '\n');
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
static void start_system_services(void);

// Kernel main entry point
void kernel_main(void) {
    // Clear VGA screen
    volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
    for (int i = 0; i < 80 * 25; i++) {
        vga[i] = 0x0720;  // White on black, space
    }
    
    // Show banner
    vga_print("Cat-OS Microkernel v1.0", 0);
    vga_print("========================", 1);
    vga_print("Initializing kernel...", 2);
    
    // Clear BSS
    clear_bss();
    vga_print("BSS cleared", 3);
    
    // HAL
    hal_cpu_init();
    vga_print("CPU initialized", 4);
    hal_io_init();
    vga_print("I/O initialized", 5);
    hal_pic_init();
    vga_print("PIC initialized", 6);
    
    // Memory
    memory_init();
    vga_print("Memory manager initialized", 7);
    
    // Process subsystems
    scheduler_init();
    vga_print("Scheduler initialized", 8);
    process_init();
    vga_print("Process management initialized", 9);
    ipc_init();
    vga_print("IPC initialized", 10);
    capability_init();
    vga_print("Capability system initialized", 11);
    
    // Interrupts & syscalls
    syscall_init();
    vga_print("System calls initialized", 12);
    init_interrupts();
    vga_print("Interrupts initialized", 13);
    hal_timer_init(100);
    vga_print("Timer enabled", 14);
    
    kernel_initialized = true;
    vga_print("Kernel initialization complete!", 15);
    vga_print("Cat-OS Microkernel is RUNNING!", 16);
    
    // Enable interrupts
    hal_cpu_enable_interrupts();
    
    // Start services
    vga_print("Starting system services...", 17);
    start_system_services();
    vga_print("All services started!", 18);
    
    // Idle loop
    while (1) {
        __asm__ volatile("hlt");
    }
}

// Clear BSS section
__attribute__((unused))
static void clear_bss(void) {
    uint8_t* bss = (uint8_t*)&__bss_start;
    uint32_t size = &__bss_end - &__bss_start;
    
    for (uint32_t i = 0; i < size; i++) {
        bss[i] = 0;
    }
}

// Initialize interrupt handlers
__attribute__((unused))
static void init_interrupts(void) {
    // Set up interrupt descriptor table (IDT)
    // This will be implemented in interrupt_init()
    interrupt_init();
}

extern uint32_t kernel_page_dir; // Defined in memory.c

__attribute__((unused))
static void start_service(uint32_t binary_offset) {
    pcb_t* proc = process_create(0);
    if (!proc) {
        kernel_print("Failed to create process for service\r\n");
        return;
    }
    
    // We assume binaries are max 64KB for now (16 pages)
    uint32_t pages_needed = 16; 
    
    // Source data is in physical memory at 0x400000 + binary_offset
    // We can access it directly since it's identity mapped in kernel 0-4MB
    uint32_t src_addr = 0x400000 + binary_offset;
    
    // Temp kernel mapping address for destination
    uint32_t temp_virt = 0xE00000; 
    
    for (uint32_t i = 0; i < pages_needed; i++) {
        uint32_t phys_page = (uint32_t)memory_alloc_physical();
        if (!phys_page) break;
        
        // Map to process address space at 0x400000
        uint32_t dest_virt = 0x400000 + i * PAGE_SIZE;
        memory_map_page(proc->page_directory, dest_virt, phys_page, 0x07); // User, RW, Present
        
        // Map to kernel temp space to write data
        memory_map_page(kernel_page_dir, temp_virt, phys_page, 0x03); // Kernel, RW, Present
        
        // Copy page
        memcpy((void*)temp_virt, (void*)(src_addr + i * PAGE_SIZE), PAGE_SIZE);
        
        // Unmap temp (optional, good practice)
        memory_unmap_page(kernel_page_dir, temp_virt);
    }
    
    // Set entry point to 0x400000
    process_setup_stack(proc, 0x400000);
    
    // Add to scheduler
    scheduler_add_process(proc);
}

__attribute__((unused))
static void start_system_services(void) {
    vga_print("Starting Init Process (PID 1)...", 12);
    start_service(0x00000); // Init
    
    vga_print("Starting Keyboard Driver (PID 2)...", 13);
    start_service(0x08000); // Keyboard (32KB offset - sector 138-74=64)
    
    vga_print("Starting Console Driver (PID 3)...", 14);
    start_service(0x10000); // Console (64KB offset - sector 202-74=128)
    
    vga_print("Starting Timer Driver (PID 4)...", 15);
    start_service(0x18000); // Timer (96KB offset - sector 266-74=192)
    
    vga_print("Starting Shell (PID 5)...", 16);
    start_service(0x20000); // Shell (128KB offset - sector 330-74=256)
    
    kernel_print("System services started.\r\n");
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
        // Output to serial (always)
        outb(0x3F8, *str);
        
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
