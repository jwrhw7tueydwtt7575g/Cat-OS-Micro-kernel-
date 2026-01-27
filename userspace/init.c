// Init Process (PID 1)
// First user-space process, starts system services

#include "userspace.h"
#include "driver.h"

// Service configuration
typedef struct {
    const char* name;
    uint32_t pid;
    bool critical;
} service_t;

// System services
static service_t services[] = {
    {"keyboard", 2, true},
    {"console", 3, true},
    {"timer", 4, true},
    {"shell", 5, false}
};

static const uint32_t service_count = sizeof(services) / sizeof(service_t);

// Forward declarations
static void start_service(uint32_t service_index);
static void monitor_services(void);
static void handle_service_failure(uint32_t service_index);

// Init main function
int main(void) {
    print("MiniSecureOS Init Process v1.0\r\n");
    print("Starting system services...\r\n");
    
    // Start critical services first
    for (uint32_t i = 0; i < service_count; i++) {
        if (services[i].critical) {
            start_service(i);
        }
    }
    
    // Start non-critical services
    for (uint32_t i = 0; i < service_count; i++) {
        if (!services[i].critical) {
            start_service(i);
        }
    }
    
    print("All services started\r\n");
    print("Init process entering monitor mode\r\n");
    
    // Monitor services and handle failures
    monitor_services();
    
    return 0;
}

// Start a service
static void start_service(uint32_t service_index) {
    if (service_index >= service_count) {
        return;
    }
    
    service_t* service = &services[service_index];
    
    print("Starting service: ");
    print(service->name);
    print(" (PID ");
    print_hex(service->pid);
    print(")\r\n");
    
    // In a real implementation, this would load and execute the service binary
    // For now, we'll just print a message
    print("Service started successfully\r\n");
    
    // Give service time to initialize
    sleep(100);  // 100ms
}

// Monitor services and handle failures
static void monitor_services(void) {
    while (1) {
        // Check for service failures
        for (uint32_t i = 0; i < service_count; i++) {
            // Check if service is still running
            // In a real implementation, this would check process status
            // For now, we'll simulate monitoring
            
            // Wait for service exit messages
            ipc_abi_message_t msg = {0};  // Initialize to zero
            uint32_t result = ipc_receive(0, &msg, false);
            if (result == 0) {  // STATUS_SUCCESS
                if (msg.msg_type == MSG_SIGNAL) {
                    uint32_t exited_pid = *(uint32_t*)msg.data;
                    
                    // Find which service exited
                    for (uint32_t j = 0; j < service_count; j++) {
                        if (services[j].pid == exited_pid) {
                            print("Service ");
                            print(services[j].name);
                            print(" (PID ");
                            print_hex(exited_pid);
                            print(") exited\r\n");
                            
                            handle_service_failure(j);
                            break;
                        }
                    }
                }
            }
        }
        
        // Yield CPU
        process_yield();
        
        // Sleep for monitoring interval
        sleep(1000);  // 1 second
    }
}

// Handle service failure
static void handle_service_failure(uint32_t service_index) {
    if (service_index >= service_count) {
        return;
    }
    
    service_t* service = &services[service_index];
    
    if (service->critical) {
        print("Critical service failed, restarting...\r\n");
        
        // Restart critical service
        start_service(service_index);
    } else {
        print("Non-critical service failed, not restarting\r\n");
    }
}

// Get current PID
uint32_t get_pid(void) {
    // This would be implemented as a system call
    return 1;  // Init process
}

// Sleep function
void sleep(uint32_t ms) {
    // Request timer driver to sleep
    ipc_abi_message_t msg = {0};  // Initialize to zero
    msg.msg_type = DRIVER_MSG_IOCTL;
    msg.data_size = 3 * sizeof(uint32_t);
    uint32_t* data = (uint32_t*)msg.data;
    data[0] = 0x03;  // Set delay request
    data[1] = ms;    // Delay in milliseconds
    data[2] = 0;     // Unused
    
    uint32_t request_id = driver_request(4, &msg);  // Timer driver PID
    
    if (request_id != 0) {
        // Wait for timer notification
        ipc_abi_message_t response = {0};  // Initialize to zero
        while (ipc_receive(0, &response, true) == 0) {  // STATUS_SUCCESS
            if (response.msg_type == DRIVER_MSG_IOCTL && 
                response.data_size >= sizeof(uint32_t) &&
                *(uint32_t*)response.data == request_id) {
                break;
            }
        }
    }
}

// Print function
void print(const char* str) {
    // Send to console driver
    ipc_abi_message_t msg;
    msg.msg_type = DRIVER_MSG_WRITE;
    msg.data_size = strlen(str) + 1;
    memcpy(msg.data, str, msg.data_size);
    ipc_send(3, &msg);  // Console driver PID
}

// Print hex function
void print_hex(uint32_t value) {
    const char hex_chars[] = "0123456789ABCDEF";
    char buffer[11] = "0x00000000";
    
    for (int i = 0; i < 8; i++) {
        buffer[9 - i] = hex_chars[value & 0xF];
        value >>= 4;
    }
    
    print(buffer);
}

// String functions
uint32_t strlen(const char* str) {
    uint32_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* orig_dest = dest;
    while ((*dest++ = *src++));
    return orig_dest;
}

char* strcat(char* dest, const char* src) {
    char* orig_dest = dest;
    while (*dest) dest++;
    while ((*dest++ = *src++));
    return orig_dest;
}

int strcmp(const char* str1, const char* str2) {
    while (*str1 && *str2 && *str1 == *str2) {
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

void* memset(void* ptr, int value, uint32_t size) {
    uint8_t* p = (uint8_t*)ptr;
    for (uint32_t i = 0; i < size; i++) {
        p[i] = value;
    }
    return ptr;
}

void* memcpy(void* dest, const void* src, uint32_t size) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (uint32_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
    return dest;
}
