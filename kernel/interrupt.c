// Kernel Interrupt Handler
// Handles all hardware and software interrupts

#include "kernel.h"
#include "hal.h"
#include <stddef.h>

// Trap frame structure
typedef struct {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, user_esp, user_ss;
} trap_frame_t;

// IDT setup function
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

// External assembly handlers
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

// IDT structures
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

static struct idt_entry idt[256];
static struct idt_ptr idt_ptr;

// Initialize interrupt system
void interrupt_init(void) {
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
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
    
    idt_set_gate(32, (uint32_t)timer_irq_handler, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)keyboard_irq_handler, 0x08, 0x8E);
    
    // Syscall handler (DPL=3)
    idt_set_gate(0x80, (uint32_t)syscall_handler_wrapper, 0x08, 0xEE);
    
    idt_ptr.limit = (sizeof(struct idt_entry) * 256) - 1;
    idt_ptr.base = (uint32_t)&idt;
    
    __asm__ volatile("lidt %0" : : "m"(idt_ptr));
    kernel_print("Interrupt system initialized\r\n");
}

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].type_attr = flags;
}

void interrupt_handler_common(trap_frame_t* frame) {
    if (frame->int_no < 32) {
        pcb_t* current = scheduler_get_current();
        
        kernel_print("\r\nCPU EXCEPTION ");
        kernel_print_hex(frame->int_no);
        kernel_print(" (");
        if (frame->int_no == 14) kernel_print("Page Fault");
        else if (frame->int_no == 13) kernel_print("GPF");
        else kernel_print("Other");
        kernel_print(") Error Code: ");
        kernel_print_hex(frame->err_code);
        kernel_print("\r\n");
        
        if (current) {
            kernel_print("PID: "); kernel_print_hex(current->pid);
            kernel_print("\r\n");
        }
        
        kernel_print("EIP: "); kernel_print_hex(frame->eip);
        kernel_print(" CS: "); kernel_print_hex(frame->cs);
        kernel_print(" EFLAGS: "); kernel_print_hex(frame->eflags);
        kernel_print("\r\n");
        
        kernel_print("EAX: "); kernel_print_hex(frame->eax);
        kernel_print(" EBX: "); kernel_print_hex(frame->ebx);
        kernel_print(" ECX: "); kernel_print_hex(frame->ecx);
        kernel_print(" EDX: "); kernel_print_hex(frame->edx);
        kernel_print("\r\n");
        
        kernel_print("DS: "); kernel_print_hex(frame->ds);
        kernel_print(" ES: "); kernel_print_hex(frame->es);
        kernel_print(" FS: "); kernel_print_hex(frame->fs);
        kernel_print(" GS: "); kernel_print_hex(frame->gs);
        kernel_print("\r\n");
        
        kernel_print("ESP: "); kernel_print_hex(frame->user_esp);
        kernel_print(" SS: "); kernel_print_hex(frame->user_ss);
        kernel_print("\r\n");
        
        if (frame->int_no == 14) {
            uint32_t fault_addr = hal_cpu_get_cr2();
            kernel_print("Fault Address: "); kernel_print_hex(fault_addr);
            kernel_print(" (");
            if (frame->err_code & 0x01) kernel_print("Present ");
            else kernel_print("Non-present ");
            if (frame->err_code & 0x02) kernel_print("Write ");
            else kernel_print("Read ");
            if (frame->err_code & 0x04) kernel_print("User ");
            else kernel_print("Kernel ");
            kernel_print(")\r\n");
        }
        
        // Decide whether to kill process or panic
        if ((frame->cs & 0x03) == 0x03) {
            if (current) {
                kernel_print("User process crashed. Terminating.\r\n");
                process_exit(current, frame->int_no);
                // The scheduler will switch to another process
                return;
            }
        }
        
        kernel_panic("Unhandled CPU exception in kernel");
    } else if (frame->int_no >= 32 && frame->int_no < 48) {
        uint32_t irq = frame->int_no - 32;
        if (irq == 0) {
            extern void timer_interrupt_handler(void);
            timer_interrupt_handler();
        } else if (irq == 1) {
            extern void keyboard_interrupt_handler(void);
            keyboard_interrupt_handler();
        }
        hal_pic_send_eoi(irq);
    }
}

// Timer and Keyboard handlers (minimal stubs or move logic here)
void timer_interrupt_handler(void) {
    extern void hal_timer_interrupt_handler(void);
    hal_timer_interrupt_handler();
}

void keyboard_interrupt_handler(void) {
    uint8_t scancode = hal_inb(PORT_KEYBOARD_DATA);
    (void)scancode;
    // Simple IPC send to keyboard driver (PID 2)
    // For now, just print or ignore if nobody is listening
}

// Assembly wrappers
__asm__ (
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
    "push %esp\n"
    "call interrupt_handler_common\n"
    "add $4, %esp\n"
    "pop %gs\n"
    "pop %fs\n"
    "pop %es\n"
    "pop %ds\n"
    "popa\n"
    "add $8, %esp\n"
    "iret\n"

"divide_error_handler: push $0; push $0; jmp interrupt_common\n"
"debug_exception_handler: push $0; push $1; jmp interrupt_common\n"
"nmi_handler: push $0; push $2; jmp interrupt_common\n"
"breakpoint_handler: push $0; push $3; jmp interrupt_common\n"
"overflow_handler: push $0; push $4; jmp interrupt_common\n"
"bound_range_exceeded_handler: push $0; push $5; jmp interrupt_common\n"
"invalid_opcode_handler: push $0; push $6; jmp interrupt_common\n"
"device_not_available_handler: push $0; push $7; jmp interrupt_common\n"
"double_fault_handler: push $8; jmp interrupt_common\n"
"invalid_tss_handler: push $10; jmp interrupt_common\n"
"segment_not_present_handler: push $11; jmp interrupt_common\n"
"stack_segment_fault_handler: push $12; jmp interrupt_common\n"
"general_protection_fault_handler: push $13; jmp interrupt_common\n"
"page_fault_handler: push $14; jmp interrupt_common\n"
"x87_fpu_error_handler: push $0; push $16; jmp interrupt_common\n"
"alignment_check_handler: push $17; jmp interrupt_common\n"
"machine_check_handler: push $0; push $18; jmp interrupt_common\n"
"simd_floating_point_handler: push $0; push $19; jmp interrupt_common\n"
"timer_irq_handler: push $0; push $32; jmp interrupt_common\n"
"keyboard_irq_handler: push $0; push $33; jmp interrupt_common\n"

"syscall_handler_wrapper:\n"
    "push $0\n"
    "push $0x80\n"
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
    "push %esp\n"
    "call syscall_dispatch\n"
    "add $4, %esp\n"
    "pop %gs\n"
    "pop %fs\n"
    "pop %es\n"
    "pop %ds\n"
    "popa\n"
    "add $8, %esp\n"
    "iret\n"
);
