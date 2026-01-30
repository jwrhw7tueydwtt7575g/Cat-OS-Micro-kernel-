## Cat-OS Microkernel — System Documentation

This document consolidates architecture, boot flow, core internals, troubleshooting tips, and scaling guidance for Cat-OS (i386 microkernel educational project).

**Goal:** Make the repository self-contained for contributors: explain how components fit together, how to extend them, and a clear roadmap for scaling the system.

---

### Contents (quick)
- Architecture overview and diagrams
- Boot sequence and early init
- Kernel subsystems (IPC, scheduler, memory, capabilities)
- Drivers and userspace services
- Troubleshooting (kernel panics / page faults)
- How to scale the project (engineering & roadmap)

---

### Architecture (summary)

Cat-OS is a microkernel: the kernel provides minimal services (IPC, scheduling, memory management, syscalls and capability checks). Device drivers and most services run in user-space processes that communicate with the kernel via IPC.

Key directories:
- [boot](boot): stage1/2 bootloaders and early setup
- [hal](hal): low-level, hardware-specific helpers (I/O, PIC, timer)
- [kernel](kernel): core kernel code (scheduler, IPC, memory, syscalls)
- [drivers](drivers): driver implementations (console, keyboard, timer)
- [userspace](userspace): simple apps (init, shell, monitor)

---

### Box-and-Arrow Diagram (Mermaid + ASCII fallback)

Mermaid (rendered by many markdown viewers):

```mermaid
graph LR
    A[BIOS/Hardware] --> B[Stage1 Bootloader @0x7C00]
    B --> C[Stage2 Bootloader]
    C --> D[Kernel Entry]
    subgraph KERNEL
        D --> E[HAL]
        D --> F[Memory Manager]
        D --> G[Scheduler]
        D --> H[IPC / Syscall Dispatcher]
        D --> I[Capability Manager]
    end
    subgraph USERSPACE
        J[Init (PID 1)] --> K[Drivers (Keyboard, Console, Timer)]
        J --> L[Shell]
        L --> M[User Apps]
    end
    H <--> K
    G -->|context switch| M
    F -->|paging| M
```

ASCII fallback:

```
     [BIOS]
            |
     [Stage1] -> [Stage2]
            |
     [Kernel Entry]
     +--------------------------------+
     |   Kernel (IPC, Scheduler, MM)  |
     |  +-- HAL --+  +-- CapMngr --+  |
     +--------------------------------+
            |          |           |
    [Drivers]  [Init PID1]   [Shell]
            |          |           |
     [Console] [Timer]     [User Apps]
```

---

### Boot flow (high level)
1. BIOS -> loads Stage1 at 0x7C00
2. Stage1 loads Stage2 from disk and jumps
3. Stage2 enables A20, sets up GDT, switches to protected mode and loads kernel
4. Kernel starts: init HAL, memory, IDT, scheduler, IPC
5. Kernel spawns `init` and other early services; userspace drivers attach via IPC

---

### Detailed internals (where to look)

- Boot: `boot/stage1.asm`, `boot/stage2.asm`, `boot/stage2_c.c` — sequence to load kernel and enable protected mode
- Entry: `kernel/entry.asm` — kernel entry and stack setup
- Initialization: `kernel/main.c` — `kernel_main()` creates subsystems and launches init
- Memory: `kernel/memory.c` — physical allocator, page table setup, identity mappings
- IPC: `kernel/ipc.c` — message structures, send/receive paths, blocking behavior
- Scheduler/process: `kernel/process.c`, `kernel/scheduler.c` — PCB layout, context switching, states
- Syscalls: `kernel/syscall.c` — syscall dispatcher and argument validation
- Capabilities: `kernel/capability.c` — capability issuance and checks
- HAL: `hal/*.c` — `io.c`, `timer.c`, `pic.c`, `cpu.c` (hardware helpers used by kernel)
- Drivers: `drivers/*.c` — user-space drivers that expect messages from kernel
- Userspace: `userspace/*` — `init.c`, `shell.c`, `monitor.c` — how services are started

---

### Troubleshooting (common issues)

- Kernel panic on boot ("Unhandled CPU exception in kernel")
    - Check the exception handler in `kernel/interrupt.c` and any printed registers/trap numbers.
    - A page fault often points to missing identity mappings or incorrect page table setup in `kernel/memory.c`.
- Devices not responding (no console)
    - Confirm the console driver was launched by `init` (`userspace/init.c`). If not, check the loader / hardcoded offsets in `kernel/main.c`.
- IPC hangs
    - Inspect `kernel/ipc.c` for queue logic and ensure message delivery wakes blocked processes; review `scheduler.c` for blocked->ready transitions.

Debugging tips:
- Run QEMU with `-serial stdio` to capture kernel prints and panics.
- Add diagnostic prints in `kernel/main.c` and `kernel/interrupt.c` to narrow init failures.
- Use tests in `tests/test_framework.c` for unit-level checks.

---

### How to extend & scale (practical steps)

Keep these principles as the system grows:
- Kernel minimalism: only put code into `kernel/` that must run in ring-0.
- Services in user-space: drivers, network stack, VFS, and libc reduce kernel complexity.
- Versioned IPC schemas: add a small header with message type/version to enable compatibility.
- Tests and CI: automated boots and smoke tests prevent regressions.

Scaling roadmap (high level):
1. Add a tiny filesystem and ELF loader to replace hardcoded PID loading.
2. Convert drivers to named services using a service registry.
3. Implement user-mode runtime (libc) and shared libraries to reduce duplication.
4. Add CI: nightly QEMU boots that run a test script (boot->run `ps`->`shell` smoke tests).

---

### Next steps I can take (pick any)
1. Create `FUTURE_IMPROVEMENTS.md` with tasks and complexity estimates.
2. Add a minimal Quick Start to `README.md` with `make` + `qemu` commands and debug tips.
3. Add a checklist linking to files that need improved docstrings and unit tests.

Tell me which of these you'd like next and I'll implement them.
