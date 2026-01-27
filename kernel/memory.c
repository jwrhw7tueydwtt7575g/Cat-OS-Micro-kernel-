// Kernel Memory Manager
// Handles physical memory allocation and paging

#include "kernel.h"
#include "hal.h"
#include <stddef.h>

// Memory management constants
#define MEMORY_SIZE 0x1000000      // 16MB
#define PAGE_COUNT (MEMORY_SIZE / PAGE_SIZE)
#define BITMAP_SIZE (PAGE_COUNT / 8)

// Physical memory bitmap
static uint8_t memory_bitmap[BITMAP_SIZE];
static uint32_t memory_used_pages = 0;
static uint32_t memory_total_pages = PAGE_COUNT;

// Kernel page directory
static uint32_t kernel_page_dir;

// Forward declarations
static void bitmap_set_bit(uint32_t page);
static void bitmap_clear_bit(uint32_t page);
static bool bitmap_test_bit(uint32_t page);
static uint32_t bitmap_find_free(void);

// Initialize memory manager
void memory_init(void) {
    // Clear bitmap
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        memory_bitmap[i] = 0;
    }
    
    // Mark first 4MB as used (kernel space)
    uint32_t kernel_pages = 0x400000 / PAGE_SIZE;  // 4MB
    for (uint32_t i = 0; i < kernel_pages; i++) {
        bitmap_set_bit(i);
        memory_used_pages++;
    }
    
    // Create kernel page directory
    kernel_page_dir = memory_create_page_directory();
    
    // Map kernel space identity mapping
    for (uint32_t i = 0; i < kernel_pages; i++) {
        uint32_t phys_addr = i * PAGE_SIZE;
        uint32_t virt_addr = phys_addr;
        memory_map_page(kernel_page_dir, virt_addr, phys_addr, 0x03);  // Read/write, present
    }
    
    // Enable paging
    hal_cpu_enable_paging(kernel_page_dir);
    
    kernel_print("Memory manager initialized\r\n");
    kernel_print("Total memory: ");
    kernel_print_hex(MEMORY_SIZE);
    kernel_print(" bytes\r\n");
}

// Allocate a physical page
void* memory_alloc_physical(void) {
    uint32_t page = bitmap_find_free();
    if (page == 0xFFFFFFFF) {
        return NULL;  // Out of memory
    }
    
    bitmap_set_bit(page);
    memory_used_pages++;
    
    return (void*)(page * PAGE_SIZE);
}

// Free a physical page
void memory_free_physical(void* addr) {
    uint32_t page = (uint32_t)addr / PAGE_SIZE;
    
    if (bitmap_test_bit(page)) {
        bitmap_clear_bit(page);
        memory_used_pages--;
    }
}

// Create a new page directory
uint32_t memory_create_page_directory(void) {
    uint32_t* page_dir = (uint32_t*)memory_alloc_physical();
    if (!page_dir) {
        return 0;
    }
    
    // Clear page directory
    for (int i = 0; i < 1024; i++) {
        page_dir[i] = 0;
    }
    
    return (uint32_t)page_dir;
}

// Destroy a page directory
void memory_destroy_page_directory(uint32_t page_dir) {
    uint32_t* pd = (uint32_t*)page_dir;
    
    // Free all page tables
    for (int i = 0; i < 1024; i++) {
        if (pd[i] & 0x01) {  // Present bit
            uint32_t page_table = pd[i] & 0xFFFFF000;
            memory_free_physical((void*)page_table);
        }
    }
    
    // Free page directory itself
    memory_free_physical((void*)page_dir);
}

// Map a virtual page to a physical page
void memory_map_page(uint32_t page_dir, uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) {
    uint32_t* pd = (uint32_t*)page_dir;
    uint32_t pd_index = virt_addr >> 22;
    uint32_t pt_index = (virt_addr >> 12) & 0x3FF;
    
    // Check if page table exists
    if (!(pd[pd_index] & 0x01)) {
        // Allocate new page table
        uint32_t* page_table = (uint32_t*)memory_alloc_physical();
        if (!page_table) {
            return;  // Out of memory
        }
        
        // Clear page table
        for (int i = 0; i < 1024; i++) {
            page_table[i] = 0;
        }
        
        // Set page directory entry
        pd[pd_index] = (uint32_t)page_table | 0x03;  // Read/write, present
    }
    
    // Get page table
    uint32_t* pt = (uint32_t*)(pd[pd_index] & 0xFFFFF000);
    
    // Set page table entry
    pt[pt_index] = phys_addr | flags;
    
    // Flush TLB
    hal_cpu_flush_tlb();
}

// Unmap a virtual page
void memory_unmap_page(uint32_t page_dir, uint32_t virt_addr) {
    uint32_t* pd = (uint32_t*)page_dir;
    uint32_t pd_index = virt_addr >> 22;
    uint32_t pt_index = (virt_addr >> 12) & 0x3FF;
    
    // Check if page table exists
    if (!(pd[pd_index] & 0x01)) {
        return;  // Page not mapped
    }
    
    // Get page table
    uint32_t* pt = (uint32_t*)(pd[pd_index] & 0xFFFFF000);
    
    // Clear page table entry
    pt[pt_index] = 0;
    
    // Flush TLB
    hal_cpu_flush_tlb();
}

// Get memory usage statistics
void memory_get_stats(uint32_t* total, uint32_t* used, uint32_t* free) {
    if (total) *total = memory_total_pages * PAGE_SIZE;
    if (used) *used = memory_used_pages * PAGE_SIZE;
    if (free) *free = (memory_total_pages - memory_used_pages) * PAGE_SIZE;
}

// Bitmap helper functions
static void bitmap_set_bit(uint32_t page) {
    memory_bitmap[page / 8] |= (1 << (page % 8));
}

static void bitmap_clear_bit(uint32_t page) {
    memory_bitmap[page / 8] &= ~(1 << (page % 8));
}

static bool bitmap_test_bit(uint32_t page) {
    return (memory_bitmap[page / 8] & (1 << (page % 8))) != 0;
}

static uint32_t bitmap_find_free(void) {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        if (memory_bitmap[i] != 0xFF) {
            for (uint32_t j = 0; j < 8; j++) {
                if (!(memory_bitmap[i] & (1 << j))) {
                    return i * 8 + j;
                }
            }
        }
    }
    return 0xFFFFFFFF;  // No free pages
}

// Allocate multiple contiguous pages
void* memory_alloc_pages(uint32_t count) {
    uint32_t start_page = 0;
    uint32_t found_pages = 0;
    
    for (uint32_t i = 0; i < memory_total_pages; i++) {
        if (!bitmap_test_bit(i)) {
            if (found_pages == 0) {
                start_page = i;
            }
            found_pages++;
            
            if (found_pages == count) {
                // Allocate the pages
                for (uint32_t j = start_page; j < start_page + count; j++) {
                    bitmap_set_bit(j);
                    memory_used_pages++;
                }
                return (void*)(start_page * PAGE_SIZE);
            }
        } else {
            found_pages = 0;
        }
    }
    
    return NULL;  // Not enough contiguous pages
}

// Free multiple contiguous pages
void memory_free_pages(void* addr, uint32_t count) {
    uint32_t start_page = (uint32_t)addr / PAGE_SIZE;
    
    for (uint32_t i = 0; i < count; i++) {
        memory_free_physical((void*)((start_page + i) * PAGE_SIZE));
    }
}
