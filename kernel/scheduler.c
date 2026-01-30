// Kernel Scheduler
// Round-robin process scheduling

#include "kernel.h"
#include "hal.h"
#include <stddef.h>

// Scheduler state
static pcb_t* current_process = NULL;
static pcb_t* ready_queue_head = NULL;
static pcb_t* ready_queue_tail = NULL;
static uint32_t next_pid = 1;
static uint32_t scheduler_ticks = 0;

#define TIME_QUANTUM 10

// Forward declarations
static void scheduler_add_to_ready(pcb_t* process);
static void scheduler_remove_from_ready(pcb_t* process);

// Assembly context switch (Linker will handle the label)
extern void context_switch_asm(pcb_t* from, pcb_t* to);

__asm__ (
".global context_switch_asm\n"
"context_switch_asm:\n"
    "mov 4(%esp), %eax\n"
    "mov 8(%esp), %edx\n"
    
    "test %eax, %eax\n"
    "jz .first_run\n"
    
    "pushfl\n"
    "push %ebp\n"
    "push %ebx\n"
    "push %esi\n"
    "push %edi\n"
    
    "mov %esp, 68(%eax)\n"
    
".first_run:\n"
    "mov 68(%edx), %esp\n"
    
    "pop %edi\n"
    "pop %esi\n"
    "pop %ebx\n"
    "pop %ebp\n"
    "popfl\n"
    "ret\n"
);

void scheduler_init(void) {
    current_process = NULL;
    ready_queue_head = NULL;
    ready_queue_tail = NULL;
    next_pid = 1;
    scheduler_ticks = 0;
    kernel_print("Scheduler initialized\r\n");
}

void scheduler_add_process(pcb_t* process) {
    if (!process) return;
    if (process->pid == 0) process->pid = next_pid++;
    
    if (process->state == PROCESS_READY) return;
    
    scheduler_add_to_ready(process);
    process->state = PROCESS_READY;
}

void scheduler_remove_process(pcb_t* process) {
    if (!process) return;
    
    if (process->state == PROCESS_READY) {
        scheduler_remove_from_ready(process);
    }
    
    if (process == current_process) {
        current_process = NULL;
        process->state = PROCESS_TERMINATED;
        scheduler_yield();
    }
}

void scheduler_tick(void) {
    scheduler_ticks++;
    if (!current_process) {
        scheduler_yield();
        return;
    }
    current_process->cpu_time++;
    if (scheduler_ticks % TIME_QUANTUM == 0) {
        scheduler_yield();
    }
}

void scheduler_yield(void) {
    pcb_t* next_process = ready_queue_head;
    
    if (!next_process) {
        if (current_process && current_process->state == PROCESS_RUNNING) return;
        return;
    }
    
    if (current_process && current_process->state == PROCESS_RUNNING) {
        current_process->state = PROCESS_READY;
        scheduler_add_to_ready(current_process);
    }
    
    next_process = ready_queue_head;
    scheduler_remove_from_ready(next_process);
    
    scheduler_switch_to(next_process);
}

void scheduler_switch_to(pcb_t* next) {
    pcb_t* prev = current_process;
    current_process = next;
    next->state = PROCESS_RUNNING;
    
    if (prev != next) {
        kernel_print("S");
        context_switch_asm(prev, next);
    }
}

pcb_t* scheduler_get_current(void) {
    return current_process;
}

pcb_t* scheduler_find_process(uint32_t pid) {
    if (current_process && current_process->pid == pid) return current_process;
    pcb_t* current = ready_queue_head;
    while (current) {
        if (current->pid == pid) return current;
        current = current->next;
    }
    return NULL;
}

void scheduler_unblock_process(pcb_t* process) {
    if (process && process->state == PROCESS_BLOCKED) {
        process->state = PROCESS_READY;
        scheduler_add_to_ready(process);
    }
}

void scheduler_block_current(void) {
    if (current_process) {
        current_process->state = PROCESS_BLOCKED;
        scheduler_yield();
    }
}

static void scheduler_add_to_ready(pcb_t* process) {
    if (!process) return;
    process->next = NULL;
    process->prev = NULL;
    
    if (!ready_queue_head) {
        ready_queue_head = process;
        ready_queue_tail = process;
    } else {
        ready_queue_tail->next = process;
        process->prev = ready_queue_tail;
        ready_queue_tail = process;
    }
}

static void scheduler_remove_from_ready(pcb_t* process) {
    if (!process) return;
    
    if (process->prev) {
        process->prev->next = process->next;
    } else if (ready_queue_head == process) {
        ready_queue_head = process->next;
    } else {
        return;
    }
    
    if (process->next) {
        process->next->prev = process->prev;
    } else if (ready_queue_tail == process) {
        ready_queue_tail = process->prev;
    }
    
    process->next = NULL;
    process->prev = NULL;
}

void scheduler_set_priority(pcb_t* process, uint32_t priority) {
    if (process) process->priority = priority;
}
