// Kernel IPC Module
// Inter-process communication via message passing

#include "kernel.h"
#include "hal.h"
#include <stddef.h>

// External memcpy from kernel main
extern void* memcpy(void* dest, const void* src, uint32_t n);

// IPC state
static ipc_message_t* message_queues[MAX_PROCESSES];
static uint32_t next_msg_id = 1;
static void (*msg_handlers[32])(ipc_message_t*) = {NULL};

// Message queue structure
typedef struct {
    ipc_message_t* head;
    ipc_message_t* tail;
    uint32_t count;
    uint32_t max_count;
} message_queue_t;

// Forward declarations
static void ipc_add_to_queue(uint32_t pid, ipc_message_t* msg);
static ipc_message_t* ipc_remove_from_queue(uint32_t pid);
static ipc_message_t* ipc_find_in_queue(uint32_t pid, uint32_t sender_pid);
static void ipc_wakeup_receiver(uint32_t pid);

// Initialize IPC system
void ipc_init(void) {
    // Clear all message queues
    for (int i = 0; i < MAX_PROCESSES; i++) {
        message_queues[i] = NULL;
    }
    
    next_msg_id = 1;
    
    // Clear message handlers
    for (int i = 0; i < 32; i++) {
        msg_handlers[i] = NULL;
    }
    
    kernel_print("IPC system initialized\r\n");
}

// Send message to another process
status_t ipc_send(uint32_t receiver_pid, ipc_abi_message_t* user_msg) {
    if (!user_msg) {
        return STATUS_INVALID_PARAM;
    }
    
    pcb_t* receiver = scheduler_find_process(receiver_pid);
    if (!receiver) {
        return STATUS_NOT_FOUND;
    }
    
    // Validate message size
    if (user_msg->data_size > 256) {
        return STATUS_INVALID_PARAM;
    }
    
    // Allocate kernel message with proper size
    size_t msg_size = sizeof(ipc_message_t) + user_msg->data_size;
    ipc_message_t* kernel_msg = (ipc_message_t*)memory_alloc_pages(
        (msg_size + PAGE_SIZE - 1) / PAGE_SIZE
    );
    
    if (!kernel_msg) {
        return STATUS_OUT_OF_MEMORY;
    }
    
    // Copy message header
    kernel_msg->msg_id = next_msg_id++;
    kernel_msg->sender_pid = scheduler_get_current()->pid;
    kernel_msg->receiver_pid = receiver_pid;
    kernel_msg->msg_type = user_msg->msg_type;
    kernel_msg->flags = user_msg->flags;
    kernel_msg->timestamp = 0;  // Would get from timer
    kernel_msg->data_size = user_msg->data_size;
    kernel_msg->next = NULL;
    
    // Copy message data from userspace
    if (user_msg->data_size > 0) {
        memcpy(kernel_msg->data, user_msg->data, user_msg->data_size);
    }
    
    // Add to receiver's queue
    ipc_add_to_queue(receiver_pid, kernel_msg);
    
    // Wake up receiver if blocked
    ipc_wakeup_receiver(receiver_pid);
    
    return STATUS_SUCCESS;
}

// Receive message from another process
status_t ipc_receive(uint32_t sender_pid, ipc_abi_message_t* user_msg, bool block) {
    pcb_t* receiver = scheduler_get_current();
    if (!receiver) {
        return STATUS_PERMISSION_DENIED;
    }
    
    // Try to find message in queue
    ipc_message_t* kernel_msg = ipc_find_in_queue(receiver->pid, sender_pid);
    if (!kernel_msg) {
        if (block) {
            // Block current process until message arrives
            scheduler_block_current();
            return STATUS_SUCCESS;  // Will be resumed when message arrives
        } else {
            return STATUS_NOT_FOUND;
        }
    }
    
    // Copy message back to userspace
    if (user_msg) {
        // Copy message header
        user_msg->msg_id = kernel_msg->msg_id;
        user_msg->sender_pid = kernel_msg->sender_pid;
        user_msg->receiver_pid = kernel_msg->receiver_pid;
        user_msg->msg_type = kernel_msg->msg_type;
        user_msg->flags = kernel_msg->flags;
        user_msg->timestamp = kernel_msg->timestamp;
        user_msg->data_size = kernel_msg->data_size;
        
        // Copy message data
        if (kernel_msg->data_size > 0 && kernel_msg->data_size <= 256) {
            memcpy(user_msg->data, kernel_msg->data, kernel_msg->data_size);
        }
    }
    
    // Free kernel message
    memory_free_pages(kernel_msg, 1);
    
    return STATUS_SUCCESS;
}

// Register message handler
status_t ipc_register_handler(uint32_t msg_type, void (*handler)(ipc_message_t*)) {
    if (msg_type >= 32 || !handler) {
        return STATUS_INVALID_PARAM;
    }
    
    msg_handlers[msg_type] = handler;
    return STATUS_SUCCESS;
}

// Broadcast message to all processes
status_t ipc_broadcast(uint32_t msg_type, ipc_abi_message_t* user_msg) {
    (void)msg_type;  // Suppress unused parameter warning
    if (!user_msg) {
        return STATUS_INVALID_PARAM;
    }
    
    uint32_t sent_count = 0;
    
    // Send to all processes
    for (int i = 1; i < MAX_PROCESSES; i++) {  // Skip PID 0 (kernel)
        // Create a copy of the message for each process
        status_t result = ipc_send(i, user_msg);
        if (result == STATUS_SUCCESS) {
            sent_count++;
        }
    }
    
    return (sent_count > 0) ? STATUS_SUCCESS : STATUS_ERROR;
}

// Get message queue statistics
status_t ipc_get_queue_stats(uint32_t pid, uint32_t* count, uint32_t* max_count) {
    if (pid >= MAX_PROCESSES) {
        return STATUS_INVALID_PARAM;
    }
    
    message_queue_t* queue = (message_queue_t*)message_queues[pid];
    if (!queue) {
        if (count) *count = 0;
        if (max_count) *max_count = 0;
        return STATUS_SUCCESS;
    }
    
    if (count) *count = queue->count;
    if (max_count) *max_count = queue->max_count;
    
    return STATUS_SUCCESS;
}

// Clear message queue
status_t ipc_clear_queue(uint32_t pid) {
    if (pid >= MAX_PROCESSES) {
        return STATUS_INVALID_PARAM;
    }
    
    message_queue_t* queue = (message_queue_t*)message_queues[pid];
    if (!queue) {
        return STATUS_SUCCESS;
    }
    
    // Free all messages
    ipc_message_t* current = queue->head;
    while (current) {
        ipc_message_t* next = current->next;
        memory_free_pages(current, 1);
        current = next;
    }
    
    // Clear queue
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    
    return STATUS_SUCCESS;
}

// Add message to queue
static void ipc_add_to_queue(uint32_t pid, ipc_message_t* msg) {
    if (!msg) {
        return;
    }
    
    // Create queue if it doesn't exist
    if (!message_queues[pid]) {
        message_queues[pid] = (ipc_message_t*)memory_alloc_pages(1);
        if (!message_queues[pid]) {
            return;
        }
        
        // Initialize queue header
        message_queue_t* queue = (message_queue_t*)message_queues[pid];
        queue->head = NULL;
        queue->tail = NULL;
        queue->count = 0;
        queue->max_count = 100;  // Maximum 100 messages per process
    }
    
    message_queue_t* queue = (message_queue_t*)message_queues[pid];
    
    // Check queue limit
    if (queue->count >= queue->max_count) {
        // Remove oldest message
        ipc_remove_from_queue(pid);
    }
    
    // Add to end of queue
    msg->next = NULL;
    
    if (!queue->head) {
        queue->head = msg;
        queue->tail = msg;
    } else {
        queue->tail->next = msg;
        queue->tail = msg;
    }
    
    queue->count++;
}

// Remove message from queue
static ipc_message_t* ipc_remove_from_queue(uint32_t pid) {
    message_queue_t* queue = (message_queue_t*)message_queues[pid];
    if (!queue || !queue->head) {
        return NULL;
    }
    
    ipc_message_t* msg = queue->head;
    queue->head = msg->next;
    
    if (!queue->head) {
        queue->tail = NULL;
    }
    
    queue->count--;
    msg->next = NULL;
    
    return msg;
}

// Find message from specific sender
static ipc_message_t* ipc_find_in_queue(uint32_t pid, uint32_t sender_pid) {
    message_queue_t* queue = (message_queue_t*)message_queues[pid];
    if (!queue) {
        return NULL;
    }
    
    ipc_message_t* current = queue->head;
    ipc_message_t* prev = NULL;
    
    while (current) {
        if (sender_pid == 0 || current->sender_pid == sender_pid) {
            // Remove from queue
            if (prev) {
                prev->next = current->next;
            } else {
                queue->head = current->next;
            }
            
            if (current == queue->tail) {
                queue->tail = prev;
            }
            
            queue->count--;
            current->next = NULL;
            return current;
        }
        
        prev = current;
        current = current->next;
    }
    
    return NULL;
}

// Wake up receiver process
static void ipc_wakeup_receiver(uint32_t pid) {
    pcb_t* process = scheduler_find_process(pid);
    if (process && process->state == PROCESS_BLOCKED) {
        scheduler_unblock_process(process);
        process->waiting_for = 0;
    }
}
