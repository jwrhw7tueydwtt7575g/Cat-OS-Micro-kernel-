// MiniSecureOS// Kernel Main Entry Point
// Initializes all kernel subsystems and starts user space

#include "kernel.h"
#include "hal.h"
#include <stddef.h>

// Helper to expose private process functions for main.c
void process_init(void);
void process_setup_stack(pcb_t* process, uint32_t entry_point);

// VGA text mode memory
#define VGA_MEMORY 0xB8000
#define VGA_WIDTH  80

// Simple VGA print function
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static void serial_putc(char c) {
    while ((hal_inb(0x3F8 + 5) & 0x20) == 0);
    hal_outb(0x3F8, c);
}

void vga_print(const char* str, int line) {
    volatile uint16_t* vga = (volatile uint16_t*)VGA_MEMORY;
    int pos = line * VGA_WIDTH;
    
    while (*str) {
        vga[pos++] = (uint16_t)(*str | 0x0F00);
        serial_putc(*str);
        str++;
    }
    serial_putc('\r');
    serial_putc('\n');
}

// Global kernel state
bool kernel_initialized = false;
extern uint32_t __bss_start;
extern uint32_t __bss_end;

// Debug functions
void kernel_print(const char* str) {
    while (*str) {
        serial_putc(*str++);
    }
}

void kernel_print_hex(uint32_t value) {
    const char* hex_chars = "0123456789ABCDEF";
    kernel_print("0x");
    for (int i = 28; i >= 0; i -= 4) {
        serial_putc(hex_chars[(value >> i) & 0x0F]);
    }
}

void kernel_panic(const char* message) {
    vga_print("KERNEL PANIC: ", 20);
    vga_print(message, 21);
    kernel_print("\r\nKERNEL PANIC: ");
    kernel_print(message);
    kernel_print("\r\n");
    hal_cpu_disable_interrupts();
    while (1) {
        hal_cpu_halt();
    }
}

void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

void* memset(void* s, int c, size_t n) {
    uint8_t* p = (uint8_t*)s;
    while (n--) {
        *p++ = (uint8_t)c;
    }
    return s;
}

// Forward declarations
static void clear_bss(void);
static void start_system_services(void);

// Kernel main entry point
void kernel_main(void) {
    // Clear screen
    for (int i = 0; i < 80 * 25; i++) {
        ((uint16_t*)VGA_MEMORY)[i] = 0x0F20;
    }
    
    vga_print("Cat-OS Microkernel v1.0", 0);
    vga_print("========================", 1);
    vga_print("Initializing kernel...", 2);
    
    // Low-level initialization
    clear_bss();
    vga_print("BSS cleared", 3);
    
    // HAL
    hal_gdt_init();
    vga_print("GDT initialized", 4);
    hal_cpu_init();
    vga_print("CPU initialized", 5);
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
    interrupt_init();
    vga_print("Interrupts initialized", 13);
    hal_timer_init(100);
    vga_print("Timer enabled", 14);
    
    kernel_initialized = true;
    vga_print("Kernel initialization complete!", 15);
    vga_print("Cat-OS Microkernel is RUNNING!", 16);
    
    // Start services
    vga_print("Starting system services...", 17);
    start_system_services();
    vga_print("All services started!", 18);
    
    // Enable interrupts
    hal_cpu_enable_interrupts();
    
    // Idle loop
    while (1) {
        __asm__ volatile("hlt");
    }
}

static void clear_bss(void) {
    uint8_t* bss = (uint8_t*)&__bss_start;
    uint32_t size = (uint32_t)&__bss_end - (uint32_t)&__bss_start;
    for (uint32_t i = 0; i < size; i++) {
        bss[i] = 0;
    }
}

static void start_service(const char* name, uint32_t phys_addr, bool is_user) {
    pcb_t* proc;
    if (is_user) {
        proc = process_create(0);
    } else {
        proc = process_create_kernel();
    }
    
    if (!proc) {
        kernel_print("Failed to create process for ");
        kernel_print(name);
        kernel_print("\r\n");
        return;
    }
    
    // Map the binary (assuming 32KB max for now) to virtual 0x400000
    for (uint32_t i = 0; i < 8; i++) { // 8 pages = 32KB
        memory_map_page(proc->page_directory, 0x400000 + i * PAGE_SIZE, phys_addr + i * PAGE_SIZE, 0x07);
    }
    
    // Set entry point to standard 0x400000
    process_setup_stack(proc, 0x400000);
    
    scheduler_add_process(proc);
}

static void start_system_services(void) {
    vga_print("Starting Init Process (PID 1)...", 12);
    start_service("Init", 0x400000, true);
    
    vga_print("Starting Keyboard Driver (PID 2)...", 13);
    start_service("Keyboard", 0x408000, false);
    
    vga_print("Starting Console Driver (PID 3)...", 14);
    start_service("Console", 0x410000, false);
    
    vga_print("Starting Timer Driver (PID 4)...", 15);
    start_service("Timer", 0x418000, false);
    
    vga_print("Starting Shell (PID 5)...", 16);
    start_service("Shell", 0x420000, true);
    
    kernel_print("System services started.\r\n");
}
