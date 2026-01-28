// Simple Test Kernel for Cat-OS
// Just prints to VGA to verify protected mode is working

#include <stdint.h>

// VGA text mode memory
#define VGA_MEMORY 0xB8000
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

// Colors
#define VGA_COLOR_BLACK 0
#define VGA_COLOR_WHITE 15

// Simple kernel entry point
void kernel_main(void) {
    // Clear screen
    volatile uint16_t* vga = (volatile uint16_t*)VGA_MEMORY;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = (VGA_COLOR_BLACK << 8) | ' ';
    }
    
    // Print success message
    const char* msg = "Cat-OS Kernel Running in Protected Mode!";
    int pos = 0;
    
    while (msg[pos] != 0) {
        vga[pos] = (VGA_COLOR_WHITE << 8) | msg[pos];
        pos++;
    }
    
    // Print second line
    const char* msg2 = "Microkernel Successfully Loaded!";
    pos = 80;  // Second line
    
    while (msg2[pos - 80] != 0) {
        vga[pos] = (VGA_COLOR_WHITE << 8) | msg2[pos - 80];
        pos++;
    }
    
    // Halt the system
    while (1) {
        __asm__ volatile("hlt");
    }
}
