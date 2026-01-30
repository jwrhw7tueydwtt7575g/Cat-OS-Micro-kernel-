// Init process with debug print
#include "stdint.h"

void _start(void) __attribute__((section(".text.entry")));

static void debug_print(const char* s) {
    __asm__ volatile ("int $0x80" : : "a"(0x41), "b"(s));
}

void _start(void) {
    debug_print("PID 1 (Init) is alive and running in user mode!\r\n");
    
    while (1) {
        // Yield to let other processes run
        __asm__ volatile ("int $0x80" : : "a"(0x03));
    }
}
