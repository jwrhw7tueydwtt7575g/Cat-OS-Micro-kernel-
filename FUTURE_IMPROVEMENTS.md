# Future Improvements & Roadmap

This file lists actionable improvements, grouped by priority, with brief notes on complexity and suggested starting points.

Short-term (low to medium complexity)
- Replace hardcoded service loading with a tiny filesystem (TarFS/FAT12) and an ELF loader.
  - Files to touch: `boot/stage2_c.c`, `kernel/main.c`, `userspace/init.c`
  - Complexity: medium — requires parsing simple file headers and relocating sections.
- Add unit tests for `kernel/memory.c` and `kernel/ipc.c` using `tests/test_framework.c`.
  - Complexity: low — refactor small functions to be testable.
- Improve debug logging and structured panic output (print trap number, EIP, stack pointer).
  - Files: `kernel/interrupt.c`, `kernel/entry.asm`.

Mid-term (medium complexity)
- Implement user-mode dynamic loading and simple `libc` runtime.
  - Benefit: Easier user app development and smaller binaries.
  - Complexity: medium-high — requires relocations and symbol resolution.
- Add VFS abstraction and at least one filesystem (FAT12 or TarFS).
  - Complexity: medium.
- Implement a service registry and named ports for drivers rather than fixed PIDs.
  - Complexity: low-medium.

Long-term (high complexity)
- Add networking stack (user-space): basic NIC driver + TCP/IP (uIP or lwIP style).
  - Complexity: high; requires careful driver-to-user IPC design and network buffering.
- SMP / Multi-core support.
  - Complexity: high — scheduler redesign, per-core runqueues, locking primitives.
- Port to x86_64.
  - Complexity: very high — data models, paging, syscall ABI, and assembly stubs need rewrite.

Operational & process improvements
- Add CI tests that boot a QEMU image and run smoke tests (boot to shell, run `ps`, start/stop driver).
- Add a contributor guide with coding standards and a template for RFCs for major changes.
- Track issues with tags: `good-first-issue`, `design`, `security`, `performance`.

If you'd like, I can:
1. Create a minimal ELF loader prototype (small C file + tests).
2. Add a QEMU-based CI job script that runs on the repository.
3. Create `docs/CONTRIBUTING.md` with contribution process and coding guidelines.

Pick which of the above you'd like me to implement next.
