// Kernel interfaces and data structures
#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include "types.h"
#include "syscall_numbers.h"
#include "ipc_abi.h"

// Process Control Block
typedef struct pcb {
    uint32_t pid;              // Process identifier
    uint32_t parent_pid;       // Parent process ID
    uint32_t state;            // Current process state
    uint32_t priority;         // Process priority
    uint32_t cpu_time;         // Total CPU time used
    uint32_t page_directory;   // Page directory physical address
    uint32_t kernel_stack;     // Kernel stack pointer
    uint32_t user_stack;       // User stack pointer
    uint32_t capabilities;     // Process capabilities
    uint32_t exit_code;        // Process exit status
    uint32_t waiting_for;      // Event/process waiting for
    struct pcb* next;          // Next process in queue
    struct pcb* prev;          // Previous process in queue
    uint32_t registers[16];    // Saved registers
    bool is_user;              // Whether this is a user-space process
} pcb_t;

// Message structure for IPC
typedef struct ipc_message {
    uint32_t msg_id;           // Unique message identifier
    uint32_t sender_pid;       // Sender process ID
    uint32_t receiver_pid;     // Receiver process ID
    uint32_t msg_type;         // Message type identifier
    uint32_t flags;            // Message flags (sync/async)
    uint32_t timestamp;        // Message timestamp
    uint32_t data_size;        // Size of message data
    struct ipc_message* next;  // Next message in queue
    uint8_t data[];            // Message payload
} ipc_message_t;

// Capability structure
typedef struct {
    uint32_t cap_id;          // Unique capability identifier
    uint32_t owner_pid;       // Process that owns capability
    uint32_t cap_type;        // Type of capability
    uint32_t permissions;     // Permission bitmask
    uint32_t resource_id;      // Resource this capability controls
    uint32_t expiration_time;  // Optional expiration time
    uint8_t signature[16];    // Cryptographic signature
} capability_t;

// Memory management
#define PAGE_SIZE 4096
#define PAGE_ALIGN(addr) (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

// Process management
#define MAX_PROCESSES 64
#define KERNEL_STACK_SIZE 8192
#define USER_STACK_SIZE 16384

// System call numbers
#define SYS_PROCESS_CREATE    0x01
#define SYS_PROCESS_EXIT      0x02
#define SYS_PROCESS_YIELD     0x03
#define SYS_PROCESS_KILL      0x04
#define SYS_MEMORY_ALLOC      0x10
#define SYS_MEMORY_FREE       0x11
#define SYS_MEMORY_MAP        0x12
#define SYS_IPC_SEND          0x20
#define SYS_IPC_RECEIVE       0x21
#define SYS_IPC_REGISTER      0x22
#define SYS_DRIVER_REGISTER   0x30
#define SYS_DRIVER_REQUEST    0x31
#define SYS_SYSTEM_SHUTDOWN   0x40

// Kernel function prototypes
void kernel_main(void);
void kernel_init(void);

// Scheduler functions
void scheduler_init(void);
void scheduler_add_process(pcb_t* process);
void scheduler_remove_process(pcb_t* process);
void scheduler_tick(void);
void scheduler_yield(void);
pcb_t* scheduler_get_current(void);
void scheduler_switch_to(pcb_t* next);

// Memory management functions
void memory_init(void);
void* memory_alloc_physical(void);
void memory_free_physical(void* addr);
uint32_t memory_create_page_directory(void);
void memory_destroy_page_directory(uint32_t page_dir);
void memory_map_page(uint32_t page_dir, uint32_t virt_addr, uint32_t phys_addr, uint32_t flags);
void memory_unmap_page(uint32_t page_dir, uint32_t virt_addr);
void memory_map_kernel(uint32_t page_dir);
extern uint32_t kernel_page_dir;

// IPC functions
void ipc_init(void);
status_t ipc_send(uint32_t receiver_pid, ipc_abi_message_t* user_msg);
status_t ipc_receive(uint32_t sender_pid, ipc_abi_message_t* user_msg, bool block);
status_t ipc_register_handler(uint32_t msg_type, void (*handler)(ipc_message_t*));
status_t ipc_clear_queue(uint32_t pid);
status_t ipc_broadcast(uint32_t msg_type, ipc_abi_message_t* msg);

// System call functions
void syscall_init(void);
uint32_t syscall_handler(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

// Capability functions
void capability_init(void);
capability_t* capability_create(uint32_t owner_pid, uint32_t cap_type, uint32_t permissions);
status_t capability_check(uint32_t pid, uint32_t cap_type, uint32_t permissions);
void capability_destroy(capability_t* cap);

// Process management functions
pcb_t* process_create(uint32_t parent_pid);
pcb_t* process_create_kernel(void);
void process_exit(pcb_t* process, uint32_t exit_code);
status_t process_kill(uint32_t pid);
pcb_t* process_find(uint32_t pid);

// Scheduler functions (forward declarations)
pcb_t* scheduler_find_process(uint32_t pid);
void scheduler_block_current(void);
void scheduler_unblock_process(pcb_t* process);

// Memory management functions (forward declarations)
void* memory_alloc_pages(uint32_t count);
void memory_free_pages(void* addr, uint32_t count);

// Capability functions (forward declarations)
status_t capability_grant(uint32_t pid, uint32_t cap_type, uint32_t permissions, uint32_t resource_id);
status_t capability_revoke(uint32_t pid, uint32_t cap_type, uint32_t resource_id);

// IPC functions (forward declarations)
status_t ipc_clear_queue(uint32_t pid);

// Scheduler functions (forward declarations)
void scheduler_set_priority(pcb_t* process, uint32_t priority);

// Interrupt handling
void interrupt_init(void);
void timer_interrupt_handler(void);
void keyboard_interrupt_handler(void);
void syscall_dispatch(void* frame);

// Debug functions
void kernel_panic(const char* message);
void kernel_print(const char* str);
void kernel_print_hex(uint32_t value);

#endif // KERNEL_H
