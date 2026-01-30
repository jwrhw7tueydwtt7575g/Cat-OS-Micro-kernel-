
#include "hal.h"
#include <stddef.h>

// GDT definitions
#define GDT_ENTRIES 6

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// TSS structure
struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr gdt_ptr;
static volatile struct tss_entry tss_entry;

// Forward declaration
static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
static void write_tss(int32_t num, uint16_t ss0, uint32_t esp0);

// Initialize GDT
void hal_gdt_init(void) {
    gdt_ptr.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gdt_ptr.base = (uint32_t)&gdt;
    
    // Null segment
    gdt_set_gate(0, 0, 0, 0, 0);
    
    // Kernel Code Segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    
    // Kernel Data Segment
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    
    // User Code Segment
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    
    // User Data Segment
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
    
    // TSS Segment
    write_tss(5, 0x10, 0x0);
    
    // Load GDT
    __asm__ volatile("lgdt %0" : : "m"(gdt_ptr));
    
    // Flush Segment Registers
    __asm__ volatile(
        "mov $0x10, %ax\n"
        "mov %ax, %ds\n"
        "mov %ax, %es\n"
        "mov %ax, %fs\n"
        "mov %ax, %gs\n"
        "mov %ax, %ss\n"
        "ljmp $0x08, $.flush\n"
        ".flush:\n"
    );
    
    // Load Task Register (TSS)
    // TSS selector is 5 * 8 = 40 = 0x28|0|0 = 0x28
    __asm__ volatile("ltr %0" : : "r"((uint16_t)0x28));
}

// Update TSS Kernel Stack
void hal_tss_set_esp0(uint32_t esp0) {
    tss_entry.esp0 = esp0;
}

// Set GDT Gate
static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;
    
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access = access;
}

// Initialise TSS
static void write_tss(int32_t num, uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t)&tss_entry;
    uint32_t limit = sizeof(tss_entry) - 1;
    
    // Add TSS descriptor to GDT (DPL=0, Type=9)
    gdt_set_gate(num, base, limit, 0x89, 0x00);
    
    // Zero out TSS
    uint8_t* p = (uint8_t*)(void*)&tss_entry;
    for (uint32_t i = 0; i < sizeof(tss_entry); i++) {
        p[i] = 0;
    }
    
    tss_entry.ss0 = ss0;
    tss_entry.esp0 = esp0;
    tss_entry.cs = 0x0b;
    tss_entry.ss = 0x13;
    tss_entry.ds = 0x13;
    tss_entry.es = 0x13;
    tss_entry.fs = 0x13;
    tss_entry.gs = 0x13;
    tss_entry.iomap_base = sizeof(tss_entry);
}
