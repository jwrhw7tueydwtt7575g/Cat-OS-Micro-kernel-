# MiniSecureOS API Reference

## System Calls

### Process Management

#### `process_create()`
```c
uint32_t process_create(void);
```
Creates a new process.
- **Returns**: PID of new process, or error code
- **Capabilities Required**: CAP_PROCESS with PERM_CREATE

#### `process_exit(uint32_t exit_code)`
```c
void process_exit(uint32_t exit_code);
```
Terminates the current process.
- **Parameters**: 
  - `exit_code`: Process exit status
- **Capabilities Required**: None (always allowed)

#### `process_yield()`
```c
void process_yield(void);
```
Yields CPU to next process.
- **Capabilities Required**: None (always allowed)

#### `process_kill(uint32_t pid)`
```c
uint32_t process_kill(uint32_t pid);
```
Terminates specified process.
- **Parameters**: 
  - `pid`: Process ID to kill
- **Returns**: STATUS_SUCCESS or error code
- **Capabilities Required**: CAP_PROCESS with PERM_DELETE or CAP_SYSTEM

### Memory Management

#### `memory_alloc(uint32_t size)`
```c
void* memory_alloc(uint32_t size);
```
Allocates memory for current process.
- **Parameters**: 
  - `size`: Size in bytes to allocate
- **Returns**: Pointer to allocated memory, or NULL on failure
- **Capabilities Required**: CAP_MEMORY with PERM_ALLOC

#### `memory_free(void* ptr)`
```c
void memory_free(void* ptr);
```
Frees previously allocated memory.
- **Parameters**: 
  - `ptr`: Pointer to memory to free
- **Capabilities Required**: CAP_MEMORY with PERM_FREE

#### `memory_map(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags)`
```c
uint32_t memory_map(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags);
```
Maps virtual address to physical address.
- **Parameters**: 
  - `virt_addr`: Virtual address
  - `phys_addr`: Physical address
  - `flags`: Page protection flags
- **Returns**: STATUS_SUCCESS or error code
- **Capabilities Required**: CAP_MEMORY with PERM_WRITE

### Inter-Process Communication

#### `ipc_send(uint32_t receiver_pid, ipc_message_t* msg)`
```c
uint32_t ipc_send(uint32_t receiver_pid, ipc_message_t* msg);
```
Sends message to another process.
- **Parameters**: 
  - `receiver_pid`: PID of receiver process
  - `msg`: Message to send
- **Returns**: STATUS_SUCCESS or error code
- **Capabilities Required**: CAP_IPC with PERM_WRITE

#### `ipc_receive(uint32_t sender_pid, ipc_message_t* msg, bool block)`
```c
uint32_t ipc_receive(uint32_t sender_pid, ipc_message_t* msg, bool block);
```
Receives message from another process.
- **Parameters**: 
  - `sender_pid`: PID of sender process (0 = any)
  - `msg`: Message buffer to fill
  - `block`: Whether to block waiting for message
- **Returns**: STATUS_SUCCESS or error code
- **Capabilities Required**: CAP_IPC with PERM_READ

#### `ipc_register_handler(uint32_t msg_type, void (*handler)(ipc_message_t*))`
```c
uint32_t ipc_register_handler(uint32_t msg_type, void (*handler)(ipc_message_t*));
```
Registers message handler for specific message type.
- **Parameters**: 
  - `msg_type`: Message type to handle
  - `handler`: Handler function
- **Returns**: STATUS_SUCCESS or error code
- **Capabilities Required**: CAP_IPC with PERM_WRITE

### Driver Operations

#### `driver_register(const char* name, uint32_t capabilities)`
```c
uint32_t driver_register(const char* name, uint32_t capabilities);
```
Registers current process as a driver.
- **Parameters**: 
  - `name`: Driver name
  - `capabilities`: Driver capabilities bitmask
- **Returns**: STATUS_SUCCESS or error code
- **Capabilities Required**: CAP_DRIVER with PERM_CREATE

#### `driver_request(uint32_t driver_pid, ipc_message_t* request)`
```c
uint32_t driver_request(uint32_t driver_pid, ipc_message_t* request);
```
Sends request to specific driver.
- **Parameters**: 
  - `driver_pid`: PID of driver process
  - `request`: Request message
- **Returns**: STATUS_SUCCESS or error code
- **Capabilities Required**: Varies by driver

### System Operations

#### `system_shutdown()`
```c
void system_shutdown(void);
```
Shuts down the system.
- **Capabilities Required**: CAP_SYSTEM with PERM_DELETE

## Data Structures

### Process Control Block (PCB)
```c
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
} pcb_t;
```

### IPC Message
```c
typedef struct {
    uint32_t msg_id;           // Unique message identifier
    uint32_t sender_pid;       // Sender process ID
    uint32_t receiver_pid;     // Receiver process ID
    uint32_t msg_type;         // Message type identifier
    uint32_t flags;            // Message flags (sync/async)
    uint32_t timestamp;        // Message timestamp
    uint32_t data_size;        // Size of message data
    uint8_t data[];            // Message payload
} ipc_message_t;
```

### Capability
```c
typedef struct {
    uint32_t cap_id;          // Unique capability identifier
    uint32_t owner_pid;       // Process that owns capability
    uint32_t cap_type;        // Type of capability
    uint32_t permissions;     // Permission bitmask
    uint32_t resource_id;      // Resource this capability controls
    uint32_t expiration_time;  // Optional expiration time
    uint8_t signature[16];    // Cryptographic signature
} capability_t;
```

## Constants and Enums

### Process States
```c
typedef enum {
    PROCESS_CREATED = 0,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;
```

### Message Types
```c
typedef enum {
    MSG_DATA = 0,
    MSG_CONTROL,
    MSG_SIGNAL,
    MSG_RESPONSE,
    MSG_DRIVER
} message_type_t;
```

### Capability Types
```c
typedef enum {
    CAP_PROCESS = 0,
    CAP_MEMORY,
    CAP_DRIVER,
    CAP_HARDWARE,
    CAP_SYSTEM,
    CAP_IPC
} capability_type_t;
```

### Status Codes
```c
typedef enum {
    STATUS_SUCCESS = 0,
    STATUS_ERROR = -1,
    STATUS_INVALID_PARAM = -2,
    STATUS_OUT_OF_MEMORY = -3,
    STATUS_PERMISSION_DENIED = -4,
    STATUS_NOT_FOUND = -5,
    STATUS_TIMEOUT = -6
} status_t;
```

### System Call Numbers
```c
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
```

### Permission Bits
```c
#define PERM_READ       0x01
#define PERM_WRITE      0x02
#define PERM_EXECUTE    0x04
#define PERM_CREATE     0x08
#define PERM_DELETE     0x10
#define PERM_TRANSFER   0x20
```

## Driver APIs

### Keyboard Driver (PID 2)
#### Message Types
- `DRIVER_MSG_READ`: Read character from keyboard
- `DRIVER_MSG_IOCTL`: Control operations

#### IOCTL Commands
- None currently implemented

### Console Driver (PID 3)
#### Message Types
- `DRIVER_MSG_WRITE`: Write text to console
- `DRIVER_MSG_IOCTL`: Control operations

#### IOCTL Commands
- `0x01`: Clear screen
- `0x02`: Set color (parameters: fg_color, bg_color)
- `0x03`: Set cursor position (parameters: x, y)

### Timer Driver (PID 4)
#### Message Types
- `DRIVER_MSG_READ`: Get current tick count
- `DRIVER_MSG_IOCTL`: Control operations

#### IOCTL Commands
- `0x01`: Get tick count
- `0x02`: Get timer frequency
- `0x03`: Set delay request (parameters: delay_ms)
- `0x04`: Cancel delay request (parameters: request_id)

## HAL APIs

### CPU Control
```c
void hal_cpu_init(void);
uint32_t hal_cpu_get_features(void);
void hal_cpu_enable_paging(uint32_t page_dir);
void hal_cpu_set_cr3(uint32_t page_dir);
void hal_cpu_halt(void);
uint64_t hal_cpu_get_cycles(void);
```

### I/O Ports
```c
void hal_outb(uint16_t port, uint8_t value);
uint8_t hal_inb(uint16_t port);
void hal_outw(uint16_t port, uint16_t value);
uint16_t hal_inw(uint16_t port);
```

### Timer
```c
void hal_timer_init(uint32_t frequency);
void hal_timer_set_frequency(uint32_t hz);
uint32_t hal_timer_get_ticks(void);
void hal_timer_delay_ms(uint32_t ms);
void hal_timer_enable_irq(void);
```

### PIC
```c
void hal_pic_init(void);
void hal_pic_mask_irq(uint8_t irq);
void hal_pic_unmask_irq(uint8_t irq);
void hal_pic_send_eoi(uint8_t irq);
void hal_pic_remap(uint8_t offset1, uint8_t offset2);
```

## Error Handling

### Return Values
- **Success**: STATUS_SUCCESS (0)
- **Error**: Negative status codes
- **PID**: Positive values for process creation

### Common Errors
- `STATUS_INVALID_PARAM`: Invalid parameter passed
- `STATUS_OUT_OF_MEMORY`: Insufficient memory
- `STATUS_PERMISSION_DENIED`: Insufficient capabilities
- `STATUS_NOT_FOUND`: Resource not found
- `STATUS_TIMEOUT`: Operation timed out

### Error Recovery
- Check return values for all system calls
- Handle errors gracefully
- Clean up resources on failure
- Log errors for debugging

## Usage Examples

### Process Creation
```c
uint32_t child_pid = process_create();
if (child_pid > 0) {
    // Parent process
    print("Child PID: ");
    print_hex(child_pid);
    print("\n");
} else if (child_pid == 0) {
    // Child process
    print("Child process running\n");
    process_exit(0);
} else {
    // Error
    print("Failed to create process\n");
}
```

### IPC Communication
```c
// Send message
ipc_message_t msg;
msg.msg_type = MSG_DATA;
msg.data_size = sizeof(uint32_t);
*(uint32_t*)msg.data = 0x12345678;

uint32_t result = ipc_send(target_pid, &msg);
if (result == STATUS_SUCCESS) {
    print("Message sent\n");
}

// Receive message
ipc_message_t received_msg;
result = ipc_receive(0, &received_msg, true);
if (result == STATUS_SUCCESS) {
    print("Message received\n");
}
```

### Memory Allocation
```c
void* buffer = memory_alloc(1024);
if (buffer) {
    // Use buffer
    memset(buffer, 0, 1024);
    
    // Free when done
    memory_free(buffer);
} else {
    print("Memory allocation failed\n");
}
```

### Driver Communication
```c
// Send text to console
ipc_message_t msg;
msg.msg_type = DRIVER_MSG_WRITE;
msg.data_size = strlen("Hello, World!") + 1;
strcpy(msg.data, "Hello, World!");

uint32_t result = driver_request(3, &msg);  // Console driver
if (result == STATUS_SUCCESS) {
    print("Text sent to console\n");
}
```
