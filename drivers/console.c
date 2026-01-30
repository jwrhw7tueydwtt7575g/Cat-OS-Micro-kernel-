// Console Driver
// User-space console output driver

#include "driver.h"
#include "userspace.h"
#include "hal.h"
#include <stddef.h>

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

// Forward declarations
static void console_scroll_up(void);
static void console_put_char(char c);
static void console_move_cursor(uint32_t x, uint32_t y);
static void console_clear_line(uint32_t y);
status_t console_driver_handle_message(ipc_abi_message_t* msg);

// Console driver interface
static driver_interface_t console_driver = {
    .name = "console",
    .driver_id = 3,
    .capabilities = CAP_DRIVER_WRITE,
    .init = console_driver_init,
    .cleanup = console_driver_shutdown, 
    .shutdown = NULL,
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
    
    status_t result = driver_register(&console_driver);
    if (result != STATUS_SUCCESS) {
        return result;
    }

    // Also register with kernel via syscall
    driver_register_wrapper(console_driver.name, console_driver.capabilities);
    
    console_initialized = true;
    driver_print("Console driver initialized\r\n");
    
    return STATUS_SUCCESS;
}

status_t console_driver_shutdown(void) {
    if (!console_initialized) {
        return STATUS_SUCCESS;
    }
    
    // Clear screen
    for (uint32_t i = 0; i < VGA_SIZE; i++) {
        vga_memory[i] = 0x0720;  // Light gray on black, space
    }
    
    driver_unregister(console_driver.driver_id);
    console_initialized = false;
    return STATUS_SUCCESS;
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
                for (uint32_t i = 0; i < msg->data_size - 1; i++) {
                    console_put_char(text[i]);
                }
            }
            break;
            
        case DRIVER_MSG_IOCTL:
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
    for (uint32_t i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
        vga_memory[i] = vga_memory[i + VGA_WIDTH];
    }
    console_clear_line(VGA_HEIGHT - 1);
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
    hal_outb(0x3D4, 0x0F);
    hal_outb(0x3D5, pos & 0xFF);
    hal_outb(0x3D4, 0x0E);
    hal_outb(0x3D5, (pos >> 8) & 0xFF);
}

// Put character on screen
static void console_put_char(char c) {
    // Mirror to serial port for debugging/automation
    hal_outb(0x3F8, (uint8_t)c);
    
    switch (c) {
        case '\r': console_x = 0; break;
        case '\n': 
            console_x = 0; 
            console_y++; 
            if (console_y >= VGA_HEIGHT) console_scroll_up(); 
            break;
        case '\t':
            for (uint32_t i = 0; i < 4; i++) console_put_char(' ');
            break;
        case '\b':
            if (console_x > 0) {
                console_x--;
                vga_memory[console_y * VGA_WIDTH + console_x] = (console_color << 8) | ' ';
            }
            break;
        default:
            if (c >= 32 && c <= 126) {
                vga_memory[console_y * VGA_WIDTH + console_x] = (console_color << 8) | c;
                console_x++;
                if (console_x >= VGA_WIDTH) {
                    console_x = 0;
                    console_y++;
                    if (console_y >= VGA_HEIGHT) console_scroll_up();
                }
            }
            break;
    }
    console_move_cursor(console_x, console_y);
}

void driver_print(const char* str) {
    if (!str || !console_initialized) return;
    while (*str) {
        console_put_char(*str++);
    }
}

int main(void);
void _start(void) __attribute__((section(".text.entry")));
void _start(void) {
    main();
    while(1) { process_yield(); }
}

// Main entry point
int main(void) {
    if (console_driver_init() != STATUS_SUCCESS) {
        return 1;
    }

    uint8_t buffer[sizeof(ipc_abi_message_t) + 1024]; 
    ipc_abi_message_t* msg_ptr = (ipc_abi_message_t*)buffer;

    while (1) {
        if (ipc_receive(0, msg_ptr, true) == STATUS_SUCCESS) {
            console_driver_handle_message(msg_ptr);
        }
    }
    return 0;
}
