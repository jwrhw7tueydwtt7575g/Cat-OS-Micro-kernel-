# Architecture — Cat-OS

This file contains a technical deep-dive for implementers: architecture diagrams, component responsibilities, memory layout, interrupt model, and developer guidance.

## High-level model

Cat-OS is a message-driven microkernel. The kernel provides scheduling, memory management, inter-process communication (IPC) and syscalls. Drivers and higher-level services run in user-space and communicate through well-defined IPC messages. A capability manager enforces fine-grained permissions.

### Layers (concise)
- Hardware (CPU, PIC, PIT, Keyboard, VGA)
- HAL: hardware helpers used by kernel
- Microkernel: Scheduler, Memory Manager, IPC, Syscall dispatcher, Capability manager
- User-space: init, drivers, shell, apps

## Reference Diagram (ASCII + Mermaid already in main doc)

Refer to the diagrams in the root documentation (`CAT_OS_SYSTEM_DOCUMENTATION.md`) for both Mermaid and ASCII views. The architecture in this file focuses on implementation notes and responsibilities for each component.

## Component Responsibilities

- Scheduler (`kernel/scheduler.c`)
	- Round-robin queue; time-slice handled by PIT interrupts.
	- Save/restore CPU registers (see `kernel/entry.asm` for context layout).
- Memory Manager (`kernel/memory.c`)
	- Physical page allocator (bitmap), page table creation and per-process address spaces.
	- Early identity mapping traces during boot; ensure kernel pages are mapped before enabling interrupts.
- IPC Core (`kernel/ipc.c`)
	- Message envelope format and queueing.
	- Blocking `receive` moves process to BLOCKED; `send` wakes receiver.
- Syscall Dispatcher (`kernel/syscall.c`)
	- Validates user pointers, capability tokens, and dispatches to kernel services.
- Capability Manager (`kernel/capability.c`)
	- Issue, validate and revoke capabilities. Keep capability metadata small and immutable when possible.

## Memory & Interrupts (quick)
- Kernel virtual region: 0xC0000000+ (reserved for kernel mappings)
- User space: below 0xC0000000
- Interrupt vectors: hardware vectors remapped through PIC; IDT entries initialized early in `kernel/main.c`.

## Implementation notes and pitfalls
- Ensure your early boot mapping includes the pages used by the stack and by the interrupt handlers.
- Page faults during boot commonly stem from missing page-table entries or incorrect permissions.
- IPC message copies should validate user addresses early to avoid kernel crashes.

## Development guidance
- Add unit tests for `kernel/memory.c` and `kernel/ipc.c` using `tests/test_framework.c`.
- Document message layouts in a single header to keep IPC stable across refactors.

## Scaling considerations
- Introduce a service registry for locating drivers by name rather than PID.
- Provide versioned IPC messages and a small compatibility layer.

For contributor onboarding and diagrams, see `CAT_OS_SYSTEM_DOCUMENTATION.md`.
