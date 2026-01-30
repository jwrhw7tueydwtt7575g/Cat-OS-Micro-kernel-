// Kernel Memory Management
// Handles physical page allocation, deallocation, and paging

#include "kernel.h"
#include "hal.h"
#include <stddef.h>

#define MEMORY_SIZE (16 * 1024 * 1024)  // 16MB for now
#define PHYS_PAGES (MEMORY_SIZE / PAGE_SIZE)
#define BITMAP_SIZE (PHYS_PAGES / 32)

static uint32_t page_bitmap[BITMAP_SIZE];
static uint32_t total_allocated_pages = 0;

uint32_t kernel_page_dir = 0;
extern uint32_t __bss_start;
extern uint32_t __bss_end;

// Forward declarations
static void bitmap_set(uint32_t page);
static void bitmap_clear(uint32_t page);
static bool bitmap_test(uint32_t page);

// Initialize memory manager
void memory_init(void) {
    // Clear bitmap
    for (int i = 0; i < BITMAP_SIZE; i++) {
        page_bitmap[i] = 0;
    }
    
    // Mark first MB as reserved (BIOS, EBDA, Video memory etc)
    for (uint32_t i = 0; i < (1024 * 1024) / PAGE_SIZE; i++) {
        bitmap_set(i);
    }
    
    // Mark kernel as reserved
    // Kernel is loaded at 1MB, let's assume it's up to 2MB for now
    for (uint32_t i = (1024 * 1024) / PAGE_SIZE; i < (2 * 1024 * 1024) / PAGE_SIZE; i++) {
        bitmap_set(i);
    }
    
    // Create kernel page directory
    kernel_page_dir = (uint32_t)memory_alloc_pages(1);
    __builtin_memset((void*)kernel_page_dir, 0, PAGE_SIZE);
    
    // Map kernel space (identity map first 16MB) - but keep page 0 unmapped for User
    memory_map_kernel(kernel_page_dir);
    
    // Load kernel page directory
    hal_cpu_enable_paging(kernel_page_dir);
    
    kernel_print("Memory manager initialized\r\n");
    kernel_print("Total memory: ");
    kernel_print_hex(MEMORY_SIZE);
    kernel_print(" bytes\r\n");
}

// Map kernel space identity mapping into a page directory
void memory_map_kernel(uint32_t page_dir) {
    // Map entire kernel memory (16MB) as Supervisor-only
    uint32_t total_pages = MEMORY_SIZE / PAGE_SIZE;
    
    for (uint32_t i = 0; i < total_pages; i++) {
        uint32_t phys_addr = i * PAGE_SIZE;
        uint32_t virt_addr = phys_addr;
        
        // Page 0 remains Supervisor-only (no User flag)
        // Rest of kernel (up to 4MB) remains Supervisor-only
        // Wait, for stability let's keep it all Supervisor-only except what's specifically mapped for user
        memory_map_page(page_dir, virt_addr, phys_addr, 0x03); // Present, RW, Supervisor
    }
}

// Map a single page
void memory_map_page(uint32_t page_dir, uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) {
    uint32_t pd_index = virt_addr >> 22;
    uint32_t pt_index = (virt_addr >> 12) & 0x3FF;
    
    uint32_t* pd = (uint32_t*)page_dir;
    uint32_t* page_table;
    
    if (!(pd[pd_index] & 0x01)) {
        // Create new page table
        page_table = (uint32_t*)memory_alloc_pages(1);
        __builtin_memset(page_table, 0, PAGE_SIZE);
        // OR flags from map request into the directory entry to allow user access to the table itself
        pd[pd_index] = (uint32_t)page_table | (flags & 0x07);
    } else {
        page_table = (uint32_t*)(pd[pd_index] & ~0xFFF);
        // If we are mapping a user page, ensure the page directory entry also has the User flag
        if (flags & 0x04) {
             pd[pd_index] |= 0x04;
        }
    }
    
    page_table[pt_index] = (phys_addr & ~0xFFF) | (flags & 0xFFF) | 0x01;
    hal_cpu_flush_tlb();
}

// Create process page directory
uint32_t memory_create_page_directory(void) {
    uint32_t* pd = (uint32_t*)memory_alloc_pages(1);
    if (!pd) return 0;
    __builtin_memset(pd, 0, PAGE_SIZE);
    return (uint32_t)pd;
}

// Destroy page directory
void memory_destroy_page_directory(uint32_t page_dir) {
    uint32_t* pd = (uint32_t*)page_dir;
    for (int i = 0; i < 1024; i++) {
        if (pd[i] & 0x01) {
            memory_free_pages((void*)(pd[i] & ~0xFFF), 1);
        }
    }
    memory_free_pages((void*)pd, 1);
}

// Allocate physical pages
void* memory_alloc_pages(uint32_t count) {
    for (uint32_t i = 0; i < PHYS_PAGES - count; i++) {
        bool free = true;
        for (uint32_t j = 0; j < count; j++) {
            if (bitmap_test(i + j)) {
                free = false;
                break;
            }
        }
        
        if (free) {
            for (uint32_t j = 0; j < count; j++) {
                bitmap_set(i + j);
            }
            total_allocated_pages += count;
            return (void*)(i * PAGE_SIZE);
        }
    }
    return NULL;
}

// Free physical pages
void memory_free_pages(void* ptr, uint32_t count) {
    uint32_t page = (uint32_t)ptr / PAGE_SIZE;
    for (uint32_t i = 0; i < count; i++) {
        bitmap_clear(page + i);
    }
    total_allocated_pages -= count;
}

// Bitmap helpers
static void bitmap_set(uint32_t page) {
    page_bitmap[page / 32] |= (1 << (page % 32));
}

static void bitmap_clear(uint32_t page) {
    page_bitmap[page / 32] &= ~(1 << (page % 32));
}

static bool bitmap_test(uint32_t page) {
    return (page_bitmap[page / 32] & (1 << (page % 32))) != 0;
}

void memory_get_stats(uint32_t* total, uint32_t* used) {
    if (total) *total = MEMORY_SIZE;
    if (used) *used = total_allocated_pages * PAGE_SIZE;
}
