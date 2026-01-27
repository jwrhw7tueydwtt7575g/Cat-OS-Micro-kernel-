// Test Framework for MiniSecureOS
// Provides unit testing capabilities

#include "userspace.h"
#include "driver.h"

// Test framework state
static uint32_t tests_run = 0;
static uint32_t tests_passed = 0;
static uint32_t tests_failed = 0;

// Test structure
typedef struct {
    const char* name;
    void (*test_func)(void);
    bool passed;
    const char* error_msg;
} test_t;

// Forward declarations
static void test_memory_allocation(void);
static void test_ipc_messaging(void);
static void test_process_creation(void);
static void test_driver_communication(void);
static void test_timer_functionality(void);
static void test_capability_system(void);

// Test suite
static test_t tests[] = {
    {"Memory Allocation", test_memory_allocation, false, NULL},
    {"IPC Messaging", test_ipc_messaging, false, NULL},
    {"Process Creation", test_process_creation, false, NULL},
    {"Driver Communication", test_driver_communication, false, NULL},
    {"Timer Functionality", test_timer_functionality, false, NULL},
    {"Capability System", test_capability_system, false, NULL}
};

static const uint32_t test_count = sizeof(tests) / sizeof(test_t);

// Test assertion macros
#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            current_test->passed = false; \
            current_test->error_msg = message; \
            return; \
        } \
    } while(0)

#define ASSERT_EQ(expected, actual, message) \
    ASSERT((expected) == (actual), message)

#define ASSERT_NE(value1, value2, message) \
    ASSERT((value1) != (value2), message)

#define ASSERT_NULL(ptr, message) \
    ASSERT((ptr) == NULL, message)

#define ASSERT_NOT_NULL(ptr, message) \
    ASSERT((ptr) != NULL, message)

// Test runner
int main(void) {
    print("MiniSecureOS Test Framework v1.0\r\n");
    print("Running ");
    print_hex(test_count);
    print(" tests...\r\n\r\n");
    
    // Run all tests
    for (uint32_t i = 0; i < test_count; i++) {
        test_t* current_test = &tests[i];
        
        print("Running test: ");
        print(current_test->name);
        print("... ");
        
        current_test->passed = true;
        current_test->error_msg = NULL;
        
        // Run test
        current_test->test_func();
        
        // Update statistics
        tests_run++;
        if (current_test->passed) {
            tests_passed++;
            print("PASSED\r\n");
        } else {
            tests_failed++;
            print("FAILED\r\n");
            if (current_test->error_msg) {
                print("  Error: ");
                print(current_test->error_msg);
                print("\r\n");
            }
        }
    }
    
    // Print summary
    print("\r\n=== TEST SUMMARY ===\r\n");
    print("Tests Run: ");
    print_hex(tests_run);
    print("\r\n");
    print("Tests Passed: ");
    print_hex(tests_passed);
    print("\r\n");
    print("Tests Failed: ");
    print_hex(tests_failed);
    print("\r\n");
    
    if (tests_failed == 0) {
        print("All tests PASSED!\r\n");
        return 0;
    } else {
        print("Some tests FAILED!\r\n");
        return 1;
    }
}

// Test implementations
static void test_memory_allocation(void) {
    // Test basic memory allocation
    void* ptr = memory_alloc(1024);
    ASSERT_NOT_NULL(ptr, "Memory allocation failed");
    
    // Test memory free
    memory_free(ptr);
    
    // Test large allocation
    void* large_ptr = memory_alloc(64 * 1024);  // 64KB
    ASSERT_NOT_NULL(large_ptr, "Large memory allocation failed");
    memory_free(large_ptr);
    
    // Test multiple allocations
    void* ptrs[10];
    for (int i = 0; i < 10; i++) {
        ptrs[i] = memory_alloc(1024);
        ASSERT_NOT_NULL(ptrs[i], "Multiple allocation failed");
    }
    
    // Free all allocations
    for (int i = 0; i < 10; i++) {
        memory_free(ptrs[i]);
    }
}

static void test_ipc_messaging(void) {
    // Test message creation and sending
    ipc_message_t msg;
    msg.msg_type = MSG_DATA;
    msg.data_size = sizeof(uint32_t);
    *(uint32_t*)msg.data = 0x12345678;
    
    // Send message to self (PID 0 = any)
    uint32_t result = ipc_send(0, &msg);
    ASSERT_EQ(STATUS_SUCCESS, result, "IPC send failed");
    
    // Receive message
    ipc_message_t received_msg;
    result = ipc_receive(0, &received_msg, true);
    ASSERT_EQ(STATUS_SUCCESS, result, "IPC receive failed");
    
    // Verify message content
    ASSERT_EQ(MSG_DATA, received_msg.msg_type, "Wrong message type");
    ASSERT_EQ(sizeof(uint32_t), received_msg.data_size, "Wrong message size");
    ASSERT_EQ(0x12345678, *(uint32_t*)received_msg.data, "Wrong message data");
}

static void test_process_creation(void) {
    // Test process creation
    uint32_t child_pid = process_create();
    ASSERT_NE(0, child_pid, "Process creation failed");
    
    // Test process exit (in child)
    if (child_pid == 0) {
        // Child process
        process_exit(0);
    } else {
        // Parent process - wait a bit for child to exit
        sleep(100);
    }
}

static void test_driver_communication(void) {
    // Test communication with console driver
    ipc_message_t msg;
    msg.msg_type = DRIVER_MSG_WRITE;
    msg.data_size = 5;  // "test\0"
    memcpy(msg.data, "test", 5);
    
    uint32_t result = driver_request(3, &msg);  // Console driver PID
    ASSERT_EQ(STATUS_SUCCESS, result, "Driver request failed");
    
    // Test communication with timer driver
    msg.msg_type = DRIVER_MSG_READ;
    msg.data_size = 0;
    
    result = driver_request(4, &msg);  // Timer driver PID
    ASSERT_EQ(STATUS_SUCCESS, result, "Timer driver request failed");
    
    // Wait for response
    ipc_message_t response;
    result = ipc_receive(4, &response, true);
    ASSERT_EQ(STATUS_SUCCESS, result, "Timer driver response failed");
    ASSERT_EQ(DRIVER_MSG_READ, response.msg_type, "Wrong response type");
}

static void test_timer_functionality(void) {
    // Test timer tick retrieval
    uint32_t start_ticks = driver_get_ticks();
    ASSERT_NE(0, start_ticks, "Failed to get timer ticks");
    
    // Test sleep functionality
    sleep(100);  // Sleep 100ms
    
    uint32_t end_ticks = driver_get_ticks();
    ASSERT(end_ticks > start_ticks, "Timer did not advance");
    
    // Test timer delay request
    ipc_message_t msg;
    msg.msg_type = DRIVER_MSG_IOCTL;
    msg.data_size = 3 * sizeof(uint32_t);
    uint32_t* data = (uint32_t*)msg.data;
    data[0] = 0x03;  // Set delay request
    data[1] = 50;    // 50ms delay
    data[2] = 0;
    
    uint32_t request_id = driver_request(4, &msg);  // Timer driver PID
    ASSERT_NE(0, request_id, "Timer delay request failed");
    
    // Wait for timer notification
    ipc_message_t response;
    uint32_t result = ipc_receive(0, &response, true);
    ASSERT_EQ(STATUS_SUCCESS, result, "Timer notification failed");
    ASSERT_EQ(DRIVER_MSG_IOCTL, response.msg_type, "Wrong notification type");
    ASSERT_EQ(request_id, *(uint32_t*)response.data, "Wrong request ID");
}

static void test_capability_system(void) {
    // Test capability checking (this would require kernel support)
    // For now, we'll test the system call interface
    
    // Test invalid capability check
    uint32_t result = syscall(0xFF, 0, 0, 0);  // Invalid syscall
    ASSERT_EQ(STATUS_INVALID_PARAM, result, "Invalid syscall should fail");
    
    // Test valid syscall
    result = syscall(SYS_PROCESS_YIELD, 0, 0, 0);
    ASSERT_EQ(STATUS_SUCCESS, result, "Process yield should succeed");
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

void print(const char* str) {
    // Send to console driver
    ipc_message_t msg;
    msg.msg_type = DRIVER_MSG_WRITE;
    msg.data_size = strlen(str) + 1;
    memcpy(msg.data, str, msg.data_size);
    ipc_send(3, &msg);  // Console driver PID
}

void print_hex(uint32_t value) {
    const char hex_chars[] = "0123456789ABCDEF";
    char buffer[11] = "0x00000000";
    
    for (int i = 0; i < 8; i++) {
        buffer[9 - i] = hex_chars[value & 0xF];
        value >>= 4;
    }
    
    print(buffer);
}

void sleep(uint32_t ms) {
    // Request timer driver to sleep
    ipc_message_t msg;
    msg.msg_type = DRIVER_MSG_IOCTL;
    msg.data_size = 3 * sizeof(uint32_t);
    uint32_t* data = (uint32_t*)msg.data;
    data[0] = 0x03;  // Set delay request
    data[1] = ms;    // Delay in milliseconds
    data[2] = 0;     // Unused
    
    uint32_t request_id = driver_request(4, &msg);  // Timer driver PID
    
    if (request_id != 0) {
        // Wait for timer notification
        ipc_message_t response;
        while (ipc_receive(0, &response, true) == STATUS_SUCCESS) {
            if (response.msg_type == DRIVER_MSG_IOCTL && 
                response.data_size >= sizeof(uint32_t) &&
                *(uint32_t*)response.data == request_id) {
                break;
            }
        }
    }
}
