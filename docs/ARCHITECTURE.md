# Cat-OS Architecture

## Core Design
Cat-OS follows a hybrid microkernel approach where core services are split by privilege level (Ring) for stability and performance.

### 1. Privilege Separation
- **Ring 0 (Kernel Tasks)**: Device drivers (Keyboard, Console, Timer) run in Ring 0. This allows them to execute privileged instructions (like `inb`/`outb`) without the overhead of syscall-based port access.
- **Ring 3 (User Processes)**: Programs like `Init` and `Shell` run in Ring 3. They are isolated from hardware and must use System Calls (`int 0x80`) to interact with the kernel.

### 2. Context Switching
The scheduler uses a robust assembly-based context switch residing in `kernel/scheduler.c`:
- **State Preservation**: Saves/restores `EFLAGS`, `EBP`, `EBX`, `ESI`, and `EDI`.
- **Address Space**: Automatically switches `CR3` (Page Directory) on every task switch.
- **Interrupt Safety**: Updates `TSS.esp0` to ensure user-space interrupts have a valid kernel stack to land on.

### 3. Inter-Process Communication (IPC)
The IPC system facilitates communication between user processes and kernel tasks:
- **Send/Receive**: Processes can send messages to a target PID or wait for incoming messages.
- **Message Format**: Standardized `ipc_abi_message_t` ensures compatibility across the system.

## System Components

| Component | Responsibility | Privilege |
|-----------|----------------|-----------|
| **Scheduler** | Round-robin multitasking, CR3/TSS switching | Ring 0 |
| **MMU** | Page allocation, per-process page tables | Ring 0 |
| **IPC** | Message passing and process unblocking | Ring 0 |
| **Drivers** | Hardware interaction (Keyboard, VGA, Timer) | Ring 0 |
| **Shell/Init** | User interface and service management | Ring 3 |

## Memory Layout
- **0x00100000 (1MB)**: Kernel Binary (linked via `kernel.ld`)
- **0x00400000 (4MB)**: Common Virtual Base for all User/Driver binaries.
- Per-process stacks are allocated dynamically in high memory.
