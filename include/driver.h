// Driver interface and common definitions
#ifndef DRIVER_H
#define DRIVER_H

#include <stdint.h>
#include "ipc_abi.h"
#include "types.h"

// Driver interface structure
typedef struct {
    const char* name;
    uint32_t driver_id;
    uint32_t capabilities;
    status_t (*init)(void);
    status_t (*cleanup)(void);
    void (*shutdown)(void);
    status_t (*handle_message)(ipc_abi_message_t* msg);
} driver_interface_t;

// Driver management functions
status_t driver_register(driver_interface_t* driver);
status_t driver_unregister(uint32_t driver_id);
status_t driver_find(const char* name, uint32_t* driver_id);
status_t driver_send_message(uint32_t driver_id, ipc_abi_message_t* msg);
status_t driver_broadcast_message(ipc_abi_message_t* msg);

// Common driver message types
#define DRIVER_MSG_READ    0x01
#define DRIVER_MSG_WRITE   0x02
#define DRIVER_MSG_IOCTL   0x03

// Driver capabilities
#define CAP_DRIVER_READ    0x01
#define CAP_DRIVER_WRITE   0x02
#define CAP_DRIVER_IOCTL   0x04

// Specific driver interfaces
status_t keyboard_driver_handle_message(ipc_abi_message_t* msg);
status_t console_driver_handle_message(ipc_abi_message_t* msg);
status_t timer_driver_handle_message(ipc_abi_message_t* msg);

// Driver utility functions
status_t driver_send_response(uint32_t sender_pid, ipc_abi_message_t* original_msg, const void* data, uint32_t data_size);

// Driver utilities
void driver_print(const char* str);
void driver_print_hex(uint32_t value);
uint32_t driver_get_ticks(void);

// Keyboard driver
status_t keyboard_driver_init(void);
void keyboard_driver_shutdown(void);

// Console driver
status_t console_driver_init(void);
void console_driver_shutdown(void);

// Timer driver
status_t timer_driver_init(void);
void timer_driver_shutdown(void);

#endif // DRIVER_H
