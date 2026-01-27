// Shell Program
// Command-line interface for MiniSecureOS

#include "userspace.h"
#include "driver.h"

// Shell state
static bool shell_running = true;
static char command_buffer[256];
static uint32_t command_pos = 0;

// Command structure
typedef struct {
    const char* name;
    const char* description;
    int (*handler)(int argc, char* argv[]);
} command_t;

// Forward declarations
static void display_prompt(void);
static void read_command(void);
static void execute_command(void);
static int parse_command(char* cmd, char* argv[], int max_args);
static void clear_command_buffer(void);

// External utility functions (from userspace.c)
extern void print(const char* str);
extern void print_hex(uint32_t value);
extern void sleep(uint32_t ms);

// Command handlers
static int cmd_help(int argc, char* argv[]);
static int cmd_clear(int argc, char* argv[]);
static int cmd_exit(int argc, char* argv[]);
static int cmd_ps(int argc, char* argv[]);
static int cmd_kill(int argc, char* argv[]);
static int cmd_mem(int argc, char* argv[]);
static int cmd_uptime(int argc, char* argv[]);
static int cmd_drivers(int argc, char* argv[]);
static int cmd_test(int argc, char* argv[]);

// Command table
static command_t commands[] = {
    {"help", "Show available commands", cmd_help},
    {"clear", "Clear screen", cmd_clear},
    {"exit", "Exit shell", cmd_exit},
    {"ps", "List processes", cmd_ps},
    {"kill", "Kill a process", cmd_kill},
    {"mem", "Show memory usage", cmd_mem},
    {"uptime", "Show system uptime", cmd_uptime},
    {"drivers", "List active drivers", cmd_drivers},
    {"test", "Run system tests", cmd_test}
};

static const uint32_t command_count = sizeof(commands) / sizeof(commands[0]);

// Main function
int main(void) {
    print("MiniSecureOS Shell v1.0\r\n");
    print("Type 'help' for available commands\r\n");
    
    display_prompt();
    
    while (shell_running) {
        read_command();
        
        if (command_pos > 0) {
            execute_command();
        }
        
        clear_command_buffer();
    }
    
    print("Shell terminated\r\n");
    return 0;
}

// Display command prompt
static void display_prompt(void) {
    print("MiniSecureOS> ");
}

// Read command from user input
static void read_command(void) {
    // Request keyboard input from driver
    ipc_abi_message_t msg = {0};
    msg.msg_type = DRIVER_MSG_READ;
    msg.data_size = 0;
    
    uint32_t result = driver_request(2, &msg);  // Keyboard driver PID
    
    if (result == 0) {
        ipc_abi_message_t response = {0};
        if (ipc_receive(2, &response, true) == 0) {  // STATUS_SUCCESS
            if (response.data_size >= sizeof(uint8_t)) {
                uint8_t ch = *(uint8_t*)response.data;
                
                if (ch == '\r') {
                    // End of command
                    print("\r\n");
                    command_buffer[command_pos] = '\0';
                    execute_command();
                    clear_command_buffer();
                    display_prompt();
                } else if (ch == '\b' || ch == 127) {
                    // Backspace
                    if (command_pos > 0) {
                        command_pos--;
                        print("\b \b");
                    }
                } else if (ch >= 32 && ch <= 126 && command_pos < sizeof(command_buffer) - 1) {
                    // Printable character
                    command_buffer[command_pos++] = ch;
                    // Echo character
                    char echo[2] = {ch, '\0'};
                    print(echo);
                }
            }
        }
    }
}

// Execute command
static void execute_command(void) {
    char* argv[16];
    int argc = parse_command(command_buffer, argv, 16);
    
    if (argc == 0) {
        return;
    }
    
    // Find command
    for (uint32_t i = 0; i < command_count; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            commands[i].handler(argc, argv);
            return;
        }
    }
    
    print("Unknown command: ");
    print(argv[0]);
    print("\r\n");
    print("Type 'help' for available commands\r\n");
}

// Parse command into arguments
static int parse_command(char* cmd, char* argv[], int max_args) {
    int argc = 0;
    char* p = cmd;
    
    while (*p && argc < max_args) {
        // Skip whitespace
        while (*p && (*p == ' ' || *p == '\t')) {
            p++;
        }
        
        if (!*p) {
            break;
        }
        
        // Start of argument
        argv[argc++] = p;
        
        // Find end of argument
        while (*p && *p != ' ' && *p != '\t') {
            p++;
        }
        
        // Terminate argument
        if (*p) {
            *p++ = '\0';
        }
    }
    
    return argc;
}

// Clear command buffer
static void clear_command_buffer(void) {
    command_pos = 0;
    memset(command_buffer, 0, sizeof(command_buffer));
}

// Command handlers
static int cmd_help(int argc, char* argv[]) {
    (void)argc; (void)argv;  // Suppress unused parameter warnings
    print("Available commands:\r\n");
    for (uint32_t i = 0; i < command_count; i++) {
        print("  ");
        print(commands[i].name);
        print(" - ");
        print(commands[i].description);
        print("\r\n");
    }
    return 0;
}

static int cmd_clear(int argc, char* argv[]) {
    (void)argc; (void)argv;  // Suppress unused parameter warnings
    // Send clear command to console driver
    ipc_abi_message_t msg = {0};
    msg.msg_type = DRIVER_MSG_IOCTL;
    msg.data_size = sizeof(uint32_t);
    *(uint32_t*)msg.data = 0x01;  // Clear screen command
    driver_request(3, &msg);  // Console driver PID
    return 0;
}

static int cmd_exit(int argc, char* argv[]) {
    (void)argc; (void)argv;  // Suppress unused parameter warnings
    shell_running = false;
    return 0;
}

static int cmd_ps(int argc, char* argv[]) {
    (void)argc; (void)argv;  // Suppress unused parameter warnings
    print("PID\tSTATE\tCPU_TIME\r\n");
    print("---\t-----\t--------\r\n");
    
    // In a real implementation, this would query the kernel for process list
    // For now, we'll show some example processes
    print("1\tRUNNING\t1000\r\n");  // Init
    print("2\tRUNNING\t500\r\n");   // Keyboard driver
    print("3\tRUNNING\t300\r\n");   // Console driver
    print("4\tRUNNING\t200\r\n");   // Timer driver
    print("5\tRUNNING\t100\r\n");   // Shell
    
    return 0;
}

static int cmd_kill(int argc, char* argv[]) {
    (void)argc; (void)argv;  // Suppress unused parameter warnings
    if (argc < 2) {
        print("Usage: kill <pid>\r\n");
        return 1;
    }
    
    uint32_t pid = 0;
    for (char* p = argv[1]; *p; p++) {
        if (*p >= '0' && *p <= '9') {
            pid = pid * 10 + (*p - '0');
        } else {
            print("Invalid PID\r\n");
            return 1;
        }
    }
    
    uint32_t result = process_kill(pid);
    if (result == 0) {
        print("Process ");
        print_hex(pid);
        print(" killed\r\n");
    } else {
        print("Failed to kill process ");
        print_hex(pid);
        print("\r\n");
    }
    
    return 0;
}

static int cmd_mem(int argc, char* argv[]) {
    (void)argc; (void)argv;  // Suppress unused parameter warnings
    print("Memory Usage:\r\n");
    print("Total: 16MB\r\n");
    print("Used:  4MB\r\n");
    print("Free:  12MB\r\n");
    return 0;
}

static int cmd_uptime(int argc, char* argv[]) {
    (void)argc; (void)argv;  // Suppress unused parameter warnings
    // Get uptime from timer driver
    ipc_abi_message_t msg = {0};
    msg.msg_type = DRIVER_MSG_READ;
    msg.data_size = 0;
    
    uint32_t result = driver_request(4, &msg);  // Timer driver PID
    
    if (result == 0) {
        ipc_abi_message_t response = {0};
        if (ipc_receive(4, &response, true) == 0) {  // STATUS_SUCCESS
            if (response.data_size >= sizeof(uint32_t)) {
                uint32_t ticks = *(uint32_t*)response.data;
                uint32_t seconds = ticks / 100;  // Assuming 100 Hz timer
                
                print("System uptime: ");
                print_hex(seconds);
                print(" seconds\r\n");
            }
        }
    }
    
    return 0;
}

static int cmd_drivers(int argc, char* argv[]) {
    (void)argc; (void)argv;  // Suppress unused parameter warnings
    print("Active Drivers:\r\n");
    print("PID\tNAME\t\tCAPABILITIES\r\n");
    print("---\t----\t\t-----------\r\n");
    print("2\tkeyboard\t\tREAD, IRQ\r\n");
    print("3\tconsole\t\tWRITE\r\n");
    print("4\ttimer\t\tREAD, IOCTL\r\n");
    return 0;
}

static int cmd_test(int argc, char* argv[]) {
    (void)argc; (void)argv;  // Suppress unused parameter warnings
    print("Running system tests...\r\n");
    
    // Test memory allocation
    print("Testing memory allocation...\r\n");
    void* ptr = memory_alloc(1024);
    if (ptr) {
        print("Memory allocation: SUCCESS\r\n");
        memory_free(ptr);
    } else {
        print("Memory allocation: FAILED\r\n");
    }
    
    // Test IPC
    print("Testing IPC...\r\n");
    ipc_abi_message_t test_msg = {0};
    test_msg.msg_type = MSG_DATA;
    test_msg.data_size = 4;
    *(uint32_t*)test_msg.data = 0x12345678;
    
    uint32_t result = ipc_send(1, &test_msg);  // Send to init
    if (result == 0) {
        print("IPC send: SUCCESS\r\n");
    } else {
        print("IPC send: FAILED\r\n");
    }
    
    // Test timer
    print("Testing timer...\r\n");
    uint32_t start_ticks = driver_get_ticks();
    sleep(100);  // Sleep 100ms
    uint32_t end_ticks = driver_get_ticks();
    
    if (end_ticks > start_ticks) {
        print("Timer test: SUCCESS\r\n");
    } else {
        print("Timer test: FAILED\r\n");
    }
    
    print("System tests completed\r\n");
    return 0;
}
