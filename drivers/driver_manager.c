// Driver Manager
// Manages driver registration and communication

#include "driver.h"
#include "kernel.h"
#include "hal.h"
#include <stddef.h>

// Constants
#define MAX_DRIVERS 16

// Simple string functions
static int strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

// Stub for ipc_send (drivers can't directly call kernel IPC)
static status_t ipc_send_stub(uint32_t receiver_pid, ipc_abi_message_t* msg) {
    (void)receiver_pid;
    (void)msg;
    return STATUS_SUCCESS;  // Stub implementation
}

// Registered drivers
static driver_interface_t* registered_drivers[MAX_DRIVERS];
static uint32_t driver_count = 0;

// Initialize driver manager
void driver_manager_init(void) {
    for (int i = 0; i < MAX_DRIVERS; i++) {
        registered_drivers[i] = NULL;
    }
}

// Register a driver
status_t driver_register(driver_interface_t* driver) {
    if (!driver) {
        return STATUS_INVALID_PARAM;
    }
    
    // Check if driver already registered
    for (int i = 0; i < MAX_DRIVERS; i++) {
        if (registered_drivers[i] && registered_drivers[i]->driver_id == driver->driver_id) {
            return STATUS_ALREADY_EXISTS;
        }
    }
    
    // Find empty slot
    for (int i = 0; i < MAX_DRIVERS; i++) {
        if (!registered_drivers[i]) {
            registered_drivers[i] = driver;
            driver_count++;
            
            // Grant driver capabilities
            // capability_grant(driver->driver_id, CAP_DRIVER, driver->capabilities, 0);
            
            return STATUS_SUCCESS;
        }
    }
    
    return STATUS_OUT_OF_MEMORY;
}

// Unregister a driver
status_t driver_unregister(uint32_t driver_id) {
    for (int i = 0; i < MAX_DRIVERS; i++) {
        if (registered_drivers[i] && registered_drivers[i]->driver_id == driver_id) {
            registered_drivers[i] = NULL;
            driver_count--;
            return STATUS_SUCCESS;
        }
    }
    
    return STATUS_NOT_FOUND;
}

// Find driver by name
status_t driver_find(const char* name, uint32_t* driver_id) {
    if (!name || !driver_id) {
        return STATUS_INVALID_PARAM;
    }
    
    for (int i = 0; i < MAX_DRIVERS; i++) {
        if (registered_drivers[i] && strcmp(registered_drivers[i]->name, name) == 0) {
            *driver_id = registered_drivers[i]->driver_id;
            return STATUS_SUCCESS;
        }
    }
    
    return STATUS_NOT_FOUND;
}

// Send message to driver
status_t driver_send_message(uint32_t driver_id, ipc_abi_message_t* msg) {
    if (!msg) {
        return STATUS_INVALID_PARAM;
    }
    
    // Convert driver_id to array index (driver_id - 1)
    uint32_t index = driver_id - 1;
    if (index >= MAX_DRIVERS || !registered_drivers[index]) {
        return STATUS_NOT_FOUND;
    }
    
    // Send message to driver process
    return ipc_send_stub(driver_id, msg);
}

// Broadcast message to all drivers
status_t driver_broadcast_message(ipc_abi_message_t* msg) {
    if (!msg) {
        return STATUS_INVALID_PARAM;
    }
    
    uint32_t sent_count = 0;
    
    for (int i = 0; i < MAX_DRIVERS; i++) {
        if (registered_drivers[i]) {
            if (ipc_send_stub(registered_drivers[i]->driver_id, msg) == STATUS_SUCCESS) {
                sent_count++;
            }
        }
    }
    
    return (sent_count > 0) ? STATUS_SUCCESS : STATUS_ERROR;
}

// Shutdown all drivers
void driver_shutdown_all(void) {
    for (int i = MAX_DRIVERS; i > 0; i--) {
        if (registered_drivers[i - 1]) {
            if (registered_drivers[i - 1]->shutdown) {
                registered_drivers[i - 1]->shutdown();
            }
            registered_drivers[i - 1] = NULL;
        }
    }
    driver_count = 0;
}

// Get driver list
status_t driver_list(driver_interface_t* drivers, uint32_t* count) {
    if (!count) {
        return STATUS_INVALID_PARAM;
    }
    
    uint32_t available = driver_count;
    
    if (drivers && *count >= available) {
        for (uint32_t i = 0; i < available; i++) {
            if (registered_drivers[i]) {
                drivers[i] = *registered_drivers[i];
            }
        }
    }
    
    *count = available;
    return STATUS_SUCCESS;
}

// Get driver statistics
status_t driver_get_stats(uint32_t* total_drivers, uint32_t* active_drivers) {
    if (!total_drivers) {
        return STATUS_INVALID_PARAM;
    }
    
    if (total_drivers) *total_drivers = driver_count;
    if (active_drivers) *active_drivers = driver_count;
    
    return STATUS_SUCCESS;
}
