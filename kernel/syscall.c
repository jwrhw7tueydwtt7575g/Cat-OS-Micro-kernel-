// Kernel System Call Dispatcher
// Handles all system calls from user space

#include "kernel.h"
#include "hal.h"
#include <stddef.h>

// System call handler function pointer
typedef status_t (*syscall_func_t)(uint32_t ebx, uint32_t ecx, uint32_t edx);

// System call table
static syscall_func_t syscall_table[256];

// Forward declarations
static status_t sys_process_create(uint32_t ebx, uint32_t ecx, uint32_t edx);
static status_t sys_process_exit(uint32_t ebx, uint32_t ecx, uint32_t edx);
static status_t sys_process_yield(uint32_t ebx, uint32_t ecx, uint32_t edx);
static status_t sys_process_kill(uint32_t ebx, uint32_t ecx, uint32_t edx);
static status_t sys_memory_alloc(uint32_t ebx, uint32_t ecx, uint32_t edx);
static status_t sys_memory_free(uint32_t ebx, uint32_t ecx, uint32_t edx);
static status_t sys_memory_map(uint32_t ebx, uint32_t ecx, uint32_t edx);
static status_t sys_ipc_send(uint32_t ebx, uint32_t ecx, uint32_t edx);
static status_t sys_ipc_receive(uint32_t ebx, uint32_t ecx, uint32_t edx);
static status_t sys_ipc_register(uint32_t ebx, uint32_t ecx, uint32_t edx);
static status_t sys_driver_register(uint32_t ebx, uint32_t ecx, uint32_t edx);
static status_t sys_driver_request(uint32_t ebx, uint32_t ecx, uint32_t edx);
static status_t sys_system_shutdown(uint32_t ebx, uint32_t ecx, uint32_t edx);

// Initialize system call system
void syscall_init(void) {
    // Clear syscall table
    for (int i = 0; i < 256; i++) {
        syscall_table[i] = NULL;
    }
    
    // Register system calls
    syscall_table[SYS_PROCESS_CREATE] = sys_process_create;
    syscall_table[SYS_PROCESS_EXIT] = sys_process_exit;
    syscall_table[SYS_PROCESS_YIELD] = sys_process_yield;
    syscall_table[SYS_PROCESS_KILL] = sys_process_kill;
    syscall_table[SYS_MEMORY_ALLOC] = sys_memory_alloc;
    syscall_table[SYS_MEMORY_FREE] = sys_memory_free;
    syscall_table[SYS_MEMORY_MAP] = sys_memory_map;
    syscall_table[SYS_IPC_SEND] = sys_ipc_send;
    syscall_table[SYS_IPC_RECEIVE] = sys_ipc_receive;
    syscall_table[SYS_IPC_REGISTER] = sys_ipc_register;
    syscall_table[SYS_DRIVER_REGISTER] = sys_driver_register;
    syscall_table[SYS_DRIVER_REQUEST] = sys_driver_request;
    syscall_table[SYS_SYSTEM_SHUTDOWN] = sys_system_shutdown;
    
    kernel_print("System call system initialized\r\n");
}

// System call handler (called from interrupt handler)
void syscall_handler(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    // Check if syscall number is valid
    if (eax >= 256 || !syscall_table[eax]) {
        eax = STATUS_INVALID_PARAM;
        return;
    }
    
    // Get current process
    pcb_t* current = scheduler_get_current();
    if (!current) {
        eax = STATUS_PERMISSION_DENIED;
        return;
    }
    
    // Check capabilities for this syscall
    if (capability_check(current->pid, eax, 0) != STATUS_SUCCESS) {
        eax = STATUS_PERMISSION_DENIED;
        return;
    }
    
    // Call syscall function
    status_t result = syscall_table[eax](ebx, ecx, edx);
    
    // Return result in EAX
    eax = result;
}

// Process management system calls
static status_t sys_process_create(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)ebx; (void)ecx; (void)edx;  // Suppress unused parameter warnings
    pcb_t* parent = scheduler_get_current();
    if (!parent) {
        return STATUS_PERMISSION_DENIED;
    }
    
    // Create new process
    pcb_t* child = process_create(parent->pid);
    if (!child) {
        return STATUS_OUT_OF_MEMORY;
    }
    
    // Add to scheduler
    scheduler_add_process(child);
    
    // Return child PID
    return child->pid;
}

static status_t sys_process_exit(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)ecx; (void)edx;  // Suppress unused parameter warnings
    pcb_t* current = scheduler_get_current();
    if (!current) {
        return STATUS_PERMISSION_DENIED;
    }
    
    process_exit(current, ebx);  // EBX contains exit code
    return STATUS_SUCCESS;
}

static status_t sys_process_yield(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)ebx; (void)ecx; (void)edx;  // Suppress unused parameter warnings
    scheduler_yield();
    return STATUS_SUCCESS;
}

static status_t sys_process_kill(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)ecx; (void)edx;  // Suppress unused parameter warnings
    uint32_t target_pid = ebx;
    return process_kill(target_pid);
}

// Memory management system calls
static status_t sys_memory_alloc(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)ecx; (void)edx;  // Suppress unused parameter warnings
    uint32_t size = ebx;
    if (size == 0) {
        return STATUS_INVALID_PARAM;
    }
    
    // Allocate pages
    uint32_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    void* memory = memory_alloc_pages(pages);
    
    if (!memory) {
        return STATUS_OUT_OF_MEMORY;
    }
    
    // Map into current process address space
    pcb_t* current = scheduler_get_current();
    if (current && current->page_directory) {
        for (uint32_t i = 0; i < pages; i++) {
            uint32_t phys_addr = (uint32_t)memory + (i * PAGE_SIZE);
            uint32_t virt_addr = (uint32_t)memory + (i * PAGE_SIZE);
            memory_map_page(current->page_directory, virt_addr, phys_addr, 0x03);
        }
    }
    
    return (uint32_t)memory;
}

static status_t sys_memory_free(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)ecx; (void)edx;  // Suppress unused parameter warnings
    void* memory = (void*)ebx;
    if (!memory) {
        return STATUS_INVALID_PARAM;
    }
    
    // Free pages
    memory_free_pages(memory, 1);  // Simplified: assume 1 page
    
    return STATUS_SUCCESS;
}

static status_t sys_memory_map(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    uint32_t virt_addr = ebx;
    uint32_t phys_addr = ecx;
    uint32_t flags = edx;
    
    pcb_t* current = scheduler_get_current();
    if (!current) {
        return STATUS_PERMISSION_DENIED;
    }
    
    memory_map_page(current->page_directory, virt_addr, phys_addr, flags);
    return STATUS_SUCCESS;
}

// IPC system calls
static status_t sys_ipc_send(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)edx;  // Suppress unused parameter warning
    uint32_t receiver_pid = ebx;
    ipc_abi_message_t* msg = (ipc_abi_message_t*)ecx;
    
    return ipc_send(receiver_pid, msg);
}

static status_t sys_ipc_receive(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    uint32_t sender_pid = ebx;
    ipc_abi_message_t* msg = (ipc_abi_message_t*)ecx;
    bool block = (edx != 0);
    
    return ipc_receive(sender_pid, msg, block);
}

static status_t sys_ipc_register(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)edx;  // Suppress unused parameter warning
    uint32_t msg_type = ebx;
    void (*handler)(ipc_message_t*) = (void(*)(ipc_message_t*))ecx;
    
    return ipc_register_handler(msg_type, handler);
}

// Driver system calls
static status_t sys_driver_register(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)edx;  // Suppress unused parameter warning
    char* name = (char*)ebx;
    uint32_t capabilities = ecx;
    
    // Check driver capabilities
    pcb_t* current = scheduler_get_current();
    if (!current) {
        return STATUS_PERMISSION_DENIED;
    }
    
    // Create driver capability
    capability_t* cap = capability_create(current->pid, CAP_DRIVER, capabilities);
    if (!cap) {
        return STATUS_OUT_OF_MEMORY;
    }
    
    kernel_print("Driver registered: ");
    kernel_print(name);
    kernel_print("\r\n");
    
    return STATUS_SUCCESS;
}

static status_t sys_driver_request(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)edx;  // Suppress unused parameter warning
    uint32_t driver_pid = ebx;
    ipc_abi_message_t* request = (ipc_abi_message_t*)ecx;
    
    return ipc_send(driver_pid, request);
}

// System system calls
static status_t sys_system_shutdown(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)ebx; (void)ecx; (void)edx;  // Suppress unused parameter warnings
    kernel_print("System shutdown requested\r\n");
    
    // Check if current process has system capability
    pcb_t* current = scheduler_get_current();
    if (!current || capability_check(current->pid, CAP_SYSTEM, 0) != STATUS_SUCCESS) {
        return STATUS_PERMISSION_DENIED;
    }
    
    // Shutdown system
    hal_cpu_disable_interrupts();
    kernel_print("System halted\r\n");
    
    while (1) {
        hal_cpu_halt();
    }
    
    return STATUS_SUCCESS;
}

// Register custom system call
status_t syscall_register(uint32_t syscall_num, syscall_func_t handler) {
    if (syscall_num >= 256 || !handler) {
        return STATUS_INVALID_PARAM;
    }
    
    syscall_table[syscall_num] = handler;
    return STATUS_SUCCESS;
}

// Get system call statistics
void syscall_get_stats(uint32_t* total_calls, uint32_t* registered_calls) {
    uint32_t registered = 0;
    
    for (int i = 0; i < 256; i++) {
        if (syscall_table[i]) {
            registered++;
        }
    }
    
    if (total_calls) *total_calls = 0;  // Not implemented
    if (registered_calls) *registered_calls = registered;
}
