// Keyboard Driver
// User-space keyboard input driver

#include "driver.h"
#include "userspace.h"
#include "hal.h"
#include <stddef.h>

// Keyboard state
static bool keyboard_initialized = false;
static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;
static uint8_t keyboard_buffer[256];
static uint32_t keyboard_head = 0;
static uint32_t keyboard_tail = 0;

// Scancode to ASCII conversion table (US layout)
static const uint8_t scancode_to_ascii[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const uint8_t scancode_to_ascii_shift[128] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0, 'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Forward declarations
static void keyboard_handle_scancode(uint8_t scancode);
static uint8_t scancode_to_ascii_convert(uint8_t scancode);
status_t keyboard_driver_handle_message(ipc_abi_message_t* msg);

// Keyboard driver interface
static driver_interface_t keyboard_driver = {
    .name = "keyboard",
    .driver_id = 2,
    .capabilities = CAP_DRIVER_READ,
    .init = keyboard_driver_init,
    .cleanup = keyboard_driver_shutdown,
    .shutdown = NULL,
    .handle_message = keyboard_driver_handle_message
};

status_t keyboard_driver_init(void) {
    if (keyboard_initialized) return STATUS_SUCCESS;
    keyboard_head = 0; keyboard_tail = 0;
    shift_pressed = false; ctrl_pressed = false; alt_pressed = false;
    driver_register(&keyboard_driver);
    driver_register_wrapper(keyboard_driver.name, keyboard_driver.capabilities);
    keyboard_initialized = true;
    return STATUS_SUCCESS;
}

status_t keyboard_driver_shutdown(void) {
    if (!keyboard_initialized) return STATUS_SUCCESS;
    driver_unregister(keyboard_driver.driver_id);
    keyboard_initialized = false;
    return STATUS_SUCCESS;
}

status_t keyboard_driver_handle_message(ipc_abi_message_t* msg) {
    if (!msg || !keyboard_initialized) return STATUS_INVALID_PARAM;
    switch (msg->msg_type) {
        case MSG_DRIVER:
            if (msg->data_size >= sizeof(uint8_t)) keyboard_handle_scancode(*(uint8_t*)msg->data);
            break;
        case DRIVER_MSG_READ:
            if (keyboard_head != keyboard_tail) {
                uint8_t ascii = keyboard_buffer[keyboard_tail];
                keyboard_tail = (keyboard_tail + 1) % 256;
                ipc_abi_message_t response = {0};
                response.msg_type = DRIVER_MSG_READ;
                response.data_size = sizeof(uint8_t);
                *(uint8_t*)response.data = ascii;
                ipc_send(msg->sender_pid, &response);
            }
            break;
        default: return STATUS_INVALID_PARAM;
    }
    return STATUS_SUCCESS;
}

static uint8_t scancode_to_ascii_convert(uint8_t scancode) {
    if (scancode >= 128) return 0;
    return shift_pressed ? scancode_to_ascii_shift[scancode] : scancode_to_ascii[scancode];
}

static void keyboard_handle_scancode(uint8_t scancode) {
    if (scancode & 0x80) {
        scancode &= 0x7F;
        if (scancode == 0x2A || scancode == 0x36) shift_pressed = false;
        if (scancode == 0x1D) ctrl_pressed = false;
        if (scancode == 0x38) alt_pressed = false;
        return;
    }
    if (scancode == 0x2A || scancode == 0x36) { shift_pressed = true; return; }
    if (scancode == 0x1D) { ctrl_pressed = true; return; }
    if (scancode == 0x38) { alt_pressed = true; return; }
    uint8_t ascii = scancode_to_ascii_convert(scancode);
    if (ascii != 0) {
        keyboard_buffer[keyboard_head] = ascii;
        keyboard_head = (keyboard_head + 1) % 256;
        if (keyboard_head == keyboard_tail) keyboard_tail = (keyboard_tail + 1) % 256;
    }
}

void driver_print(const char* str) { print(str); }

int main(void);
void _start(void) __attribute__((section(".text.entry")));
void _start(void) {
    main();
    while(1) { process_yield(); }
}

int main(void) {
    if (keyboard_driver_init() != STATUS_SUCCESS) return 1;
    uint8_t buffer[sizeof(ipc_abi_message_t) + 128];
    ipc_abi_message_t* msg_ptr = (ipc_abi_message_t*)buffer;
    while (1) {
        // Poll Serial Port for input (Automation support)
        if (hal_inb(0x3F8 + 5) & 0x01) {
            uint8_t c = hal_inb(0x3F8);
            if (c == '\r') c = '\n'; // Convert CR to LF for shell
            keyboard_buffer[keyboard_head] = c;
            keyboard_head = (keyboard_head + 1) % 256;
            if (keyboard_head == keyboard_tail) keyboard_tail = (keyboard_tail + 1) % 256;
        }
        if (ipc_receive(0, msg_ptr, false) == STATUS_SUCCESS) {
            keyboard_driver_handle_message(msg_ptr);
        } else {
            process_yield();
        }
    }
}
