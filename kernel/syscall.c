// Kernel System Call Handler
// Handles all system calls from user space

#include "kernel.h"
#include "hal.h"
#include "syscall_numbers.h"
#include "ipc_abi.h"
#include <stddef.h>

// Trap frame structure (must match interrupt.c)
typedef struct {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, user_esp, user_ss;
} syscall_frame_t;

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
static status_t sys_debug_print(uint32_t ebx, uint32_t ecx, uint32_t edx);

// System call handler table
static status_t (*syscall_table[256])(uint32_t, uint32_t, uint32_t);

// Initialize system call system
void syscall_init(void) {
    for (int i = 0; i < 256; i++) {
        syscall_table[i] = NULL;
    }
    
    syscall_table[SYS_PROCESS_CREATE] = sys_process_create;
    syscall_table[SYS_PROCESS_EXIT]   = sys_process_exit;
    syscall_table[SYS_PROCESS_YIELD]  = sys_process_yield;
    syscall_table[SYS_PROCESS_KILL]   = sys_process_kill;
    syscall_table[SYS_MEMORY_ALLOC]  = sys_memory_alloc;
    syscall_table[SYS_MEMORY_FREE]   = sys_memory_free;
    syscall_table[SYS_MEMORY_MAP]    = sys_memory_map;
    syscall_table[SYS_IPC_SEND]      = sys_ipc_send;
    syscall_table[SYS_IPC_RECEIVE]   = sys_ipc_receive;
    syscall_table[SYS_IPC_REGISTER]  = sys_ipc_register;
    syscall_table[SYS_DRIVER_REGISTER] = sys_driver_register;
    syscall_table[SYS_DRIVER_REQUEST] = sys_driver_request;
    syscall_table[SYS_SYSTEM_SHUTDOWN] = sys_system_shutdown;
    syscall_table[SYS_DEBUG_PRINT]     = sys_debug_print;
    
    kernel_print("System calls initialized\r\n");
}

// New syscall dispatch function
void syscall_dispatch(void* frame_ptr) {
    syscall_frame_t* frame = (syscall_frame_t*)frame_ptr;
    uint32_t eax = frame->eax;
    uint32_t ebx = frame->ebx;
    uint32_t ecx = frame->ecx;
    uint32_t edx = frame->edx;

    pcb_t* current = scheduler_get_current();
    
    if (eax != SYS_PROCESS_YIELD) {
        kernel_print("Syscall "); kernel_print_hex(eax);
        if (current) { kernel_print(" from PID "); kernel_print_hex(current->pid); }
        kernel_print("\r\n");
    }

    status_t result;
    if (eax >= 256 || !syscall_table[eax]) {
        result = STATUS_NOT_IMPLEMENTED;
    } else {
        result = syscall_table[eax](ebx, ecx, edx);
    }
    
    // Store result in user's EAX register
    frame->eax = (uint32_t)result;
}

// Implementations
static status_t sys_process_create(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)ebx; (void)ecx; (void)edx;
    pcb_t* current = scheduler_get_current();
    pcb_t* proc = process_create(current ? current->pid : 0);
    if (!proc) return STATUS_ERROR;
    return (status_t)proc->pid;
}

static status_t sys_process_exit(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)ecx; (void)edx;
    process_exit(scheduler_get_current(), ebx);
    return STATUS_SUCCESS;
}

static status_t sys_process_yield(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)ebx; (void)ecx; (void)edx;
    scheduler_yield();
    return STATUS_SUCCESS;
}

static status_t sys_process_kill(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)ecx; (void)edx;
    return process_kill(ebx);
}

static status_t sys_memory_alloc(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)ecx; (void)edx;
    uint32_t pages = (ebx + PAGE_SIZE - 1) / PAGE_SIZE;
    void* ptr = memory_alloc_pages(pages);
    if (!ptr) return STATUS_OUT_OF_MEMORY;
    
    pcb_t* current = scheduler_get_current();
    if (current) {
        for (uint32_t i = 0; i < pages; i++) {
            uint32_t addr = (uint32_t)ptr + (i * PAGE_SIZE);
            memory_map_page(current->page_directory, addr, addr, 0x07);
        }
    }
    return (status_t)(uint32_t)ptr;
}

static status_t sys_memory_free(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)ecx; (void)edx;
    memory_free_pages((void*)ebx, 1);
    return STATUS_SUCCESS;
}

static status_t sys_memory_map(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    pcb_t* current = scheduler_get_current();
    if (current) {
        memory_map_page(current->page_directory, ebx, ecx, edx);
    }
    return STATUS_SUCCESS;
}

static status_t sys_ipc_send(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)edx;
    return ipc_send(ebx, (ipc_abi_message_t*)ecx);
}

static status_t sys_ipc_receive(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    return ipc_receive(ebx, (ipc_abi_message_t*)ecx, (edx != 0));
}

static status_t sys_ipc_register(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)edx;
    return ipc_register_handler(ebx, (void(*)(ipc_message_t*))ecx);
}

static status_t sys_driver_register(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)ebx; (void)ecx; (void)edx;
    return STATUS_SUCCESS;
}

static status_t sys_driver_request(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)edx;
    return ipc_send(ebx, (ipc_abi_message_t*)ecx);
}

static status_t sys_system_shutdown(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)ebx; (void)ecx; (void)edx;
    kernel_print("System shutdown requested. Halting.\r\n");
    hal_cpu_disable_interrupts();
    while (1) hal_cpu_halt();
    return STATUS_SUCCESS;
}

static status_t sys_debug_print(uint32_t ebx, uint32_t ecx, uint32_t edx) {
    (void)ecx; (void)edx;
    kernel_print((const char*)ebx);
    return STATUS_SUCCESS;
}
