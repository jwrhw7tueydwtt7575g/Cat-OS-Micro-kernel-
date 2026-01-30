// Kernel Process Management
// Process creation, destruction, and lifecycle management

#include "kernel.h"
#include "hal.h"
#include <stddef.h>

// Process table
static pcb_t process_table[MAX_PROCESSES];
static bool process_used[MAX_PROCESSES];

// Forward declarations
static uint32_t process_allocate_pid(void);
static void process_free_pid(uint32_t pid);
static void process_cleanup(pcb_t* process);
void process_setup_stack(pcb_t* process, uint32_t entry_point);

// First run handler for USER processes (uses iret)
__asm__ (
".global first_run_user_handler\n"
"first_run_user_handler:\n"
    "pop %gs\n"
    "pop %fs\n"
    "pop %es\n"
    "pop %ds\n"
    "popa\n"
    "add $8, %esp\n"
    "iret\n"
);
extern void first_run_user_handler(void);

// Initialize process management
void process_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_used[i] = false;
        __builtin_memset(&process_table[i], 0, sizeof(pcb_t));
    }
    kernel_print("Process management initialized\r\n");
}

// Create new process (kernel or user)
pcb_t* process_create_internal(uint32_t parent_pid, bool is_user) {
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!process_used[i]) {
            slot = i;
            break;
        }
    }
    if (slot == -1) return NULL;
    
    uint32_t pid = process_allocate_pid();
    if (pid == 0) return NULL;
    
    pcb_t* process = &process_table[slot];
    __builtin_memset(process, 0, sizeof(pcb_t));
    
    process->pid = pid;
    process->parent_pid = parent_pid;
    process->state = PROCESS_CREATED;
    process->priority = 5;
    process->is_user = is_user;
    
    // Each process gets its own page directory
    process->page_directory = memory_create_page_directory();
    if (!process->page_directory) return NULL;
    
    // Identity map kernel into process space
    memory_map_kernel(process->page_directory);
    
    process->kernel_stack = (uint32_t)memory_alloc_pages(2);
    if (!process->kernel_stack) {
        process_cleanup(process);
        return NULL;
    }

    // Map kernel stack into process page directory (Supervisor RW)
    for (int i = 0; i < 2; i++) {
        uint32_t addr = process->kernel_stack + i * PAGE_SIZE;
        memory_map_page(process->page_directory, addr, addr, 0x03);
    }

    if (is_user) {
        process->user_stack = (uint32_t)memory_alloc_pages(4);
        if (!process->user_stack) {
            process_cleanup(process);
            return NULL;
        }
        // Map user stack (User RW)
        for (int i = 0; i < 4; i++) {
            uint32_t addr = process->user_stack + i * PAGE_SIZE;
            memory_map_page(process->page_directory, addr, addr, 0x07);
        }
    }
    
    process_used[slot] = true;
    return process;
}

// Public API
pcb_t* process_create(uint32_t parent_pid) {
    return process_create_internal(parent_pid, true); // Default to user process
}

pcb_t* process_create_kernel(void) {
    return process_create_internal(0, false);
}

void process_exit(pcb_t* process, uint32_t exit_code) {
    if (!process) return;
    
    uint32_t pid = process->pid;
    kernel_print("Terminating Process ");
    kernel_print_hex(pid);
    kernel_print("\r\n");

    process->state = PROCESS_TERMINATED;
    process->exit_code = exit_code;
    
    scheduler_remove_process(process);
    
    process_cleanup(process);
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (&process_table[i] == process) {
            process_used[i] = false;
            break;
        }
    }
    process_free_pid(pid);
}

status_t process_kill(uint32_t pid) {
    pcb_t* process = process_find(pid);
    if (!process) return STATUS_NOT_FOUND;
    process_exit(process, 0);
    return STATUS_SUCCESS;
}

pcb_t* process_find(uint32_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_used[i] && process_table[i].pid == pid) return &process_table[i];
    }
    return NULL;
}

void process_setup_stack(pcb_t* process, uint32_t entry_point) {
    if (!process || entry_point == 0) {
        kernel_panic("Invalid process setup");
    }

    uint32_t* kernel_stack = (uint32_t*)(process->kernel_stack + KERNEL_STACK_SIZE);
    
    if (process->is_user) {
        // Prepare for iret to Ring 3
        *--kernel_stack = 0x23; // SS
        *--kernel_stack = process->user_stack + USER_STACK_SIZE; // ESP
        *--kernel_stack = 0x202; // EFLAGS
        *--kernel_stack = 0x1B; // CS
        *--kernel_stack = entry_point; // EIP
        
        // Dummy error code and interrupt number
        *--kernel_stack = 0;
        *--kernel_stack = 0;
        
        // pusha
        *--kernel_stack = 0; // eax
        *--kernel_stack = 0; // ecx
        *--kernel_stack = 0; // edx
        *--kernel_stack = 0; // ebx
        *--kernel_stack = 0; // esp
        *--kernel_stack = 0; // ebp
        *--kernel_stack = 0; // esi
        *--kernel_stack = 0; // edi
        
        // segments
        *--kernel_stack = 0x23; // ds
        *--kernel_stack = 0x23; // es
        *--kernel_stack = 0x23; // fs
        *--kernel_stack = 0x23; // gs
        
        // Return to first_run_user_handler
        *--kernel_stack = (uint32_t)first_run_user_handler;
    } else {
        // Kernel Task: Just prepare for a standard 'ret' in context_switch_asm
        // No iret frame, no first_run_handler needed.
        // Stack after popping in context_switch_asm: [ret_addr]
        
        // The return address for context_switch_asm's 'ret'
        *--kernel_stack = entry_point;
    }
    
    // Preparation for context_switch_asm's pops:
    // "pop %edi; pop %esi; pop %ebx; pop %ebp; popfd; ret"
    *--kernel_stack = 0x202; // Initial EFLAGS
    *--kernel_stack = 0;     // EBP
    *--kernel_stack = 0;     // EBX
    *--kernel_stack = 0;     // ESI
    *--kernel_stack = 0;     // EDI
    
    process->registers[4] = (uint32_t)kernel_stack;
}

static void process_cleanup(pcb_t* process) {
    if (!process) return;
    if (process->page_directory && process->page_directory != kernel_page_dir) {
        memory_destroy_page_directory(process->page_directory);
    }
    if (process->kernel_stack) memory_free_pages((void*)process->kernel_stack, 2);
    if (process->user_stack) memory_free_pages((void*)process->user_stack, 4);
}

static uint32_t process_allocate_pid(void) {
    static uint32_t next_pid = 1;
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        uint32_t pid = (next_pid + i) % MAX_PROCESSES;
        if (pid == 0) continue;
        bool used = false;
        for (int j = 0; j < MAX_PROCESSES; j++) {
            if (process_used[j] && process_table[j].pid == pid) { used = true; break; }
        }
        if (!used) { next_pid = pid + 1; return pid; }
    }
    return 0;
}

static void process_free_pid(uint32_t pid) { (void)pid; }
