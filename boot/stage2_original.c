// MiniSecureOS Stage 2 Bootloader
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
static void print_string(const char* str);
static void print_string_vga(const char* str);
static void enable_a20_line(void);
static void setup_gdt(void);
static void enter_protected_mode(void);
static void load_kernel(void);
static void read_disk(uint32_t lba, uint8_t sectors, void* buffer);

// Stage 2 main function
void stage2_main(void) {
    print_string("MiniSecureOS Stage 2 Bootloader\r\n");
    
    // Enable A20 line for access to memory above 1MB
    enable_a20_line();
    print_string("A20 line enabled\r\n");
    
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

// Print string to VGA (protected mode)
static void print_string_vga(const char* str) {
    volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
    static int pos = 80 * 2;  // Start at line 2
    
    while (*str) {
        if (*str == '\r') {
            pos = (pos / 80) * 80;
        } else if (*str == '\n') {
            pos += 80;
        } else {
            vga[pos++] = (uint16_t)(*str | 0x0F00);  // White on black
        }
        str++;
    }
}

// Enable A20 line using keyboard controller method
static void enable_a20_line(void) {
    uint8_t temp;
    
    // Disable keyboard
    __asm__ volatile("inb %1, %0" : "=a"(temp) : "Nd"(PORT_KEYBOARD_STATUS));
    __asm__ volatile("outb %0, %1" : : "a"(temp), "Nd"(PORT_KEYBOARD_STATUS));
    
    // Read output port
    __asm__ volatile("inb %1, %0" : "=a"(temp) : "Nd"(PORT_KEYBOARD_DATA));
    
    // Write command to enable A20
    temp = 0xD1;
    __asm__ volatile("outb %0, %1" : : "a"(temp), "Nd"(PORT_KEYBOARD_STATUS));
    
    // Enable A20 bit
    temp = 0xDF;
    __asm__ volatile("outb %0, %1" : : "a"(temp), "Nd"(PORT_KEYBOARD_DATA));
    
    // Wait for completion
    uint32_t timeout = 100000;
    while (timeout--) {
        uint8_t status;
        __asm__ volatile("inb %1, %0" : "=a"(status) : "Nd"(PORT_KEYBOARD_STATUS));
        if (!(status & 0x02)) break;
    }
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
    
    // Null descriptor (required)
    gdt[0].limit_low = 0;
    gdt[0].base_low = 0;
    gdt[0].base_mid = 0;
    gdt[0].access = 0;
    gdt[0].granularity = 0;
    gdt[0].base_high = 0;
    
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
__attribute__((noreturn))
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
    __asm__ volatile("mov $0x90000, %%esp");
    
    // Print success message in protected mode
    print_string_vga("Protected mode entered successfully!\r\n");
    print_string_vga("Jumping to kernel...\r\n");
    
    // Jump to kernel entry point
    void (*kernel_entry)(void) = (void*)KERNEL_LOAD_ADDR;
    kernel_entry();
    
    // Should never reach here
    while (1) {
        __asm__ volatile("hlt");
    }
}

// Load kernel from disk
static void load_kernel(void) {
    // Read kernel starting at sector 9 (after bootloaders)
    read_disk(9, 64, (void*)KERNEL_LOAD_ADDR);  // Read 64 sectors (32KB)
}

// Read disk sectors using BIOS
static void read_disk(uint32_t lba, uint8_t sectors, void* buffer) {
    // Convert LBA to CHS for floppy
    uint16_t cylinder = (lba / 36) / 2;
    uint8_t head = (lba / 18) % 2;
    uint8_t sector = (lba % 18) + 1;
    
    uint16_t ax_val = 0x0200 | sectors;  // AH=0x02 (read), AL=sectors
    
    __asm__ volatile(
        "mov %2, %%ah\n"         // AH = sectors count
        "mov %3, %%ch\n"         // Cylinder
        "mov %4, %%cl\n"         // Sector
        "mov %5, %%dh\n"         // Head
        "movb $0x00, %%dl\n"      // Drive A
        "int $0x13\n"
        "jc 1f\n"
        "jmp 2f\n"
        "1:\n"
        "jmp 1f\n"  // Infinite loop on error
        "2:\n"
        :
        : "b"(buffer), "a"(ax_val), "r"(sectors), "r"(cylinder), "r"(sector), "r"(head)
        : "memory"
    );
}
