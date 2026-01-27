// Keyboard Driver
// User-space keyboard input driver

#include "driver.h"
#include "kernel.h"
#include "hal.h"
#include <stddef.h>

// Forward declarations
static status_t ipc_send_stub(uint32_t receiver_pid, ipc_abi_message_t* msg);

// Keyboard state
static bool keyboard_initialized = false;
static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;
static uint8_t keyboard_buffer[256];
static uint32_t keyboard_head = 0;
static uint32_t keyboard_tail = 0;
static uint32_t keyboard_pid = 0;

// Scancode to ASCII conversion table (US layout)
static const uint8_t scancode_to_ascii[128] = {
    // 0x00-0x0F
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    // 0x10-0x1F
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 0,
    // 0x20-0x2F
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z',
    // 0x30-0x3F
    'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, 0, 0, 0, 0, 0, 0,
    // 0x40-0x4F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0x50-0x5F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0x60-0x6F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0x70-0x7F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Scancode to ASCII conversion table (shifted)
static const uint8_t scancode_to_ascii_shift[128] = {
    // 0x00-0x0F
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t',
    // 0x10-0x1F
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0, 0,
    // 0x20-0x2F
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z',
    // 0x30-0x3F
    'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, 0, 0, 0, 0, 0, 0,
    // 0x40-0x4F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0x50-0x5F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0x60-0x6F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0x70-0x7F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Forward declarations
static void keyboard_handle_scancode(uint8_t scancode);
static uint8_t scancode_to_ascii_convert(uint8_t scancode);

// Keyboard driver interface
static driver_interface_t keyboard_driver = {
    .name = "keyboard",
    .driver_id = 2,
    .capabilities = CAP_DRIVER_READ,
    .init = keyboard_driver_init,
    .cleanup = keyboard_driver_init,  // Use same function for now
    .shutdown = NULL,  // No special shutdown needed
    .handle_message = keyboard_driver_handle_message
};

// Initialize keyboard driver
status_t keyboard_driver_init(void) {
    if (keyboard_initialized) {
        return STATUS_SUCCESS;
    }
    
    // Clear buffer
    keyboard_head = 0;
    keyboard_tail = 0;
    
    // Reset state
    shift_pressed = false;
    ctrl_pressed = false;
    alt_pressed = false;
    
    // Get driver PID
    keyboard_pid = 2;  // Fixed PID for keyboard driver
    
    // Register with kernel
    status_t result = driver_register(&keyboard_driver);
    if (result != STATUS_SUCCESS) {
        return result;
    }
    
    keyboard_initialized = true;
    driver_print("Keyboard driver initialized\r\n");
    
    return STATUS_SUCCESS;
}

// Shutdown keyboard driver
void keyboard_driver_cleanup(void) {
    if (!keyboard_initialized) {
        return;
    }
    
    // Clear buffer
    keyboard_head = 0;
    keyboard_tail = 0;
    
    // Reset state
    shift_pressed = false;
    ctrl_pressed = false;
    alt_pressed = false;
    
    keyboard_initialized = false;
    driver_print("Keyboard driver shutdown\r\n");
}

// Handle driver messages
status_t keyboard_driver_handle_message(ipc_abi_message_t* msg) {
    if (!msg || !keyboard_initialized) {
        return STATUS_INVALID_PARAM;
    }
    
    switch (msg->msg_type) {
        case MSG_DRIVER:
            if (msg->data_size >= sizeof(uint8_t)) {
                uint8_t scancode = *(uint8_t*)msg->data;
                keyboard_handle_scancode(scancode);
            }
            break;
            
        case DRIVER_MSG_READ:
            // Handle read request from application
            if (keyboard_head != keyboard_tail) {
                uint8_t ascii = keyboard_buffer[keyboard_tail];
                keyboard_tail = (keyboard_tail + 1) % 256;
                
                // Send character back to application
                ipc_abi_message_t response = {0};
                response.msg_type = DRIVER_MSG_READ;
                response.data_size = sizeof(uint8_t);
                *(uint8_t*)response.data = ascii;
                ipc_send_stub(msg->sender_pid, &response);
            }
            break;
            
        default:
            return STATUS_INVALID_PARAM;
    }
    
    return STATUS_SUCCESS;
}

// Convert scancode to ASCII
static uint8_t scancode_to_ascii_convert(uint8_t scancode) {
    if (scancode >= 128) {
        return 0;
    }
    
    if (shift_pressed) {
        return scancode_to_ascii_shift[scancode];
    } else {
        return scancode_to_ascii[scancode];
    }
}

// Add character to buffer
static void keyboard_add_to_buffer(uint8_t ascii) {
    if (ascii == 0) {
        return;
    }
    
    // Add to circular buffer
    keyboard_buffer[keyboard_head] = ascii;
    keyboard_head = (keyboard_head + 1) % 256;
    
    // If buffer is full, overwrite oldest character
    if (keyboard_head == keyboard_tail) {
        keyboard_tail = (keyboard_tail + 1) % 256;
    }
}

// Handle keyboard scancode
static void keyboard_handle_scancode(uint8_t scancode) {
    // Handle key release (high bit set)
    if (scancode & 0x80) {
        scancode &= 0x7F;
        
        switch (scancode) {
            case 0x2A:  // Left shift
            case 0x36:  // Right shift
                shift_pressed = false;
                break;
            case 0x1D:  // Ctrl
                ctrl_pressed = false;
                break;
            case 0x38:  // Alt
                alt_pressed = false;
                break;
        }
        return;
    }
    
    // Handle key press
    switch (scancode) {
        case 0x2A:  // Left shift
        case 0x36:  // Right shift
            shift_pressed = true;
            break;
        case 0x1D:  // Ctrl
            ctrl_pressed = true;
            break;
        case 0x38:  // Alt
            alt_pressed = true;
            break;
        default:
            // Convert to ASCII and add to buffer
            uint8_t ascii = scancode_to_ascii_convert(scancode);
            if (ascii != 0) {
                keyboard_add_to_buffer(ascii);
                
                // Echo to console for debugging
                driver_print("Key: ");
                char ch = ascii;
                if (ch >= 32 && ch <= 126) {
                    char str[2] = {ch, 0};
                    driver_print(str);
                } else {
                    driver_print_hex(ascii);
                }
                driver_print("\r\n");
            }
            break;
    }
}

// Driver print function
void driver_print(const char* str) {
    // Simple VGA output for debugging
    volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
    static uint32_t pos = 0;
    
    while (*str) {
        if (*str == '\n') {
            pos = (pos / 80 + 1) * 80;
        } else if (*str == '\r') {
            pos = (pos / 80) * 80;
        } else if (*str == '\b') {
            if (pos > 0) pos--;
            vga[pos] = 0x0700 | ' ';
        } else if (*str == '\t') {
            pos = (pos + 8) & ~7;
        } else {
            vga[pos++] = 0x0700 | *str;
        }
        str++;
        
        // Simple screen wrapping
        if (pos >= 80 * 25) {
            pos = 0;
        }
    }
}

// Stub for ipc_send (drivers can't directly call kernel IPC)
static status_t ipc_send_stub(uint32_t receiver_pid, ipc_abi_message_t* msg) {
    (void)receiver_pid;
    (void)msg;
    return STATUS_SUCCESS;  // Stub implementation
}

// Driver print hex function
void driver_print_hex(uint32_t value) {
    const char hex_chars[] = "0123456789ABCDEF";
    char buffer[11] = "0x00000000";
    
    for (int i = 9; i >= 2; i--) {
        buffer[i] = hex_chars[value & 0x0F];
        value >>= 4;
    }
    
    driver_print(buffer);
}

// Driver main function
int main(void) {
    // Initialize keyboard driver
    status_t result = keyboard_driver_init();
    if (result != STATUS_SUCCESS) {
        return -1;
    }
    
    // Simple driver loop
    while (keyboard_initialized) {
        // In a real implementation, this would handle interrupts
        // For now, just halt
        __asm__ volatile("hlt");
    }
    
    return 0;
}
