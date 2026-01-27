// HAL CPU Control Module
// Provides CPU control and feature detection

#include "hal.h"
#include "types.h"

// CPU feature flags
static uint32_t cpu_features = 0;

// CPU feature bits
#define CPU_FEAT_FPU    0x00000001
#define CPU_FEAT_MMX    0x00000002
#define CPU_FEAT_SSE    0x00000004
#define CPU_FEAT_SSE2   0x00000008
#define CPU_FEAT_APIC   0x00000010

// Initialize CPU module
void hal_cpu_init(void) {
    cpu_features = hal_cpu_get_features();
}

// Get CPU features using CPUID instruction
uint32_t hal_cpu_get_features(void) {
    uint32_t features = 0;
    uint32_t eax, ebx, ecx, edx;
    
    // Check if CPUID is supported
    __asm__ volatile(
        "pushf\n"
        "pop %%eax\n"
        "mov %%eax, %%ebx\n"
        "xor $0x200000, %%eax\n"
        "push %%eax\n"
        "popf\n"
        "pushf\n"
        "pop %%eax\n"
        "cmp %%eax, %%ebx\n"
        "sete %%al"
        : "=a"(eax)
        :
        : "ebx", "ecx", "edx"
    );
    
    if (!eax) {
        return 0;  // CPUID not supported
    }
    
    // Get basic feature information
    __asm__ volatile(
        "mov $1, %%eax\n"
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        :
        : "memory"
    );
    
    // Check for features
    if (edx & (1 << 0))   features |= CPU_FEAT_FPU;
    if (edx & (1 << 23))  features |= CPU_FEAT_MMX;
    if (edx & (1 << 25))  features |= CPU_FEAT_SSE;
    if (edx & (1 << 26))  features |= CPU_FEAT_SSE2;
    if (edx & (1 << 9))   features |= CPU_FEAT_APIC;
    
    return features;
}

// Enable paging
void hal_cpu_enable_paging(uint32_t page_dir) {
    uint32_t cr0;
    
    // Set page directory base
    hal_cpu_set_cr3(page_dir);
    
    // Enable paging in CR0
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= CR0_PG;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}

// Set CR3 register (page directory base)
void hal_cpu_set_cr3(uint32_t page_dir) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(page_dir));
}

// Halt CPU until interrupt
void hal_cpu_halt(void) {
    __asm__ volatile("hlt");
}

// Get CPU cycle counter (if available)
uint64_t hal_cpu_get_cycles(void) {
    uint32_t low, high;
    
    if (cpu_features & CPU_FEAT_APIC) {
        __asm__ volatile(
            "rdtsc"
            : "=a"(low), "=d"(high)
        );
        return ((uint64_t)high << 32) | low;
    }
    
    return 0;  // Counter not available
}

// Get current CR0 register
uint32_t hal_cpu_get_cr0(void) {
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    return cr0;
}

// Get current CR2 register (page fault address)
uint32_t hal_cpu_get_cr2(void) {
    uint32_t cr2;
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
    return cr2;
}

// Get current CR3 register (page directory base)
uint32_t hal_cpu_get_cr3(void) {
    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

// Flush TLB (Translation Lookaside Buffer)
void hal_cpu_flush_tlb(void) {
    __asm__ volatile(
        "mov %%cr3, %%eax\n"
        "mov %%eax, %%cr3"
        :
        :
        : "eax"
    );
}

// Enable/disable interrupts
void hal_cpu_enable_interrupts(void) {
    __asm__ volatile("sti");
}

void hal_cpu_disable_interrupts(void) {
    __asm__ volatile("cli");
}

// Get current privilege level
uint32_t hal_cpu_get_cpl(void) {
    uint32_t cpl;
    __asm__ volatile("mov %%cs, %0" : "=r"(cpl));
    return cpl & 0x3;
}

// Load task register (for task switching)
void hal_cpu_load_tr(uint16_t selector) {
    __asm__ volatile("ltr %0" : : "r"(selector));
}

// Store task register
uint16_t hal_cpu_store_tr(void) {
    uint16_t tr;
    __asm__ volatile("str %0" : "=r"(tr));
    return tr;
}
