// HAL PIC Module
// Programmable Interrupt Controller management

#include "hal.h"
#include "types.h"

// PIC constants
#define PIC_EOI     0x20
#define PIC_INIT    0x11
#define PIC_ICW4_8086 0x01

// Initialize PIC controllers
void hal_pic_init(void) {
    // Remap PIC interrupts (default is 0x08-0x0F, we want 0x20-0x2F)
    hal_pic_remap(0x20, 0x28);
    
    // Mask all interrupts initially
    hal_outb(PORT_PIC_MASTER_DATA, 0xFF);
    hal_outb(PORT_PIC_SLAVE_DATA, 0xFF);
}

// Remap PIC interrupt vectors
void hal_pic_remap(uint8_t offset1, uint8_t offset2) {
    uint8_t a1, a2;
    
    // Save current masks
    a1 = hal_inb(PORT_PIC_MASTER_DATA);
    a2 = hal_inb(PORT_PIC_SLAVE_DATA);
    
    // Start initialization sequence
    hal_outb(PORT_PIC_MASTER_CMD, PIC_INIT);
    hal_outb(PORT_PIC_SLAVE_CMD, PIC_INIT);
    
    // Set offset vectors
    hal_outb(PORT_PIC_MASTER_DATA, offset1);
    hal_outb(PORT_PIC_SLAVE_DATA, offset2);
    
    // Configure cascade
    hal_outb(PORT_PIC_MASTER_DATA, 4);   // Master PIC: IRQ2 is cascade
    hal_outb(PORT_PIC_SLAVE_DATA, 2);    // Slave PIC: cascade identity
    
    // Set 8086 mode
    hal_outb(PORT_PIC_MASTER_DATA, PIC_ICW4_8086);
    hal_outb(PORT_PIC_SLAVE_DATA, PIC_ICW4_8086);
    
    // Restore masks
    hal_outb(PORT_PIC_MASTER_DATA, a1);
    hal_outb(PORT_PIC_SLAVE_DATA, a2);
}

// Send End-Of-Interrupt signal
void hal_pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        // Send EOI to slave PIC
        hal_outb(PORT_PIC_SLAVE_CMD, PIC_EOI);
    }
    
    // Always send EOI to master PIC
    hal_outb(PORT_PIC_MASTER_CMD, PIC_EOI);
}

// Mask specific IRQ line
void hal_pic_mask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PORT_PIC_MASTER_DATA;
    } else {
        port = PORT_PIC_SLAVE_DATA;
        irq -= 8;
    }
    
    value = hal_inb(port) | (1 << irq);
    hal_outb(port, value);
}

// Unmask specific IRQ line
void hal_pic_unmask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PORT_PIC_MASTER_DATA;
    } else {
        port = PORT_PIC_SLAVE_DATA;
        irq -= 8;
    }
    
    value = hal_inb(port) & ~(1 << irq);
    hal_outb(port, value);
}

// Get IRQ mask
uint16_t hal_pic_get_irq_mask(void) {
    uint16_t mask;
    mask = hal_inb(PORT_PIC_SLAVE_DATA) << 8;
    mask |= hal_inb(PORT_PIC_MASTER_DATA);
    return mask;
}

// Set IRQ mask
void hal_pic_set_irq_mask(uint16_t mask) {
    hal_outb(PORT_PIC_MASTER_DATA, mask & 0xFF);
    hal_outb(PORT_PIC_SLAVE_DATA, (mask >> 8) & 0xFF);
}

// Check if IRQ is spurious
bool hal_pic_is_spurious_irq(uint8_t irq) {
    uint16_t isr;
    
    // Read ISR register
    hal_outb(PORT_PIC_MASTER_CMD, 0x0B);
    isr = hal_inb(PORT_PIC_MASTER_CMD);
    
    if (irq >= 8) {
        hal_outb(PORT_PIC_SLAVE_CMD, 0x0B);
        isr |= hal_inb(PORT_PIC_SLAVE_CMD) << 8;
    }
    
    // Check if IRQ bit is set in ISR
    return !(isr & (1 << irq));
}

// Disable all interrupts
void hal_pic_disable_all(void) {
    hal_outb(PORT_PIC_MASTER_DATA, 0xFF);
    hal_outb(PORT_PIC_SLAVE_DATA, 0xFF);
}

// Enable all interrupts
void hal_pic_enable_all(void) {
    hal_outb(PORT_PIC_MASTER_DATA, 0x00);
    hal_outb(PORT_PIC_SLAVE_DATA, 0x00);
}
