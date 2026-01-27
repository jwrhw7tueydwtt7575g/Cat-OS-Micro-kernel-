// Kernel Interrupt Handler
// Handles all hardware and software interrupts

#include "kernel.h"
#include "hal.h"
#include <stddef.h>

// IDT setup function
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

// External assembly handlers (defined in assembly at bottom of file)
extern void divide_error_handler(void);
extern void debug_exception_handler(void);
extern void nmi_handler(void);
extern void breakpoint_handler(void);
extern void overflow_handler(void);
extern void bound_range_exceeded_handler(void);
extern void invalid_opcode_handler(void);
extern void device_not_available_handler(void);
extern void double_fault_handler(void);
extern void invalid_tss_handler(void);
extern void segment_not_present_handler(void);
extern void stack_segment_fault_handler(void);
extern void general_protection_fault_handler(void);
extern void page_fault_handler(void);
extern void x87_fpu_error_handler(void);
extern void alignment_check_handler(void);
extern void machine_check_handler(void);
extern void simd_floating_point_handler(void);
extern void timer_irq_handler(void);
extern void keyboard_irq_handler(void);
extern void syscall_handler_wrapper(void);

// Interrupt Descriptor Table (IDT)
struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// IDT structures
static struct idt_entry idt[256];
static struct idt_ptr idt_ptr;

// Initialize interrupt system
void interrupt_init(void) {
    // Clear IDT
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
    // Set up exception handlers
    idt_set_gate(0, (uint32_t)divide_error_handler, 0x08, 0x8E);
    idt_set_gate(1, (uint32_t)debug_exception_handler, 0x08, 0x8E);
    idt_set_gate(2, (uint32_t)nmi_handler, 0x08, 0x8E);
    idt_set_gate(3, (uint32_t)breakpoint_handler, 0x08, 0x8E);
    idt_set_gate(4, (uint32_t)overflow_handler, 0x08, 0x8E);
    idt_set_gate(5, (uint32_t)bound_range_exceeded_handler, 0x08, 0x8E);
    idt_set_gate(6, (uint32_t)invalid_opcode_handler, 0x08, 0x8E);
    idt_set_gate(7, (uint32_t)device_not_available_handler, 0x08, 0x8E);
    idt_set_gate(8, (uint32_t)double_fault_handler, 0x08, 0x8E);
    idt_set_gate(10, (uint32_t)invalid_tss_handler, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)segment_not_present_handler, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)stack_segment_fault_handler, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)general_protection_fault_handler, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)page_fault_handler, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)x87_fpu_error_handler, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)alignment_check_handler, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)machine_check_handler, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)simd_floating_point_handler, 0x08, 0x8E);
    
    // Set up IRQ handlers
    idt_set_gate(32, (uint32_t)timer_irq_handler, 0x08, 0x8E);  // IRQ0
    idt_set_gate(33, (uint32_t)keyboard_irq_handler, 0x08, 0x8E); // IRQ1
    
    // Set up system call handler
    idt_set_gate(0x80, (uint32_t)syscall_handler_wrapper, 0x08, 0x8E);
    
    // Set up IDT pointer
    idt_ptr.limit = (sizeof(struct idt_entry) * 256) - 1;
    idt_ptr.base = (uint32_t)&idt;
    
    // Load IDT
    __asm__ volatile("lidt %0" : : "m"(idt_ptr));
    
    // Enable interrupts
    hal_cpu_enable_interrupts();
    
    kernel_print("Interrupt system initialized\r\n");
}

// Set IDT gate
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].type_attr = flags;
}

// Common interrupt handler
void interrupt_handler_common(uint32_t interrupt_number, uint32_t error_code) {
    // Handle different interrupt types
    if (interrupt_number < 32) {
        // CPU exception
        kernel_print("CPU Exception ");
        kernel_print_hex(interrupt_number);
        kernel_print(" Error code: ");
        kernel_print_hex(error_code);
        kernel_print("\r\n");
        
        // Get faulting address if page fault
        if (interrupt_number == 14) {
            uint32_t fault_addr = hal_cpu_get_cr2();
            kernel_print("Page fault at address: ");
            kernel_print_hex(fault_addr);
            kernel_print("\r\n");
        }
        
        // Kill current process
        pcb_t* current = scheduler_get_current();
        if (current) {
            process_exit(current, interrupt_number);
        } else {
            kernel_panic("Unhandled CPU exception in kernel");
        }
    } else if (interrupt_number >= 32 && interrupt_number < 48) {
        // Hardware interrupt (IRQ)
        uint32_t irq = interrupt_number - 32;
        
        switch (irq) {
            case 0:  // Timer
                timer_interrupt_handler();
                break;
            case 1:  // Keyboard
                keyboard_interrupt_handler();
                break;
            default:
                kernel_print("Unhandled IRQ ");
                kernel_print_hex(irq);
                kernel_print("\r\n");
                break;
        }
        
        // Send EOI to PIC
        hal_pic_send_eoi(irq);
    } else if (interrupt_number == 0x80) {
        // System call - handled in assembly wrapper
        return;
    } else {
        kernel_print("Unknown interrupt ");
        kernel_print_hex(interrupt_number);
        kernel_print("\r\n");
    }
}

// Timer interrupt handler
void timer_interrupt_handler(void) {
    // Call HAL timer handler
    extern void hal_timer_interrupt_handler(void);
    hal_timer_interrupt_handler();
}

// Keyboard interrupt handler
void keyboard_interrupt_handler(void) {
    // Read scancode from keyboard
    uint8_t scancode = hal_inb(PORT_KEYBOARD_DATA);
    
    // Send to keyboard driver via IPC
    size_t payload_size = sizeof(uint8_t);
    ipc_message_t* msg = (ipc_message_t*)memory_alloc_pages(
        (sizeof(ipc_message_t) + payload_size + PAGE_SIZE - 1) / PAGE_SIZE
    );
    if (msg) {
        msg->msg_type = MSG_DRIVER;
        msg->sender_pid = 0;  // Kernel
        msg->receiver_pid = 2;  // Keyboard driver PID
        msg->data_size = payload_size;
        msg->next = NULL;
        *(uint8_t*)msg->data = scancode;
        ipc_send(2, msg);
    }
}

// Assembly interrupt handlers
__asm__ (
"divide_error_handler:\n"
    "push $0\n"
    "push $0\n"
    "jmp interrupt_common\n"

"debug_exception_handler:\n"
    "push $0\n"
    "push $1\n"
    "jmp interrupt_common\n"

"nmi_handler:\n"
    "push $0\n"
    "push $2\n"
    "jmp interrupt_common\n"

"breakpoint_handler:\n"
    "push $0\n"
    "push $3\n"
    "jmp interrupt_common\n"

"overflow_handler:\n"
    "push $0\n"
    "push $4\n"
    "jmp interrupt_common\n"

"bound_range_exceeded_handler:\n"
    "push $0\n"
    "push $5\n"
    "jmp interrupt_common\n"

"invalid_opcode_handler:\n"
    "push $0\n"
    "push $6\n"
    "jmp interrupt_common\n"

"device_not_available_handler:\n"
    "push $0\n"
    "push $7\n"
    "jmp interrupt_common\n"

"double_fault_handler:\n"
    "push $8\n"
    "push $8\n"
    "jmp interrupt_common\n"

"invalid_tss_handler:\n"
    "push $10\n"
    "push $10\n"
    "jmp interrupt_common\n"

"segment_not_present_handler:\n"
    "push $11\n"
    "push $11\n"
    "jmp interrupt_common\n"

"stack_segment_fault_handler:\n"
    "push $12\n"
    "push $12\n"
    "jmp interrupt_common\n"

"general_protection_fault_handler:\n"
    "push $13\n"
    "push $13\n"
    "jmp interrupt_common\n"

"page_fault_handler:\n"
    "push $14\n"
    "push $14\n"
    "jmp interrupt_common\n"

"x87_fpu_error_handler:\n"
    "push $0\n"
    "push $16\n"
    "jmp interrupt_common\n"

"alignment_check_handler:\n"
    "push $17\n"
    "push $17\n"
    "jmp interrupt_common\n"

"machine_check_handler:\n"
    "push $18\n"
    "push $18\n"
    "jmp interrupt_common\n"

"simd_floating_point_handler:\n"
    "push $0\n"
    "push $19\n"
    "jmp interrupt_common\n"

"timer_irq_handler:\n"
    "push $0\n"
    "push $32\n"
    "jmp interrupt_common\n"

"keyboard_irq_handler:\n"
    "push $0\n"
    "push $33\n"
    "jmp interrupt_common\n"

"interrupt_common:\n"
    "pusha\n"
    "push %ds\n"
    "push %es\n"
    "push %fs\n"
    "push %gs\n"
    "mov $0x10, %ax\n"
    "mov %ax, %ds\n"
    "mov %ax, %es\n"
    "mov %ax, %fs\n"
    "mov %ax, %gs\n"
    "mov 8(%esp), %eax\n"
    "mov 12(%esp), %edx\n"
    "push %edx\n"
    "push %eax\n"
    "call interrupt_handler_common\n"
    "add $8, %esp\n"
    "pop %gs\n"
    "pop %fs\n"
    "pop %es\n"
    "pop %ds\n"
    "popa\n"
    "add $8, %esp\n"
    "iret\n"

"syscall_handler_wrapper:\n"
    "push %ds\n"
    "push %es\n"
    "push %fs\n"
    "push %gs\n"
    "mov $0x10, %ax\n"
    "mov %ax, %ds\n"
    "mov %ax, %es\n"
    "mov %ax, %fs\n"
    "mov %ax, %gs\n"
    "push %edx\n"
    "push %ecx\n"
    "push %ebx\n"
    "push %eax\n"
    "call syscall_handler\n"
    "add $16, %esp\n"
    "pop %gs\n"
    "pop %fs\n"
    "pop %es\n"
    "pop %ds\n"
    "iret\n"
);
