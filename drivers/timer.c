// Timer Driver
// User-space timer driver

#include "driver.h"
#include "kernel.h"
#include "hal.h"
#include <stddef.h>

// Forward declarations
static uint32_t timer_add_request(uint32_t delay_ticks, uint32_t target_pid);
static void timer_remove_request(uint32_t request_id);
static void timer_check_requests(void);

// Timer driver interface
static driver_interface_t timer_driver = {
    .name = "timer",
    .driver_id = 4,
    .capabilities = CAP_DRIVER_READ,
    .init = timer_driver_init,
    .cleanup = timer_driver_init,  // Use same function for now
    .shutdown = NULL,  // No special shutdown needed
    .handle_message = timer_driver_handle_message
};

// Timer state
static bool timer_initialized = false;
static uint32_t timer_pid = 0;
static uint32_t timer_ticks = 0;
static uint32_t timer_frequency = 100;

// Timer request structure
typedef struct {
    uint32_t request_id;
    uint32_t target_pid;
    uint32_t target_ticks;
    bool active;
} timer_request_t;

// Timer requests
static timer_request_t timer_requests[32];
static uint32_t next_request_id = 1;

// Stub functions (drivers can't directly call kernel IPC)
static status_t ipc_send_stub(uint32_t receiver_pid, ipc_abi_message_t* msg) {
    (void)receiver_pid;
    (void)msg;
    return STATUS_SUCCESS;
}

// Driver print function (writes to VGA)
void driver_print(const char* str) {
    // Simple VGA output for debugging
    volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
    static uint32_t pos = 0;
    
    while (*str) {
        if (*str == '\n') {
            pos = (pos / 80 + 1) * 80;
        } else if (*str == '\r') {
            pos = (pos / 80) * 80;
        } else if (*str == '\b') {
            if (pos > 0) pos--;
            vga[pos] = 0x0700 | ' ';
        } else if (*str == '\t') {
            pos = (pos + 8) & ~7;
        } else {
            vga[pos++] = 0x0700 | *str;
        }
        str++;
        
        // Simple screen wrapping
        if (pos >= 80 * 25) {
            pos = 0;
        }
    }
}

// Driver print hex function
void driver_print_hex(uint32_t value) {
    const char hex_chars[] = "0123456789ABCDEF";
    char buffer[11] = "0x00000000";
    
    for (int i = 9; i >= 2; i--) {
        buffer[i] = hex_chars[value & 0x0F];
        value >>= 4;
    }
    
    driver_print(buffer);
}

// Initialize timer driver
status_t timer_driver_init(void) {
    if (timer_initialized) {
        return STATUS_SUCCESS;
    }
    
    // Clear timer requests
    for (int i = 0; i < 32; i++) {
        timer_requests[i].active = false;
    }
    
    // Get driver PID
    timer_pid = 4;  // Fixed PID for timer driver
    
    // Register with kernel
    status_t result = driver_register(&timer_driver);
    if (result != STATUS_SUCCESS) {
        return result;
    }
    
    timer_initialized = true;
    driver_print("Timer driver initialized\r\n");
    
    return STATUS_SUCCESS;
}

// Shutdown timer driver
void timer_driver_shutdown(void) {
    if (!timer_initialized) {
        return;
    }
    
    // Clear all timer requests
    for (int i = 0; i < 32; i++) {
        timer_requests[i].active = false;
    }
    
    // Unregister from kernel
    driver_unregister(timer_driver.driver_id);
    
    timer_initialized = false;
}

// Handle driver messages
status_t timer_driver_handle_message(ipc_abi_message_t* msg) {
    if (!msg || !timer_initialized) {
        return STATUS_INVALID_PARAM;
    }
    
    switch (msg->msg_type) {
        case MSG_DRIVER:
            // Timer tick notification from kernel
            timer_ticks++;
            timer_check_requests();
            break;
            
        case DRIVER_MSG_IOCTL:
            // Handle timer delay request
            if (msg->data_size >= 3 * sizeof(uint32_t)) {
                uint32_t* data = (uint32_t*)msg->data;
                uint32_t request_type = data[0];
                uint32_t delay_ms = data[1];
                
                if (request_type == 0x03) {  // Delay request
                    uint32_t delay_ticks = (delay_ms * timer_frequency) / 1000;
                    uint32_t actual_id = timer_add_request(delay_ticks, msg->sender_pid);
                    
                    // Send response with actual request ID
                    ipc_abi_message_t response = {0};
                    response.msg_type = DRIVER_MSG_IOCTL;
                    response.data_size = sizeof(uint32_t);
                    *(uint32_t*)response.data = actual_id;
                    ipc_send_stub(msg->sender_pid, &response);
                }
            }
            break;
            
        case DRIVER_MSG_READ:
            // Handle timer tick count request
            {
                ipc_abi_message_t response = {0};
                response.msg_type = DRIVER_MSG_READ;
                response.data_size = sizeof(uint32_t);
                *(uint32_t*)response.data = timer_ticks;
                ipc_send_stub(msg->sender_pid, &response);
            }
            break;
            
        default:
            return STATUS_INVALID_PARAM;
    }
    
    return STATUS_SUCCESS;
}

// Add timer request
static uint32_t timer_add_request(uint32_t delay_ticks, uint32_t target_pid) {
    for (uint32_t i = 0; i < 32; i++) {
        if (!timer_requests[i].active) {
            timer_requests[i].request_id = next_request_id++;
            timer_requests[i].target_pid = target_pid;
            timer_requests[i].target_ticks = timer_ticks + delay_ticks;
            timer_requests[i].active = true;
            return next_request_id - 1;
        }
    }
    
    return 0;  // No free slots
}

// Remove timer request
static void timer_remove_request(uint32_t request_id) {
    for (int i = 0; i < 32; i++) {
        if (timer_requests[i].active && timer_requests[i].request_id == request_id) {
            timer_requests[i].active = false;
            
            driver_print("Timer request removed: ID ");
            driver_print_hex(request_id);
            driver_print("\r\n");
            
            return;
        }
    }
}

// Check for expired timer requests
static void timer_check_requests(void) {
    for (uint32_t i = 0; i < 32; i++) {
        if (timer_requests[i].active && timer_requests[i].target_ticks <= timer_ticks) {
            // Timer expired
            timer_requests[i].active = false;
            
            // Send notification to target process
            ipc_abi_message_t notification = {0};
            notification.msg_type = MSG_SIGNAL;
            notification.data_size = sizeof(uint32_t);
            *(uint32_t*)notification.data = timer_requests[i].request_id;
            
            ipc_send_stub(timer_requests[i].target_pid, &notification);
            
            driver_print("Timer expired: ID ");
            driver_print_hex(timer_requests[i].request_id);
            driver_print("\r\n");
            
            // Remove request
            timer_remove_request(timer_requests[i].request_id);
        }
    }
}

// Get current tick count
uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

// Get timer frequency
uint32_t timer_get_frequency(void) {
    return timer_frequency;
}

// Convert milliseconds to ticks
uint32_t timer_ms_to_ticks(uint32_t ms) {
    return (ms * timer_frequency) / 1000;
}

// Convert ticks to milliseconds
uint32_t timer_ticks_to_ms(uint32_t ticks) {
    return (ticks * 1000) / timer_frequency;
}

// Get system uptime in milliseconds
uint32_t timer_get_uptime_ms(void) {
    return timer_ticks_to_ms(timer_ticks);
}

// Get system uptime in seconds
uint32_t timer_get_uptime_seconds(void) {
    return timer_ticks / timer_frequency;
}

// Driver main function
int main(void) {
    // Initialize timer driver
    status_t result = timer_driver_init();
    if (result != STATUS_SUCCESS) {
        return -1;
    }
    
    // Simple driver loop
    while (timer_initialized) {
        // In a real implementation, this would handle timer interrupts
        // For now, just halt
        __asm__ volatile("hlt");
    }
    
    return 0;
}
