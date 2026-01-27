// User-space system call wrappers and utilities
#ifndef USERSPACE_H
#define USERSPACE_H

#include <stdint.h>
#include <stddef.h>
#include "syscall_numbers.h"
#include "ipc_abi.h"
#include "types.h"

// System call interface
static inline uint32_t syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    uint32_t result;
    __asm__ volatile(
        "int $0x80"
        : "=a"(result)
        : "a"(eax), "b"(ebx), "c"(ecx), "d"(edx)
    );
    return result;
}

// Process management
static inline uint32_t process_create(void) {
    return syscall(SYS_PROCESS_CREATE, 0, 0, 0);
}

static inline void process_exit(uint32_t exit_code) {
    syscall(SYS_PROCESS_EXIT, exit_code, 0, 0);
}

static inline void process_yield(void) {
    syscall(SYS_PROCESS_YIELD, 0, 0, 0);
}

static inline uint32_t process_kill(uint32_t pid) {
    return syscall(SYS_PROCESS_KILL, pid, 0, 0);
}

// Memory management
static inline void* memory_alloc(uint32_t size) {
    return (void*)syscall(SYS_MEMORY_ALLOC, size, 0, 0);
}

static inline void memory_free(void* ptr) {
    syscall(SYS_MEMORY_FREE, (uint32_t)ptr, 0, 0);
}

static inline uint32_t memory_map(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) {
    return syscall(SYS_MEMORY_MAP, virt_addr, phys_addr, flags);
}

// IPC
static inline uint32_t ipc_send(uint32_t receiver_pid, ipc_abi_message_t* msg) {
    return syscall(SYS_IPC_SEND, receiver_pid, (uint32_t)msg, 0);
}

static inline uint32_t ipc_receive(uint32_t sender_pid, ipc_abi_message_t* msg, bool block) {
    return syscall(SYS_IPC_RECEIVE, sender_pid, (uint32_t)msg, block);
}

static inline uint32_t ipc_register_handler(uint32_t msg_type, void (*handler)(ipc_abi_message_t*)) {
    return syscall(SYS_IPC_REGISTER, msg_type, (uint32_t)handler, 0);
}

// Driver operations
static inline uint32_t driver_register_wrapper(const char* name, uint32_t capabilities) {
    return syscall(SYS_DRIVER_REGISTER, (uint32_t)name, capabilities, 0);
}

static inline uint32_t driver_request(uint32_t driver_pid, ipc_abi_message_t* request) {
    return syscall(SYS_DRIVER_REQUEST, driver_pid, (uint32_t)request, 0);
}

// Driver utility functions
static inline uint32_t driver_get_ticks(void) {
    ipc_abi_message_t msg = {0};
    msg.msg_type = DRIVER_MSG_READ;
    msg.data_size = 0;
    
    uint32_t result = driver_request(4, &msg);  // Timer driver PID
    if (result == 0) {
        ipc_abi_message_t response = {0};
        if (ipc_receive(4, &response, false) == 0) {  // STATUS_SUCCESS
            if (response.data_size >= sizeof(uint32_t)) {
                return *(uint32_t*)response.data;
            }
        }
    }
    return 0;
}

// System operations
static inline void system_shutdown(void) {
    syscall(SYS_SYSTEM_SHUTDOWN, 0, 0, 0);
}

// Basic utility functions
uint32_t strlen(const char* str);
char* strcpy(char* dest, const char* src);
char* strcat(char* dest, const char* src);
int strcmp(const char* str1, const char* str2);
void* memset(void* ptr, int value, uint32_t size);
void* memcpy(void* dest, const void* src, uint32_t size);

// Print functions
void print(const char* str);
void print_hex(uint32_t value);
void println(const char* str);

// Process utilities
uint32_t get_pid(void);
uint32_t get_parent_pid(void);
void sleep(uint32_t ms);

// Driver utilities
void driver_print(const char* str);
void driver_print_hex(uint32_t value);
uint32_t driver_get_ticks(void);

#endif // USERSPACE_H
