// Kernel Capability Manager
// Enforces security through capability-based access control

#include "kernel.h"
#include "hal.h"
#include <stddef.h>

// Capability storage
static capability_t* capabilities[MAX_PROCESSES * 16];  // 16 capabilities per process max
static uint32_t capability_count = 0;
static uint32_t next_cap_id = 1;

// Forward declarations
static capability_t* capability_find_by_id(uint32_t cap_id);
static bool capability_verify_signature(capability_t* cap);
static void capability_generate_signature(capability_t* cap);

// Initialize capability system
void capability_init(void) {
    // Clear all capabilities
    for (int i = 0; i < MAX_PROCESSES * 16; i++) {
        capabilities[i] = NULL;
    }
    
    capability_count = 0;
    next_cap_id = 1;
    
    kernel_print("Capability system initialized\r\n");
}

// Create new capability
capability_t* capability_create(uint32_t owner_pid, uint32_t cap_type, uint32_t permissions) {
    if (capability_count >= MAX_PROCESSES * 16) {
        return NULL;  // Too many capabilities
    }
    
    // Allocate capability structure
    capability_t* cap = (capability_t*)memory_alloc_pages(1);
    if (!cap) {
        return NULL;
    }
    
    // Initialize capability
    cap->cap_id = next_cap_id++;
    cap->owner_pid = owner_pid;
    cap->cap_type = cap_type;
    cap->permissions = permissions;
    cap->resource_id = 0;  // No specific resource
    cap->expiration_time = 0;  // No expiration
    
    // Generate signature
    capability_generate_signature(cap);
    
    // Add to capability list
    for (int i = 0; i < MAX_PROCESSES * 16; i++) {
        if (!capabilities[i]) {
            capabilities[i] = cap;
            capability_count++;
            break;
        }
    }
    
    return cap;
}

// Check if process has capability
status_t capability_check(uint32_t pid, uint32_t cap_type, uint32_t permissions) {
    // Find all capabilities for this process
    for (int i = 0; i < MAX_PROCESSES * 16; i++) {
        capability_t* cap = capabilities[i];
        if (!cap) {
            continue;
        }
        
        if (cap->owner_pid == pid && cap->cap_type == cap_type) {
            // Check if capability has required permissions
            if ((cap->permissions & permissions) == permissions) {
                // Check if capability is still valid
                if (cap->expiration_time == 0 || cap->expiration_time > hal_timer_get_ticks()) {
                    // Verify signature
                    if (capability_verify_signature(cap)) {
                        return STATUS_SUCCESS;
                    }
                }
            }
        }
    }
    
    return STATUS_PERMISSION_DENIED;
}

// Destroy capability
void capability_destroy(capability_t* cap) {
    if (!cap) {
        return;
    }
    
    // Remove from capability list
    for (int i = 0; i < MAX_PROCESSES * 16; i++) {
        if (capabilities[i] == cap) {
            capabilities[i] = NULL;
            capability_count--;
            break;
        }
    }
    
    // Free memory
    memory_free_pages(cap, 1);
}

// Transfer capability to another process
status_t capability_transfer(capability_t* cap, uint32_t new_owner_pid) {
    if (!cap) {
        return STATUS_INVALID_PARAM;
    }
    
    // Check if current owner can transfer
    pcb_t* current = scheduler_get_current();
    if (!current || cap->owner_pid != current->pid) {
        return STATUS_PERMISSION_DENIED;
    }
    
    // Check transfer permission
    if (!(cap->permissions & PERM_TRANSFER)) {
        return STATUS_PERMISSION_DENIED;
    }
    
    // Transfer ownership
    cap->owner_pid = new_owner_pid;
    
    // Regenerate signature for new owner
    capability_generate_signature(cap);
    
    return STATUS_SUCCESS;
}

// Grant capability to process
status_t capability_grant(uint32_t pid, uint32_t cap_type, uint32_t permissions, uint32_t resource_id) {
    // Check if caller has grant permission
    pcb_t* current = scheduler_get_current();
    if (!current) {
        return STATUS_PERMISSION_DENIED;
    }
    
    // Only kernel (PID 0) can grant capabilities
    if (current->pid != 0) {
        return STATUS_PERMISSION_DENIED;
    }
    
    // Create capability
    capability_t* cap = capability_create(pid, cap_type, permissions);
    if (!cap) {
        return STATUS_OUT_OF_MEMORY;
    }
    
    cap->resource_id = resource_id;
    
    return STATUS_SUCCESS;
}

// Revoke capability from process
status_t capability_revoke(uint32_t pid, uint32_t cap_type, uint32_t resource_id) {
    // Check if caller has revoke permission
    pcb_t* current = scheduler_get_current();
    if (!current) {
        return STATUS_PERMISSION_DENIED;
    }
    
    // Only kernel (PID 0) can revoke capabilities
    if (current->pid != 0) {
        return STATUS_PERMISSION_DENIED;
    }
    
    // Find and destroy matching capabilities
    for (int i = 0; i < MAX_PROCESSES * 16; i++) {
        capability_t* cap = capabilities[i];
        if (!cap) {
            continue;
        }
        
        if (cap->owner_pid == pid && cap->cap_type == cap_type && 
            (resource_id == 0 || cap->resource_id == resource_id)) {
            capability_destroy(cap);
        }
    }
    
    return STATUS_SUCCESS;
}

// Get capability by ID
capability_t* capability_get_by_id(uint32_t cap_id) {
    return capability_find_by_id(cap_id);
}

// List all capabilities for a process
status_t capability_list_process(uint32_t pid, capability_t* caps, uint32_t* count) {
    uint32_t found = 0;
    
    for (int i = 0; i < MAX_PROCESSES * 16; i++) {
        capability_t* cap = capabilities[i];
        if (!cap) {
            continue;
        }
        
        if (cap->owner_pid == pid) {
            if (caps && found < *count) {
                caps[found] = *cap;
            }
            found++;
        }
    }
    
    if (count) {
        *count = found;
    }
    
    return STATUS_SUCCESS;
}

// Set capability expiration
status_t capability_set_expiration(capability_t* cap, uint32_t expiration_time) {
    if (!cap) {
        return STATUS_INVALID_PARAM;
    }
    
    // Check if caller owns the capability
    pcb_t* current = scheduler_get_current();
    if (!current || cap->owner_pid != current->pid) {
        return STATUS_PERMISSION_DENIED;
    }
    
    cap->expiration_time = expiration_time;
    
    return STATUS_SUCCESS;
}

// Clean up expired capabilities
void capability_cleanup_expired(void) {
    uint32_t current_time = hal_timer_get_ticks();
    
    for (int i = 0; i < MAX_PROCESSES * 16; i++) {
        capability_t* cap = capabilities[i];
        if (!cap) {
            continue;
        }
        
        if (cap->expiration_time > 0 && cap->expiration_time <= current_time) {
            capability_destroy(cap);
        }
    }
}

// Get capability statistics
void capability_get_stats(uint32_t* total_caps, uint32_t* caps_per_process) {
    if (total_caps) *total_caps = capability_count;
    if (caps_per_process) *caps_per_process = 16;  // Max per process
}

// Find capability by ID
static capability_t* capability_find_by_id(uint32_t cap_id) {
    for (int i = 0; i < MAX_PROCESSES * 16; i++) {
        capability_t* cap = capabilities[i];
        if (cap && cap->cap_id == cap_id) {
            return cap;
        }
    }
    return NULL;
}

// Verify capability signature (simplified)
static bool capability_verify_signature(capability_t* cap) {
    if (!cap) {
        return false;
    }
    
    // In a real implementation, this would use cryptographic signatures
    // For now, we'll use a simple checksum
    uint32_t checksum = cap->cap_id ^ cap->owner_pid ^ cap->cap_type ^ 
                       cap->permissions ^ cap->resource_id ^ cap->expiration_time;
    
    return (checksum == *(uint32_t*)cap->signature);
}

// Generate capability signature (simplified)
static void capability_generate_signature(capability_t* cap) {
    if (!cap) {
        return;
    }
    
    // In a real implementation, this would use cryptographic signatures
    // For now, we'll use a simple checksum
    uint32_t checksum = cap->cap_id ^ cap->owner_pid ^ cap->cap_type ^ 
                       cap->permissions ^ cap->resource_id ^ cap->expiration_time;
    
    *(uint32_t*)cap->signature = checksum;
}
