# Cat-OS (x86 Microkernel)

Cat-OS is a message-driven microkernel designed for process isolation and hardware abstraction.

## Features
- **Microkernel Architecture**: Minimal Ring 0 core (scheduling, IPC, memory).
- **Process Isolation**: Drivers (Keyboard, Console, Timer) are Ring 0 Kernel Tasks, while Apps (Shell, Init) run in Ring 3.
- **IPC System**: Asynchronous message passing for inter-service communication.
- **Standard Entry Points**: Every process uses a deterministic `_start` entry point for stability.

## Quick Start (Build & Run)

1. **Build the system**:
   ```bash
   make clean && make
   ```

2. **Run in QEMU**:
   ```bash
   make run
   ```

## Architecture

```
       [ USER SPACE (Ring 3) ]             [ KERNEL SPACE (Ring 0) ]
      +-----------------------+           +-------------------------+
      |        Shell          |           |   Keyboard Driver       |
      |   (User Process)      |           |    (Kernel Task)        |
      +----------+------------+           +------------+------------+
                 |                                     |
                 |  Syscall (Int 0x80)                 |  Direct Access
                 v                                     v
      +-------------------------------------------------------------+
      |                      Cat-OS Microkernel                     |
      |  +-----------+  +-----------+  +-----------+  +----------+  |
      |  | Scheduler |  |  Mem Mgr  |  |  IPC Core |  |   HAL    |  |
      |  +-----------+  +-----------+  +-----------+  +----------+  |
      +-------------------------------------------------------------+
                 ^                                     ^
                 |                                     |
      +----------+------------+           +------------+------------+
      |     Init Process      |           |    Console/Timer        |
      |     (Ring 3)          |           |    Kernel Tasks         |
      +-----------------------+           +-------------------------+
```

## Shortcuts
- **Build**: `make`
- **Run**: `make run`
- **Output**: Serial (to terminal) and VGA (graphical window).
