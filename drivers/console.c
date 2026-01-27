// Console Driver
// User-space console output driver

#include "driver.h"
#include "kernel.h"
#include "hal.h"
#include <stddef.h>

// Forward declarations
static status_t ipc_send_stub(uint32_t receiver_pid, ipc_abi_message_t* msg);
static uint32_t strlen(const char* str);

// Simple string function
static uint32_t strlen(const char* str) {
    uint32_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

// Stub for ipc_send (drivers can't directly call kernel IPC)
static status_t ipc_send_stub(uint32_t receiver_pid, ipc_abi_message_t* msg) {
    (void)receiver_pid;
    (void)msg;
    return STATUS_SUCCESS;  // Stub implementation
}

// Console state
static bool console_initialized = false;
static uint32_t console_pid = 0;
static uint16_t* vga_memory = (uint16_t*)0xB8000;
static uint32_t console_x = 0;
static uint32_t console_y = 0;
static uint8_t console_color = 0x07;  // Light gray on black

// VGA constants
#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_SIZE   (VGA_WIDTH * VGA_HEIGHT)

// Color constants
#define COLOR_BLACK         0x0
#define COLOR_BLUE          0x1
#define COLOR_GREEN         0x2
#define COLOR_CYAN          0x3
#define COLOR_RED           0x4
#define COLOR_MAGENTA       0x5
#define COLOR_BROWN         0x6
#define COLOR_LIGHT_GRAY    0x7
#define COLOR_DARK_GRAY     0x8
#define COLOR_LIGHT_BLUE    0x9
#define COLOR_LIGHT_GREEN   0xA
#define COLOR_LIGHT_CYAN    0xB
#define COLOR_LIGHT_RED     0xC
#define COLOR_LIGHT_MAGENTA 0xD
#define COLOR_YELLOW        0xE
#define COLOR_WHITE         0xF

// Forward declarations
static void console_scroll_up(void);
static void console_put_char(char c);
static void console_move_cursor(uint32_t x, uint32_t y);
static void console_clear_line(uint32_t y);

// Console driver interface
static driver_interface_t console_driver = {
    .name = "console",
    .driver_id = 3,
    .capabilities = CAP_DRIVER_WRITE,
    .init = console_driver_init,
    .cleanup = console_driver_init,  // Use same function for now
    .shutdown = NULL,  // No special shutdown needed
    .handle_message = console_driver_handle_message
};

// Initialize console driver
status_t console_driver_init(void) {
    if (console_initialized) {
        return STATUS_SUCCESS;
    }
    
    // Clear screen
    for (uint32_t i = 0; i < VGA_SIZE; i++) {
        vga_memory[i] = (console_color << 8) | ' ';
    }
    
    // Reset cursor position
    console_x = 0;
    console_y = 0;
    
    // Get driver PID
    console_pid = 3;  // Fixed PID for console driver
    
    // Register with kernel
    status_t result = driver_register(&console_driver);
    if (result != STATUS_SUCCESS) {
        return result;
    }
    
    console_initialized = true;
    driver_print("Console driver initialized\r\n");
    
    return STATUS_SUCCESS;
}

// Shutdown console driver
void console_driver_shutdown(void) {
    if (!console_initialized) {
        return;
    }
    
    // Clear screen
    for (uint32_t i = 0; i < VGA_SIZE; i++) {
        vga_memory[i] = 0x0720;  // Light gray on black, space
    }
    
    // Unregister from kernel
    driver_unregister(console_driver.driver_id);
    
    console_initialized = false;
}

// Handle driver messages
status_t console_driver_handle_message(ipc_abi_message_t* msg) {
    if (!msg || !console_initialized) {
        return STATUS_INVALID_PARAM;
    }
    
    switch (msg->msg_type) {
        case DRIVER_MSG_WRITE:
            if (msg->data_size > 0) {
                char* text = (char*)msg->data;
                for (uint32_t i = 0; i < msg->data_size - 1; i++) {  // -1 to exclude null terminator
                    console_put_char(text[i]);
                }
            }
            break;
            
        case DRIVER_MSG_IOCTL:
            // Handle console control commands
            if (msg->data_size >= sizeof(uint32_t)) {
                uint32_t command = *(uint32_t*)msg->data;
                switch (command) {
                    case 0x01:  // Clear screen
                        for (uint32_t i = 0; i < VGA_SIZE; i++) {
                            vga_memory[i] = (console_color << 8) | ' ';
                        }
                        console_x = 0;
                        console_y = 0;
                        break;
                        
                    case 0x02:  // Set color
                        if (msg->data_size >= 2 * sizeof(uint32_t)) {
                            uint32_t fg = ((uint32_t*)msg->data)[1];
                            uint32_t bg = ((uint32_t*)msg->data)[2];
                            console_color = (bg << 4) | (fg & 0x0F);
                        }
                        break;
                        
                    case 0x03:  // Set cursor position
                        if (msg->data_size >= 3 * sizeof(uint32_t)) {
                            uint32_t x = ((uint32_t*)msg->data)[1];
                            uint32_t y = ((uint32_t*)msg->data)[2];
                            if (x < VGA_WIDTH && y < VGA_HEIGHT) {
                                console_x = x;
                                console_y = y;
                                console_move_cursor(x, y);
                            }
                        }
                        break;
                }
            }
            break;
            
        default:
            return STATUS_INVALID_PARAM;
    }
    
    return STATUS_SUCCESS;
}

// Scroll console up one line
static void console_scroll_up(void) {
    // Move all lines up
    for (uint32_t i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
        vga_memory[i] = vga_memory[i + VGA_WIDTH];
    }
    
    // Clear last line
    console_clear_line(VGA_HEIGHT - 1);
    
    // Move cursor to last line
    console_y = VGA_HEIGHT - 1;
    console_x = 0;
}

// Clear a line
static void console_clear_line(uint32_t y) {
    uint16_t* line = &vga_memory[y * VGA_WIDTH];
    for (uint32_t x = 0; x < VGA_WIDTH; x++) {
        line[x] = (console_color << 8) | ' ';
    }
}

// Move cursor to position
static void console_move_cursor(uint32_t x, uint32_t y) {
    uint16_t pos = y * VGA_WIDTH + x;
    
    // Send cursor position to VGA controller
    hal_outb(0x3D4, 0x0F);
    hal_outb(0x3D5, pos & 0xFF);
    hal_outb(0x3D4, 0x0E);
    hal_outb(0x3D5, (pos >> 8) & 0xFF);
}

// Put character on screen
static void console_put_char(char c) {
    switch (c) {
        case '\r':
            // Carriage return
            console_x = 0;
            break;
            
        case '\n':
            // New line
            console_x = 0;
            console_y++;
            if (console_y >= VGA_HEIGHT) {
                console_scroll_up();
            }
            break;
            
        case '\t':
            // Tab (4 spaces)
            for (uint32_t i = 0; i < 4; i++) {
                console_put_char(' ');
            }
            break;
            
        case '\b':
            // Backspace
            if (console_x > 0) {
                console_x--;
                vga_memory[console_y * VGA_WIDTH + console_x] = (console_color << 8) | ' ';
            }
            break;
            
        default:
            // Regular character
            if (c >= 32 && c <= 126) {
                vga_memory[console_y * VGA_WIDTH + console_x] = (console_color << 8) | c;
                console_x++;
                
                if (console_x >= VGA_WIDTH) {
                    console_x = 0;
                    console_y++;
                    if (console_y >= VGA_HEIGHT) {
                        console_scroll_up();
                    }
                }
            }
            break;
    }
    
    // Update cursor position
    console_move_cursor(console_x, console_y);
}

// Driver print function (for other drivers)
void driver_print(const char* str) {
    if (!str || !console_initialized) {
        return;
    }
    
    // Create message to send to console
    ipc_abi_message_t msg = {0};
    msg.msg_type = DRIVER_MSG_WRITE;
    msg.data_size = strlen(str) + 1;
    
    // Copy string (simple implementation)
    for (uint32_t i = 0; i < msg.data_size - 1; i++) {
        msg.data[i] = str[i];
    }
    
    ipc_send_stub(console_pid, &msg);
}

// Driver print hex function
void driver_print_hex(uint32_t value) {
    const char hex_chars[] = "0123456789ABCDEF";
    char buffer[11] = "0x00000000";
    
    for (int i = 0; i < 8; i++) {
        buffer[9 - i] = hex_chars[value & 0xF];
        value >>= 4;
    }
    
    driver_print(buffer);
}

// Get console dimensions
void console_get_size(uint32_t* width, uint32_t* height) {
    if (width) *width = VGA_WIDTH;
    if (height) *height = VGA_HEIGHT;
}

// Get cursor position
void console_get_cursor(uint32_t* x, uint32_t* y) {
    if (x) *x = console_x;
    if (y) *y = console_y;
}

// Set text color
void console_set_color(uint8_t fg, uint8_t bg) {
    console_color = (bg << 4) | (fg & 0x0F);
}
