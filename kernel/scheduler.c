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

// Time quantum for each process (in timer ticks)
#define TIME_QUANTUM 10

// Forward declarations
static void scheduler_add_to_ready(pcb_t* process);
static void scheduler_remove_from_ready(pcb_t* process);
static void context_switch(pcb_t* from, pcb_t* to);

// Initialize scheduler
void scheduler_init(void) {
    current_process = NULL;
    ready_queue_head = NULL;
    ready_queue_tail = NULL;
    next_pid = 1;
    scheduler_ticks = 0;
    
    kernel_print("Scheduler initialized\r\n");
}

// Add process to scheduler
void scheduler_add_process(pcb_t* process) {
    if (!process) {
        return;
    }
    
    // Assign PID if not already assigned
    if (process->pid == 0) {
        process->pid = next_pid++;
    }
    
    // Add to ready queue
    scheduler_add_to_ready(process);
    process->state = PROCESS_READY;
    
    kernel_print("Process added to scheduler, PID: ");
    kernel_print_hex(process->pid);
    kernel_print("\r\n");
}

// Remove process from scheduler
void scheduler_remove_process(pcb_t* process) {
    if (!process) {
        return;
    }
    
    // Remove from ready queue
    scheduler_remove_from_ready(process);
    
    // If this is the current process, switch to next
    if (process == current_process) {
        current_process = NULL;
        scheduler_yield();
    }
}

// Scheduler tick (called by timer interrupt)
void scheduler_tick(void) {
    scheduler_ticks++;
    
    if (!current_process) {
        scheduler_yield();
        return;
    }
    
    // Decrease time quantum
    current_process->cpu_time++;
    
    // Check if time quantum expired
    if (scheduler_ticks % TIME_QUANTUM == 0) {
        scheduler_yield();
    }
}

// Yield CPU to next process
void scheduler_yield(void) {
    pcb_t* next_process = ready_queue_head;
    
    if (!next_process) {
        // No processes ready to run
        current_process = NULL;
        return;
    }
    
    // If we have a current process and it's still ready, move it to back of queue
    if (current_process && current_process->state == PROCESS_READY) {
        scheduler_remove_from_ready(current_process);
        scheduler_add_to_ready(current_process);
    }
    
    // Get next process
    next_process = ready_queue_head;
    scheduler_remove_from_ready(next_process);
    
    // Switch to next process
    scheduler_switch_to(next_process);
}

// Switch to specific process
void scheduler_switch_to(pcb_t* next) {
    pcb_t* prev = current_process;
    current_process = next;
    next->state = PROCESS_RUNNING;
    
    if (prev != next) {
        context_switch(prev, next);
    }
}

// Get current process
pcb_t* scheduler_get_current(void) {
    return current_process;
}

// Find process by PID
pcb_t* scheduler_find_process(uint32_t pid) {
    pcb_t* current = ready_queue_head;
    
    while (current) {
        if (current->pid == pid) {
            return current;
        }
        current = current->next;
    }
    
    // Check current process
    if (current_process && current_process->pid == pid) {
        return current_process;
    }
    
    return NULL;
}

// Unblock a process
void scheduler_unblock_process(pcb_t* process) {
    if (process && process->state == PROCESS_BLOCKED) {
        process->state = PROCESS_READY;
        scheduler_add_to_ready(process);
    }
}

// Block current process
void scheduler_block_current(void) {
    if (current_process) {
        current_process->state = PROCESS_BLOCKED;
        scheduler_yield();
    }
}

// Get scheduler statistics
void scheduler_get_stats(uint32_t* total_processes, uint32_t* ready_processes, uint32_t* total_ticks) {
    uint32_t ready_count = 0;
    pcb_t* current = ready_queue_head;
    
    while (current) {
        ready_count++;
        current = current->next;
    }
    
    if (total_processes) *total_processes = next_pid - 1;
    if (ready_processes) *ready_processes = ready_count;
    if (total_ticks) *total_ticks = scheduler_ticks;
}

// Add process to ready queue
static void scheduler_add_to_ready(pcb_t* process) {
    if (!process) {
        return;
    }
    
    // Remove from queue if already present
    scheduler_remove_from_ready(process);
    
    // Add to end of queue
    if (!ready_queue_head) {
        ready_queue_head = process;
        ready_queue_tail = process;
        process->next = NULL;
        process->prev = NULL;
    } else {
        ready_queue_tail->next = process;
        process->prev = ready_queue_tail;
        process->next = NULL;
        ready_queue_tail = process;
    }
}

// Remove process from ready queue
static void scheduler_remove_from_ready(pcb_t* process) {
    if (!process) {
        return;
    }
    
    // Remove from queue
    if (process->prev) {
        process->prev->next = process->next;
    } else {
        ready_queue_head = process->next;
    }
    
    if (process->next) {
        process->next->prev = process->prev;
    } else {
        ready_queue_tail = process->prev;
    }
    
    process->next = NULL;
    process->prev = NULL;
}

// Context switch between processes
static void context_switch(pcb_t* from, pcb_t* to) {
    if (!to) {
        return;
    }
    
    // Save current process state
    if (from) {
        // Save registers (simplified version)
        __asm__ volatile(
            "mov %%eax, %0\n"
            "mov %%ebx, %1\n"
            "mov %%ecx, %2\n"
            "mov %%edx, %3\n"
            "mov %%esp, %4\n"
            "mov %%ebp, %5\n"
            "mov %%esi, %6\n"
            "mov %%edi, %7\n"
            : "=m"(from->registers[0]), "=m"(from->registers[1]), 
              "=m"(from->registers[2]), "=m"(from->registers[3]),
              "=m"(from->registers[4]), "=m"(from->registers[5]),
              "=m"(from->registers[6]), "=m"(from->registers[7])
        );
    }
    
    // Switch page directory
    if (to->page_directory) {
        hal_cpu_set_cr3(to->page_directory);
    }
    
    // Restore new process state
    __asm__ volatile(
        "mov %0, %%eax\n"
        "mov %1, %%ebx\n"
        "mov %2, %%ecx\n"
        "mov %3, %%edx\n"
        "mov %4, %%esp\n"
        "mov %5, %%ebp\n"
        "mov %6, %%esi\n"
        "mov %7, %%edi\n"
        :
        : "m"(to->registers[0]), "m"(to->registers[1]), 
          "m"(to->registers[2]), "m"(to->registers[3]),
          "m"(to->registers[4]), "m"(to->registers[5]),
          "m"(to->registers[6]), "m"(to->registers[7])
    );
}

// Set process priority
void scheduler_set_priority(pcb_t* process, uint32_t priority) {
    if (process) {
        process->priority = priority;
        // In a more sophisticated scheduler, this would affect scheduling order
    }
}

// Get next available PID
uint32_t scheduler_get_next_pid(void) {
    return next_pid;
}

// Check if PID is in use
bool scheduler_pid_in_use(uint32_t pid) {
    pcb_t* process = scheduler_find_process(pid);
    return process != NULL;
}
