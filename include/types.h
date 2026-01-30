#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

// Round up division macro
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

// Basic types
typedef int8_t int8_t;
typedef int16_t int16_t;
typedef int32_t int32_t;
typedef int64_t int64_t;

// Boolean type
typedef uint8_t bool;
#define true 1
#define false 0

// Permission bits
#define PERM_READ       0x01
#define PERM_WRITE      0x02
#define PERM_EXECUTE    0x04
#define PERM_CREATE     0x08
#define PERM_DELETE     0x10
#define PERM_TRANSFER   0x20
#define PERM_ALLOC      0x40
#define PERM_FREE       0x80

typedef int8_t int8_t;
typedef int16_t int16_t;
typedef int32_t int32_t;
typedef int64_t int64_t;

// Boolean type
typedef uint8_t bool;
#define true 1
#define false 0

// Status codes
typedef enum {
    STATUS_SUCCESS = 0,
    STATUS_ERROR = -1,
    STATUS_INVALID_PARAM = -2,
    STATUS_OUT_OF_MEMORY = -3,
    STATUS_PERMISSION_DENIED = -4,
    STATUS_NOT_FOUND = -5,
    STATUS_TIMEOUT = -6,
    STATUS_ALREADY_EXISTS = -7,
    STATUS_NOT_IMPLEMENTED = -8
} status_t;

// Process states
typedef enum {
    PROCESS_CREATED = 0,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;

// Capability types
typedef enum {
    CAP_PROCESS = 0,
    CAP_MEMORY,
    CAP_DRIVER,
    CAP_HARDWARE,
    CAP_SYSTEM,
    CAP_IPC
} capability_type_t;

#endif // TYPES_H
