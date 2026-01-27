// HAL// I/O Port Access
// Provides safe access to I/O ports with permission checking

#include "hal.h"
#include <stddef.h>

// Forward declaration
void hal_io_grant_port_range(uint16_t start_port, uint16_t count);
#include "types.h"

// Port access permissions (bitmask)
static uint32_t port_permissions[0x10000 / 32];  // 65536 ports, 32 bits per entry

// Initialize I/O port module
void hal_io_init(void) {
    // Clear all permissions initially
    for (int i = 0; i < (0x10000 / 32); i++) {
        port_permissions[i] = 0;
    }
    
    // Grant kernel access to common ports
    hal_io_grant_port_range(PORT_PIC_MASTER_CMD, 2);   // PIC master
    hal_io_grant_port_range(PORT_PIC_SLAVE_CMD, 2);    // PIC slave
    hal_io_grant_port_range(PORT_TIMER_DATA, 2);       // Timer
    hal_io_grant_port_range(PORT_KEYBOARD_DATA, 2);     // Keyboard
}

// Grant access to port range
void hal_io_grant_port_range(uint16_t start_port, uint16_t count) {
    for (uint16_t port = start_port; port < start_port + count; port++) {
        uint32_t index = port / 32;
        uint32_t bit = port % 32;
        port_permissions[index] |= (1 << bit);
    }
}

// Revoke access to port range
void hal_io_revoke_port_range(uint16_t start_port, uint16_t count) {
    for (uint16_t port = start_port; port < start_port + count; port++) {
        uint32_t index = port / 32;
        uint32_t bit = port % 32;
        port_permissions[index] &= ~(1 << bit);
    }
}

// Check if port access is allowed
bool hal_io_port_allowed(uint16_t port) {
    uint32_t index = port / 32;
    uint32_t bit = port % 32;
    return (port_permissions[index] & (1 << bit)) != 0;
}

// Request port access (for drivers)
status_t hal_io_request_port(uint16_t port, uint16_t size) {
    // Check if ports are already allocated
    for (uint16_t p = port; p < port + size; p++) {
        if (!hal_io_port_allowed(p)) {
            return STATUS_PERMISSION_DENIED;
        }
    }
    
    // Grant access to caller process
    // This would involve capability checking in a real implementation
    return STATUS_SUCCESS;
}

// Release port access
status_t hal_io_release_port(uint16_t port, uint16_t size) {
    (void)port; (void)size;  // Suppress unused parameter warnings
    // In a real implementation, this would clear the permission bits
    return STATUS_SUCCESS;
}

// Port I/O logging for debugging
#ifdef DEBUG_IO
static void log_port_access(uint16_t port, bool is_write, uint32_t value) {
    // In a real implementation, this would log to a debug buffer
    // For now, we'll just keep it as a placeholder
}
#endif

// Enhanced I/O functions with permission checking
void hal_outb_safe(uint16_t port, uint8_t value) {
    if (!hal_io_port_allowed(port)) {
        return;  // Permission denied
    }
    
    #ifdef DEBUG_IO
    log_port_access(port, true, value);
    #endif
    
    hal_outb(port, value);
}

uint8_t hal_inb_safe(uint16_t port) {
    if (!hal_io_port_allowed(port)) {
        return 0xFF;  // Permission denied
    }
    
    uint8_t value = hal_inb(port);
    
    #ifdef DEBUG_IO
    log_port_access(port, false, value);
    #endif
    
    return value;
}

void hal_outw_safe(uint16_t port, uint16_t value) {
    if (!hal_io_port_allowed(port)) {
        return;  // Permission denied
    }
    
    #ifdef DEBUG_IO
    log_port_access(port, true, value);
    #endif
    
    hal_outw(port, value);
}

uint16_t hal_inw_safe(uint16_t port) {
    if (!hal_io_port_allowed(port)) {
        return 0xFFFF;  // Permission denied
    }
    
    uint16_t value = hal_inw(port);
    
    #ifdef DEBUG_IO
    log_port_access(port, false, value);
    #endif
    
    return value;
}

// String I/O operations (for block transfers)
void hal_outsb(uint16_t port, const uint8_t* buffer, uint32_t count) {
    if (!hal_io_port_allowed(port)) {
        return;  // Permission denied
    }
    
    for (uint32_t i = 0; i < count; i++) {
        hal_outb(port, buffer[i]);
    }
}

void hal_insb(uint16_t port, uint8_t* buffer, uint32_t count) {
    if (!hal_io_port_allowed(port)) {
        return;  // Permission denied
    }
    
    for (uint32_t i = 0; i < count; i++) {
        buffer[i] = hal_inb(port);
    }
}

// Port conflict detection
bool hal_io_port_in_use(uint16_t port) {
    // Check if any process has this port allocated
    // This would require tracking port allocations per process
    return hal_io_port_allowed(port);
}

// Get port allocation information
status_t hal_io_get_port_info(uint16_t port, uint32_t* owner_pid) {
    if (!hal_io_port_allowed(port)) {
        return STATUS_NOT_FOUND;
    }
    
    // In a real implementation, this would return the owning process ID
    *owner_pid = 0;  // Kernel-owned
    return STATUS_SUCCESS;
}
