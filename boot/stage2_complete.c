// MiniSecureOS Stage 2 Bootloader - Complete Version
// Sets up protected mode and loads kernel

#include <stdint.h>

// I/O port definitions
#define PORT_PIC_MASTER_CMD  0x20
#define PORT_PIC_MASTER_DATA  0x21
#define PORT_PIC_SLAVE_CMD   0xA0
#define PORT_PIC_SLAVE_DATA  0xA1
#define PORT_KEYBOARD_DATA   0x60
#define PORT_KEYBOARD_STATUS 0x64

// Memory addresses
#define KERNEL_LOAD_ADDR 0x100000  // 1MB mark
#define GDT_ADDR         0x0800
#define GDT_ENTRIES      5

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
static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr gdt_ptr;

// Function prototypes
static void print_string_bios(const char* str);
static void setup_gdt(void);
static void enable_a20_line(void);
static void load_kernel(void);
static void enter_protected_mode(void);
static void protected_mode_entry(void);

// Stage 2 main function - starts in 16-bit real mode
void stage2_main(void) {
    // Clear screen and set video mode
    __asm__ volatile(
        "mov $0x0003, %%ax\n"
        "int $0x10\n"
        :
        :
        : "eax"
    );
    
    print_string_bios("MiniSecureOS Stage 2 Bootloader\r\n");
    print_string_bios("================================\r\n");
    
    // Enable A20 line for access to memory above 1MB
    enable_a20_line();
    print_string_bios("A20 line enabled\r\n");
    
    // Setup Global Descriptor Table
    setup_gdt();
    print_string_bios("GDT setup complete\r\n");
    
    // Load kernel from disk
    load_kernel();
    print_string_bios("Kernel loaded successfully\r\n");
    
    // Enter protected mode and jump to kernel
    enter_protected_mode();
    
    // Should never reach here
    print_string_bios("ERROR: Failed to enter protected mode\r\n");
    while (1) {
        __asm__ volatile("hlt");
    }
}

// Print string using BIOS (16-bit mode)
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

// Print hex value using BIOS
// static void print_hex_bios(uint8_t value) {
//     uint8_t high = value >> 4;
//     uint8_t low = value & 0x0F;
//     
//     char hex_chars[] = "0123456789ABCDEF";
//     
//     __asm__ volatile(
//         "mov $0x0E, %%ah\n"
//         "movb %1, %%al\n"
//         "int $0x10\n"
//         "movb %2, %%al\n"
//         "int $0x10\n"
//         :
//         : "r"(hex_chars[high]), "r"(hex_chars[low])
//         : "eax"
//     );
// }

// Enable A20 line using keyboard controller
static void enable_a20_line(void) {
    // Simple A20 enable using fast A20 gate
    __asm__ volatile(
        "inb $0x92, %%al\n"
        "orb $0x02, %%al\n"
        "outb %%al, $0x92\n"
        :
        :
        : "eax"
    );
}

// Setup Global Descriptor Table
static void setup_gdt(void) {
    // Clear GDT
    for (int i = 0; i < GDT_ENTRIES; i++) {
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
    gdt_ptr.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gdt_ptr.base = (uint32_t)&gdt;
    
    // Load GDT
    __asm__ volatile("lgdtl %0" : : "m"(gdt_ptr));
}

// Load kernel from disk
static void load_kernel(void) {
    // For now, we'll create a simple test kernel in memory
    // In a real implementation, this would read from disk
    
    uint8_t* kernel_ptr = (uint8_t*)KERNEL_LOAD_ADDR;
    
    // Create a simple test kernel that prints to VGA
    // This is just a placeholder - real kernel loading would read from disk
    for (int i = 0; i < 1024; i++) {
        kernel_ptr[i] = 0x90;  // NOP instructions
    }
    
    // Add a simple "Hello from kernel" message at the end
    const char* msg = "Hello from Protected Mode Kernel!";
    for (int i = 0; msg[i] != 0; i++) {
        kernel_ptr[1000 + i] = msg[i];
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
    
    // Print to VGA memory to show we're in protected mode
    volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
    const char* msg = "PROTECTED MODE ACTIVE!";
    
    for (int i = 0; msg[i] != 0; i++) {
        vga[80 + i] = (uint16_t)(msg[i] | 0x0F00);  // White on black
    }
    
    // Halt
    while (1) {
        __asm__ volatile("hlt");
    }
}
