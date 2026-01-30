// Timer Driver
// User-space timer driver

#include "driver.h"
#include "userspace.h"
#include <stddef.h>

// Timer state
static bool timer_initialized = false;
static uint32_t timer_ticks = 0;
static uint32_t timer_frequency = 100;

// Timer request structure
typedef struct {
    uint32_t request_id;
    uint32_t target_pid;
    uint32_t target_ticks;
    bool active;
} timer_request_t;

static timer_request_t timer_requests[32];
static uint32_t next_request_id = 1;

// Forward declarations
status_t timer_driver_handle_message(ipc_abi_message_t* msg);
static void timer_check_requests(void);

// Timer driver interface
static driver_interface_t timer_driver = {
    .name = "timer",
    .driver_id = 4,
    .capabilities = CAP_DRIVER_READ | CAP_DRIVER_IOCTL,
    .init = timer_driver_init,
    .cleanup = timer_driver_shutdown,
    .shutdown = NULL,
    .handle_message = timer_driver_handle_message
};

status_t timer_driver_init(void) {
    if (timer_initialized) return STATUS_SUCCESS;
    for (int i = 0; i < 32; i++) timer_requests[i].active = false;
    driver_register(&timer_driver);
    driver_register_wrapper(timer_driver.name, timer_driver.capabilities);
    timer_initialized = true;
    return STATUS_SUCCESS;
}

status_t timer_driver_shutdown(void) {
    if (!timer_initialized) return STATUS_SUCCESS;
    driver_unregister(timer_driver.driver_id);
    timer_initialized = false;
    return STATUS_SUCCESS;
}

status_t timer_driver_handle_message(ipc_abi_message_t* msg) {
    if (!msg || !timer_initialized) return STATUS_INVALID_PARAM;
    
    switch (msg->msg_type) {
        case MSG_DRIVER: // Tick from kernel
            timer_ticks++;
            timer_check_requests();
            break;
            
        case DRIVER_MSG_IOCTL:
            if (msg->data_size >= 2 * sizeof(uint32_t)) {
                uint32_t* data = (uint32_t*)msg->data;
                if (data[0] == 0x03) { // Delay request
                    uint32_t delay_ms = data[1];
                    uint32_t delay_ticks = (delay_ms * timer_frequency) / 1000;
                    
                    // Add request
                    uint32_t id = 0;
                    for (int i = 0; i < 32; i++) {
                        if (!timer_requests[i].active) {
                            timer_requests[i].request_id = next_request_id++;
                            timer_requests[i].target_pid = msg->sender_pid;
                            timer_requests[i].target_ticks = timer_ticks + delay_ticks;
                            timer_requests[i].active = true;
                            id = timer_requests[i].request_id;
                            break;
                        }
                    }
                    
                    ipc_abi_message_t response = {0};
                    response.msg_type = DRIVER_MSG_IOCTL;
                    response.data_size = sizeof(uint32_t);
                    *(uint32_t*)response.data = id;
                    ipc_send(msg->sender_pid, &response);
                }
            }
            break;
            
        case DRIVER_MSG_READ:
            {
                ipc_abi_message_t response = {0};
                response.msg_type = DRIVER_MSG_READ;
                response.data_size = sizeof(uint32_t);
                *(uint32_t*)response.data = timer_ticks;
                ipc_send(msg->sender_pid, &response);
            }
            break;
            
        default: return STATUS_INVALID_PARAM;
    }
    return STATUS_SUCCESS;
}

static void timer_check_requests(void) {
    for (int i = 0; i < 32; i++) {
        if (timer_requests[i].active && timer_requests[i].target_ticks <= timer_ticks) {
            timer_requests[i].active = false;
            ipc_abi_message_t notification = {0};
            notification.msg_type = DRIVER_MSG_IOCTL; 
            notification.data_size = sizeof(uint32_t);
            *(uint32_t*)notification.data = timer_requests[i].request_id;
            ipc_send(timer_requests[i].target_pid, &notification);
        }
    }
}

void driver_print(const char* str) { print(str); }

int main(void);
void _start(void) __attribute__((section(".text.entry")));
void _start(void) {
    main();
    while(1) { process_yield(); }
}

int main(void) {
    if (timer_driver_init() != STATUS_SUCCESS) return 1;
    uint8_t buffer[sizeof(ipc_abi_message_t) + 128];
    ipc_abi_message_t* msg_ptr = (ipc_abi_message_t*)buffer;
    while (1) {
        if (ipc_receive(0, msg_ptr, true) == STATUS_SUCCESS) {
            timer_driver_handle_message(msg_ptr);
        }
    }
    return 0;
}
