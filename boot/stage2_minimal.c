// MiniSecureOS Stage 2 Bootloader - Minimal Version
// Sets up protected mode and loads kernel

#include <stdint.h>

// Memory addresses
#define KERNEL_LOAD_ADDR 0x100000  // 1MB mark

// GDT structure
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// Global variables
static struct gdt_entry gdt[5];
static struct gdt_ptr gdt_ptr;

// Function prototypes
static void print_string(const char* str);
static void setup_gdt(void);
static void load_kernel(void);
static void enter_protected_mode(void);
static void protected_mode_entry(void);

// Stage 2 main function
void stage2_main(void) {
    print_string("MiniSecureOS Stage 2 Bootloader\r\n");
    
    // Setup Global Descriptor Table
    setup_gdt();
    print_string("GDT setup complete\r\n");
    
    // Load kernel from disk
    load_kernel();
    print_string("Kernel loaded\r\n");
    
    // Enter protected mode and jump to kernel
    enter_protected_mode();
    
    // Should never reach here
    print_string("Failed to enter protected mode\r\n");
    while (1) {
        __asm__ volatile("hlt");
    }
}

// Print string to screen (BIOS)
static void print_string(const char* str) {
    while (*str) {
        __asm__ volatile(
            "movb $0x0E, %%ah\n"
            "int $0x10"
            :
            : "al"(*str++)
        );
    }
}

// Setup Global Descriptor Table
static void setup_gdt(void) {
    // Clear GDT
    for (int i = 0; i < 5; i++) {
        gdt[i].limit_low = 0;
        gdt[i].base_low = 0;
        gdt[i].base_mid = 0;
        gdt[i].access = 0;
        gdt[i].granularity = 0;
        gdt[i].base_high = 0;
    }
    
    // Code segment (ring 0, read/write, present)
    gdt[1].limit_low = 0xFFFF;
    gdt[1].base_low = 0x0000;
    gdt[1].base_mid = 0x00;
    gdt[1].access = 0x9A;  // Present, ring 0, code, readable
    gdt[1].granularity = 0xCF;  // 4KB granularity, 32-bit
    gdt[1].base_high = 0x00;
    
    // Data segment (ring 0, read/write, present)
    gdt[2].limit_low = 0xFFFF;
    gdt[2].base_low = 0x0000;
    gdt[2].base_mid = 0x00;
    gdt[2].access = 0x92;  // Present, ring 0, data, writable
    gdt[2].granularity = 0xCF;  // 4KB granularity, 32-bit
    gdt[2].base_high = 0x00;
    
    // Setup GDT pointer
    gdt_ptr.limit = (sizeof(struct gdt_entry) * 5) - 1;
    gdt_ptr.base = (uint32_t)&gdt;
    
    // Load GDT
    __asm__ volatile("lgdtl %0" : : "m"(gdt_ptr));
}

// Load kernel from disk (simplified)
static void load_kernel(void) {
    // For now, we'll just clear the kernel area
    // In a real implementation, this would read from disk
    uint8_t* kernel_ptr = (uint8_t*)KERNEL_LOAD_ADDR;
    for (int i = 0; i < 32768; i++) {  // 32KB
        kernel_ptr[i] = 0;
    }
}

// Enter protected mode
static void enter_protected_mode(void) {
    // Disable interrupts
    __asm__ volatile("cli");
    
    // Enable protected mode bit in CR0
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x01;  // Set protected mode bit
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
    
    // Far jump to flush instruction pipeline and load CS
    __asm__ volatile(
        "ljmp $0x08, $protected_mode_entry"
    );
}

// Protected mode entry point
__attribute__((used))
void protected_mode_entry(void) {
    // Setup segment registers
    __asm__ volatile(
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        :
        :
    );
    
    // Setup stack
    __asm__ volatile("mov $0x90000, %esp");
    
    // For now, just halt - we'll implement kernel loading later
    print_string("In protected mode - halting\r\n");
    while (1) {
        __asm__ volatile("hlt");
    }
}
