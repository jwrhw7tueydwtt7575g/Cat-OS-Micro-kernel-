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

// Monitoring and failure handling logic remains the same
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

