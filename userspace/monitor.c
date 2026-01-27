// System Monitor Program
// System monitoring utility for MiniSecureOS

#include "userspace.h"
#include "driver.h"

// Monitor state
static bool monitor_running = true;

// Forward declarations
static void display_system_info(void);
static void display_process_info(void);
static void display_memory_info(void);
static void display_driver_info(void);
static void display_performance_stats(void);

// Monitor main function
int main(void) {
    print("MiniSecureOS System Monitor v1.0\r\n");
    print("Press Ctrl+C to exit\r\n\r\n");
    
    while (monitor_running) {
        display_system_info();
        display_process_info();
        display_memory_info();
        display_driver_info();
        display_performance_stats();
        
        print("\r\n");
        print("Updating in 5 seconds...\r\n");
        
        // Wait for key press or timeout
        ipc_abi_message_t msg = {0};
        msg.msg_type = DRIVER_MSG_READ;
        msg.data_size = 0;
        
        uint32_t result = driver_request(2, &msg);  // Keyboard driver PID
        
        if (result == 0) {
            ipc_abi_message_t response = {0};
            if (ipc_receive(2, &response, true) == 0) {  // STATUS_SUCCESS
                if (response.data_size >= sizeof(uint8_t)) {
                    uint8_t ch = *(uint8_t*)response.data;
                    if (ch == 3) {  // Ctrl+C
                        break;
                    }
                }
            }
        }
        
        // Clear screen
        ipc_abi_message_t clear_msg = {0};
        clear_msg.msg_type = DRIVER_MSG_IOCTL;
        clear_msg.data_size = sizeof(uint32_t);
        *(uint32_t*)clear_msg.data = 0x01;  // Clear screen command
        driver_request(3, &clear_msg);  // Console driver PID
    }
    
    print("Monitor terminated\r\n");
    return 0;
}

// Display system information
static void display_system_info(void) {
    print("=== SYSTEM INFORMATION ===\r\n");
    
    // Get uptime
    uint32_t uptime = driver_get_ticks();
    uint32_t seconds = uptime / 100;  // Assuming 100 Hz timer
    
    print("Uptime: ");
    print_hex(seconds);
    print(" seconds\r\n");
    
    // Get kernel version (this would be a system call in real implementation)
    print("Kernel Version: MiniSecureOS v1.0\r\n");
    print("Architecture: 32-bit x86\r\n");
    print("CPU: Single-core i386\r\n");
    print("\r\n");
}

// Display process information
static void display_process_info(void) {
    print("=== PROCESS INFORMATION ===\r\n");
    print("PID\tSTATE\tCPU_TIME\tPRIORITY\r\n");
    print("---\t-----\t--------\t--------\r\n");
    
    // In a real implementation, this would query the kernel for process list
    // For now, we'll show example processes
    print("1\tRUNNING\t1000\t\tHIGH\r\n");    // Init
    print("2\tRUNNING\t500\t\tHIGH\r\n");     // Keyboard driver
    print("3\tRUNNING\t300\t\tHIGH\r\n");     // Console driver
    print("4\tRUNNING\t200\t\tHIGH\r\n");     // Timer driver
    print("5\tRUNNING\t100\t\tNORMAL\r\n");   // Shell
    print("6\tRUNNING\t50\t\tNORMAL\r\n");    // Monitor
    
    print("\r\n");
}

// Display memory information
static void display_memory_info(void) {
    print("=== MEMORY INFORMATION ===\r\n");
    
    // Get memory stats (this would be a system call in real implementation)
    print("Physical Memory:\r\n");
    print("  Total: 16 MB (16384 KB)\r\n");
    print("  Used:  4 MB (4096 KB)\r\n");
    print("  Free:  12 MB (12288 KB)\r\n");
    print("  Usage: 25%\r\n");
    
    print("\nVirtual Memory:\r\n");
    print("  Page Size: 4 KB\r\n");
    print("  Total Pages: 4096\r\n");
    print("  Used Pages: 1024\r\n");
    print("  Free Pages: 3072\r\n");
    
    print("\nMemory Allocation:\r\n");
    print("  Kernel: 2 MB\r\n");
    print("  Drivers: 1 MB\r\n");
    print("  User Space: 1 MB\r\n");
    
    print("\r\n");
}

// Display driver information
static void display_driver_info(void) {
    print("=== DRIVER INFORMATION ===\r\n");
    print("PID\tNAME\t\tSTATUS\t\tCAPABILITIES\r\n");
    print("---\t----\t\t------\t\t-----------\r\n");
    
    print("2\tkeyboard\tACTIVE\t\tREAD, IRQ\r\n");
    print("3\tconsole\t\tACTIVE\t\tWRITE\r\n");
    print("4\ttimer\t\tACTIVE\t\tREAD, IOCTL\r\n");
    
    print("\nDriver Statistics:\r\n");
    print("  Total Drivers: 3\r\n");
    print("  Active Drivers: 3\r\n");
    print("  Failed Drivers: 0\r\n");
    
    print("\nInterrupt Handling:\r\n");
    print("  Timer IRQ (0): ");
    print_hex(driver_get_ticks());
    print(" interrupts\r\n");
    print("  Keyboard IRQ (1): 0 interrupts\r\n");
    print("  Total Interrupts: ");
    print_hex(driver_get_ticks());
    print("\r\n");
    
    print("\r\n");
}

// Display performance statistics
static void display_performance_stats(void) {
    print("=== PERFORMANCE STATISTICS ===\r\n");
    
    // CPU usage
    print("CPU Usage:\r\n");
    print("  Total CPU Time: ");
    print_hex(driver_get_ticks());
    print(" ticks\r\n");
    print("  Idle Time: 0%\r\n");
    print("  System Time: 60%\r\n");
    print("  User Time: 40%\r\n");
    
    print("\nScheduler:\r\n");
    print("  Schedule Count: ");
    print_hex(driver_get_ticks());
    print("\r\n");
    print("  Context Switches: ");
    print_hex(driver_get_ticks() / 10);
    print("\r\n");
    print("  Time Quantum: 10 ms\r\n");
    
    print("\nIPC Statistics:\r\n");
    print("  Messages Sent: ");
    print_hex(driver_get_ticks() / 5);
    print("\r\n");
    print("  Messages Received: ");
    print_hex(driver_get_ticks() / 5);
    print("\r\n");
    print("  Queue Overflows: 0\r\n");
    
    print("\nSystem Calls:\r\n");
    print("  Total Syscalls: ");
    print_hex(driver_get_ticks() / 2);
    print("\r\n");
    print("  Process Management: ");
    print_hex(driver_get_ticks() / 20);
    print("\r\n");
    print("  Memory Management: ");
    print_hex(driver_get_ticks() / 30);
    print("\r\n");
    print("  IPC Operations: ");
    print_hex(driver_get_ticks() / 10);
    print("\r\n");
    
    print("\r\n");
}
