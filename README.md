# Cat-OS — README

Compact overview and quick start to build and run Cat-OS locally.

## Quick start (build + run under QEMU)

1. Build the project:

```bash
make clean && make
```

2. Run in QEMU (serial output to terminal):

```bash
timeout 15s qemu-system-i386 -fda build/disk.img \
	-serial stdio
```

Notes:
- If you hit a kernel panic (e.g. "Unhandled CPU exception in kernel"), check `kernel/interrupt.c` and `kernel/memory.c` for page faults.
- Use `-serial stdio` to capture kernel prints in the terminal.

## Project structure (short)
- `boot/` — stage1 and stage2 bootloaders
- `hal/` — CPU / PIC / timer helpers
- `kernel/` — microkernel core (scheduler, IPC, memory)
- `drivers/` — console/keyboard/timer drivers (user-space)
- `userspace/` — init, shell, and small user apps
- `docs/` — design docs and architecture

## Contributing
- Read `docs/ARCHITECTURE.md` and `CAT_OS_SYSTEM_DOCUMENTATION.md` for design rationale.
- Add tests in `tests/` when changing core components.
- Follow the capability model: avoid adding ad-hoc privileged paths in the kernel.

## Debugging
- Add temporary printk statements to `kernel/main.c` and `kernel/interrupt.c`.
- Use the provided `tests/test_framework.c` to validate changes.

For more details see `CAT_OS_SYSTEM_DOCUMENTATION.md` and `docs/ARCHITECTURE.md`.
