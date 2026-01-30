#ifndef HAL_H
#define HAL_H

#include <stdint.h>

// CPU Control functions
void hal_cpu_init(void);
uint32_t hal_cpu_get_features(void);
void hal_cpu_enable_paging(uint32_t page_dir);
void hal_cpu_set_cr3(uint32_t page_dir);
void hal_cpu_halt(void);
uint32_t hal_cpu_get_cr0(void);
uint32_t hal_cpu_get_cr2(void);
uint32_t hal_cpu_get_cr3(void);
void hal_cpu_flush_tlb(void);
uint64_t hal_cpu_get_cycles(void);
void hal_cpu_enable_interrupts(void);
void hal_cpu_disable_interrupts(void);

// I/O Port functions
void hal_io_init(void);
static inline void hal_outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t hal_inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void hal_outw(uint16_t port, uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t hal_inw(uint16_t port) {
    uint16_t value;
    __asm__ volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// Timer functions
void hal_timer_init(uint32_t frequency);
void hal_timer_set_frequency(uint32_t hz);
uint32_t hal_timer_get_ticks(void);
void hal_timer_delay_ms(uint32_t ms);
void hal_timer_enable_irq(void);

// PIC functions
void hal_pic_init(void);
void hal_pic_mask_irq(uint8_t irq);
void hal_pic_unmask_irq(uint8_t irq);
void hal_pic_send_eoi(uint8_t irq);
void hal_pic_remap(uint8_t offset1, uint8_t offset2);

// Interrupt handling
void hal_interrupt_init(void);
void hal_interrupt_enable(void);
void hal_interrupt_disable(void);

// Port definitions
#define PORT_PIC_MASTER_CMD  0x20
#define PORT_PIC_MASTER_DATA  0x21
#define PORT_PIC_SLAVE_CMD   0xA0
#define PORT_PIC_SLAVE_DATA  0xA1
#define PORT_TIMER_DATA      0x40
#define PORT_TIMER_CMD       0x43
#define PORT_KEYBOARD_DATA   0x60
#define PORT_KEYBOARD_STATUS 0x64

// CPU registers
#define CR0_PE 0x01        // Protected mode enable
#define CR0_PG 0x80000000  // Paging enable

// GDT functions
void hal_gdt_init(void);
void hal_tss_set_esp0(uint32_t esp0);

#endif // HAL_H
