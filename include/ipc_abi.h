#ifndef IPC_ABI_H
#define IPC_ABI_H

#include <stdint.h>

// Fixed-size IPC message structure for userspace ABI
typedef struct {
    uint32_t msg_id;           // Unique message identifier
    uint32_t sender_pid;       // Sender process ID
    uint32_t receiver_pid;     // Receiver process ID
    uint32_t msg_type;         // Message type identifier
    uint32_t flags;            // Message flags (sync/async)
    uint32_t timestamp;        // Message timestamp
    uint32_t data_size;        // Size of message data
    uint8_t data[256];        // Fixed-size message payload
} ipc_abi_message_t;

// Message types
#define MSG_DATA        0x01
#define MSG_CONTROL     0x02
#define MSG_SIGNAL      0x03
#define MSG_RESPONSE    0x04
#define MSG_DRIVER      0x05

// Driver message types
#define DRIVER_MSG_READ    0x01
#define DRIVER_MSG_WRITE   0x02
#define DRIVER_MSG_IOCTL   0x03

#endif // IPC_ABI_H
