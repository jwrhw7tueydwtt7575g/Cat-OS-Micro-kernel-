#ifndef SYSCALL_NUMBERS_H
#define SYSCALL_NUMBERS_H

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
#define SYS_DEBUG_PRINT       0x41

#endif // SYSCALL_NUMBERS_H
