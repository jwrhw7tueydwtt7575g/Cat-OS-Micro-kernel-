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
void process_setup_stack(pcb_t* process, uint32_t entry_point);
static void process_cleanup(pcb_t* process);

// Initialize process management
void process_init(void) {
    // Clear process table
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_used[i] = false;
        __builtin_memset(&process_table[i], 0, sizeof(pcb_t));
    }
    
    kernel_print("Process management initialized\r\n");
}

// Create new process
pcb_t* process_create(uint32_t parent_pid) {
    // Find free process slot
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!process_used[i]) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return NULL;  // No free process slots
    }
    
    // Allocate PID
    uint32_t pid = process_allocate_pid();
    if (pid == 0) {
        return NULL;  // No free PIDs
    }
    
    // Initialize PCB
    pcb_t* process = &process_table[slot];
    __builtin_memset(process, 0, sizeof(pcb_t));
    
    process->pid = pid;
    process->parent_pid = parent_pid;
    process->state = PROCESS_CREATED;
    process->priority = 5;  // Default priority
    process->cpu_time = 0;
    process->page_directory = memory_create_page_directory();
    process->kernel_stack = (uint32_t)memory_alloc_pages(2);  // 8KB kernel stack
    process->user_stack = (uint32_t)memory_alloc_pages(4);    // 16KB user stack
    process->capabilities = 0;
    process->exit_code = 0;
    process->waiting_for = 0;
    process->next = NULL;
    process->prev = NULL;
    
    if (!process->page_directory || !process->kernel_stack || !process->user_stack) {
        process_cleanup(process);
        process_free_pid(pid);
        process_used[slot] = false;
        return NULL;
    }
    
    // Setup process stack
    process_setup_stack(process, 0);  // Entry point will be set later
    
    // Mark process slot as used
    process_used[slot] = true;
    
    // Grant basic capabilities
    capability_grant(pid, CAP_PROCESS, PERM_CREATE | PERM_DELETE, 0);
    capability_grant(pid, CAP_MEMORY, PERM_ALLOC | PERM_FREE, 0);
    capability_grant(pid, CAP_IPC, PERM_READ | PERM_WRITE, 0);
    
    kernel_print("Process created with PID ");
    kernel_print_hex(pid);
    kernel_print("\r\n");
    
    return process;
}

// Exit process
void process_exit(pcb_t* process, uint32_t exit_code) {
    if (!process) {
        return;
    }
    
    process->state = PROCESS_TERMINATED;
    process->exit_code = exit_code;
    
    kernel_print("Process ");
    kernel_print_hex(process->pid);
    kernel_print(" exited with code ");
    kernel_print_hex(exit_code);
    kernel_print("\r\n");
    
    // Notify parent process
    pcb_t* parent = scheduler_find_process(process->parent_pid);
    if (parent) {
        // Send exit notification to parent
        size_t payload_size = sizeof(uint32_t);
        ipc_message_t* msg = (ipc_message_t*)memory_alloc_pages(
            (sizeof(ipc_message_t) + payload_size + PAGE_SIZE - 1) / PAGE_SIZE
        );
        if (msg) {
            msg->msg_type = MSG_SIGNAL;
            msg->sender_pid = process->pid;
            msg->receiver_pid = parent->pid;
            msg->data_size = payload_size;
            msg->next = NULL;
            *(uint32_t*)msg->data = process->pid;
            ipc_abi_message_t abi_msg;
            abi_msg.msg_id = msg->msg_id;
            abi_msg.sender_pid = msg->sender_pid;
            abi_msg.receiver_pid = msg->receiver_pid;
            abi_msg.msg_type = msg->msg_type;
            abi_msg.flags = msg->flags;
            abi_msg.timestamp = msg->timestamp;
            abi_msg.data_size = msg->data_size;
            if (msg->data_size > 256) {
                abi_msg.data_size = 256;
            }
            // Manual memory copy since string.h is not available
            for (uint32_t i = 0; i < abi_msg.data_size; i++) {
                abi_msg.data[i] = msg->data[i];
            }
            ipc_send(parent->pid, &abi_msg);
        }
    }
    
    // Orphan children
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_used[i] && process_table[i].parent_pid == process->pid) {
            process_table[i].parent_pid = 0;  // Adopt by init
        }
    }
    
    // Remove from scheduler
    scheduler_remove_process(process);
    
    // Clean up process resources
    process_cleanup(process);
    
    // Free process slot
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (&process_table[i] == process) {
            process_used[i] = false;
            break;
        }
    }
    
    process_free_pid(process->pid);
}

// Kill process
status_t process_kill(uint32_t pid) {
    pcb_t* process = process_find(pid);
    if (!process) {
        return STATUS_NOT_FOUND;
    }
    
    // Check if caller has permission to kill
    pcb_t* current = scheduler_get_current();
    if (!current) {
        return STATUS_PERMISSION_DENIED;
    }
    
    // Can only kill own processes unless you have system capability
    if (current->pid != process->pid && 
        capability_check(current->pid, CAP_SYSTEM, PERM_DELETE) != STATUS_SUCCESS) {
        return STATUS_PERMISSION_DENIED;
    }
    
    process_exit(process, 0);  // Exit code 0 for killed
    return STATUS_SUCCESS;
}

// Find process by PID
pcb_t* process_find(uint32_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_used[i] && process_table[i].pid == pid) {
            return &process_table[i];
        }
    }
    return NULL;
}

// Get process list
status_t process_list(pcb_t* processes, uint32_t* count) {
    uint32_t found = 0;
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_used[i]) {
            if (processes && found < *count) {
                processes[found] = process_table[i];
            }
            found++;
        }
    }
    
    if (count) *count = found;
    return STATUS_SUCCESS;
}

// Get process information
status_t process_get_info(uint32_t pid, pcb_t* info) {
    pcb_t* process = process_find(pid);
    if (!process || !info) {
        return STATUS_INVALID_PARAM;
    }
    
    *info = *process;
    return STATUS_SUCCESS;
}

// Set process priority
status_t process_set_priority(uint32_t pid, uint32_t priority) {
    pcb_t* process = process_find(pid);
    if (!process) {
        return STATUS_NOT_FOUND;
    }
    
    // Check if caller has permission
    pcb_t* current = scheduler_get_current();
    if (!current) {
        return STATUS_PERMISSION_DENIED;
    }
    
    // Can only change own priority unless you have system capability
    if (current->pid != process->pid && 
        capability_check(current->pid, CAP_SYSTEM, PERM_WRITE) != STATUS_SUCCESS) {
        return STATUS_PERMISSION_DENIED;
    }
    
    scheduler_set_priority(process, priority);
    return STATUS_SUCCESS;
}

// Allocate PID
static uint32_t process_allocate_pid(void) {
    static uint32_t next_pid = 1;
    
    // Find next available PID
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        uint32_t pid = (next_pid + i) % MAX_PROCESSES;
        if (pid == 0) continue;  // Skip PID 0 (kernel)
        
        bool used = false;
        for (int j = 0; j < MAX_PROCESSES; j++) {
            if (process_used[j] && process_table[j].pid == pid) {
                used = true;
                break;
            }
        }
        
        if (!used) {
            next_pid = pid + 1;
            return pid;
        }
    }
    
    return 0;  // No free PIDs
}

// Free PID
static void process_free_pid(uint32_t pid) {
    (void)pid;  // Suppress unused parameter warning
    // PIDs are reused automatically by allocate_pid
}

// Setup process stack
void process_setup_stack(pcb_t* process, uint32_t entry_point) {
    if (!process) {
        return;
    }
    
    // Setup kernel stack
    uint32_t* kernel_stack = (uint32_t*)(process->kernel_stack + KERNEL_STACK_SIZE);
    
    // Initial iret frame
    *--kernel_stack = 0x23;             // SS (user data segment)
    *--kernel_stack = process->user_stack + USER_STACK_SIZE;  // ESP
    *--kernel_stack = 0x202;           // EFLAGS (interrupts enabled)
    *--kernel_stack = 0x1B;            // CS (user code segment)
    *--kernel_stack = entry_point;     // EIP
    
    // Dummy error code and interrupt number (for interrupt_common's add $8, %esp)
    *--kernel_stack = 0;               // error_code
    *--kernel_stack = 0;               // interrupt_number
    
    // pusha (EAX, ECX, EDX, EBX, Original_ESP, EBP, ESI, EDI)
    for (int i = 0; i < 8; i++) {
        *--kernel_stack = 0;           // Dummy register values
    }
    
    // Segments (GS, FS, ES, DS)
    *--kernel_stack = 0x23;            // DS
    *--kernel_stack = 0x23;            // ES
    *--kernel_stack = 0x23;            // FS
    *--kernel_stack = 0x23;            // GS
    
    // Save stack pointer to pcb's registers[4] (ESP index in scheduler)
    process->registers[4] = (uint32_t)kernel_stack;
}

// Clean up process resources
static void process_cleanup(pcb_t* process) {
    if (!process) {
        return;
    }
    
    // Free page directory
    if (process->page_directory) {
        memory_destroy_page_directory(process->page_directory);
    }
    
    // Free stacks
    if (process->kernel_stack) {
        memory_free_pages((void*)process->kernel_stack, 2);
    }
    
    if (process->user_stack) {
        memory_free_pages((void*)process->user_stack, 4);
    }
    
    // Revoke all capabilities
    capability_revoke(process->pid, 0, 0);  // Revoke all capabilities
    
    // Clear IPC queue
    ipc_clear_queue(process->pid);
}

// Get process statistics
void process_get_stats(uint32_t* total_processes, uint32_t* active_processes) {
    uint32_t total = 0;
    uint32_t active = 0;
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_used[i]) {
            total++;
            if (process_table[i].state != PROCESS_TERMINATED) {
                active++;
            }
        }
    }
    
    if (total_processes) *total_processes = total;
    if (active_processes) *active_processes = active;
}
