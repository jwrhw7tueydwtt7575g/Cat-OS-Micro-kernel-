// HAL Timer Module
// Provides timer services and interrupt handling

#include "hal.h"
#include "types.h"

// Timer configuration
static uint32_t timer_frequency = 100;  // Default 100 Hz
static uint32_t timer_ticks = 0;

// PIT (Programmable Interval Timer) constants
#define PIT_FREQUENCY   1193180
#define TIMER_IRQ       0

// Initialize timer
void hal_timer_init(uint32_t frequency) {
    timer_frequency = frequency;
    
    // Set timer frequency
    hal_timer_set_frequency(frequency);
    
    // Enable timer interrupt
    hal_timer_enable_irq();
}

// Set timer frequency
void hal_timer_set_frequency(uint32_t hz) {
    uint32_t divisor = PIT_FREQUENCY / hz;
    
    // Command byte: channel 0, square wave, access both, 16-bit
    hal_outb(PORT_TIMER_CMD, 0x36);
    
    // Send divisor
    hal_outb(PORT_TIMER_DATA, divisor & 0xFF);        // Low byte
    hal_outb(PORT_TIMER_DATA, (divisor >> 8) & 0xFF); // High byte
    
    timer_frequency = hz;
}

// Get current tick count
uint32_t hal_timer_get_ticks(void) {
    return timer_ticks;
}

// Busy-wait delay (not recommended, use sleep instead)
void hal_timer_delay_ms(uint32_t ms) {
    uint32_t start_ticks = timer_ticks;
    uint32_t delay_ticks = (ms * timer_frequency) / 1000;
    
    while ((timer_ticks - start_ticks) < delay_ticks) {
        hal_cpu_halt();
    }
}

// Enable timer interrupt
void hal_timer_enable_irq(void) {
    hal_pic_unmask_irq(TIMER_IRQ);
}

// Disable timer interrupt
void hal_timer_disable_irq(void) {
    hal_pic_mask_irq(TIMER_IRQ);
}

// Timer interrupt handler (called from interrupt handler)
void hal_timer_interrupt_handler(void) {
    timer_ticks++;
    
    // Call scheduler (this will be implemented in kernel)
    extern void scheduler_tick(void);
    scheduler_tick();
}

// Get timer frequency
uint32_t hal_timer_get_frequency(void) {
    return timer_frequency;
}

// Reset tick counter
void hal_timer_reset_ticks(void) {
    timer_ticks = 0;
}

// Wait for specific tick count
void hal_timer_wait_ticks(uint32_t ticks) {
    uint32_t target = timer_ticks + ticks;
    
    while (timer_ticks < target) {
        hal_cpu_halt();
    }
}

// Get milliseconds since boot
uint32_t hal_timer_get_ms(void) {
    return (timer_ticks * 1000) / timer_frequency;
}

// Get seconds since boot
uint32_t hal_timer_get_seconds(void) {
    return timer_ticks / timer_frequency;
}
