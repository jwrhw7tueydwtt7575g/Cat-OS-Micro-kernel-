// User-space utility functions
// Common functions used by user-space programs

#include "userspace.h"

// String functions
uint32_t strlen(const char* str) {
    uint32_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++)) {
        // Copy characters
    }
    return dest;
}

char* strcat(char* dest, const char* src) {
    char* d = dest;
    while (*d) {
        d++;
    }
    while ((*d++ = *src++)) {
        // Append characters
    }
    return dest;
}

int strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

void* memset(void* ptr, int value, uint32_t size) {
    uint8_t* p = (uint8_t*)ptr;
    while (size--) {
        *p++ = (uint8_t)value;
    }
    return ptr;
}

void* memcpy(void* dest, const void* src, uint32_t size) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (size--) {
        *d++ = *s++;
    }
    return dest;
}

// Print functions
void print(const char* str) {
    // Send to console driver
    ipc_abi_message_t msg = {0};
    msg.msg_type = DRIVER_MSG_WRITE;
    msg.data_size = strlen(str) + 1;
    memcpy(msg.data, str, msg.data_size);
    ipc_send(3, &msg);  // Console driver PID
}

void print_hex(uint32_t value) {
    const char hex_chars[] = "0123456789ABCDEF";
    char buffer[11] = "0x00000000";
    
    for (int i = 9; i >= 2; i--) {
        buffer[i] = hex_chars[value & 0x0F];
        value >>= 4;
    }
    
    print(buffer);
}

// Sleep function
void sleep(uint32_t ms) {
    // Request timer driver to sleep
    ipc_abi_message_t msg = {0};
    msg.msg_type = DRIVER_MSG_IOCTL;
    msg.data_size = 3 * sizeof(uint32_t);
    uint32_t* data = (uint32_t*)msg.data;
    data[0] = 0x03;  // Set delay request
    data[1] = ms;    // Delay in milliseconds
    data[2] = 0;     // Unused
    
    uint32_t request_id = driver_request(4, &msg);  // Timer driver PID
    
    if (request_id != 0) {
        // Wait for timer notification
        ipc_abi_message_t response = {0};
        while (ipc_receive(0, &response, true) == 0) {  // STATUS_SUCCESS
            if (response.msg_type == DRIVER_MSG_IOCTL && 
                response.data_size >= sizeof(uint32_t) &&
                *(uint32_t*)response.data == request_id) {
                break;
            }
        }
    }
}
