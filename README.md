# MiniSecureOS

A minimal, secure, microkernel-based operating system for educational purposes.

## Architecture

- **Microkernel Design**: Minimal kernel with maximum user-space services
- **32-bit x86**: Single-core i386 architecture
- **Capability-Based Security**: Complete isolation and least privilege
- **User-Space Drivers**: All drivers run in user space
- **Message-Based IPC**: Inter-process communication via kernel

## System Components

### Boot Sequence
- **Stage 1**: MBR bootloader (512 bytes)
- **Stage 2**: Protected mode setup and kernel loading
- **Kernel**: Microkernel with HAL abstraction
- **Init**: First user-space process (PID 1)

### Kernel Modules
- **Scheduler**: Round-robin process scheduling
- **Memory Manager**: Paging and memory allocation
- **IPC Core**: Message passing between processes
- **Syscall Dispatcher**: System call handling
- **Capability Manager**: Security and permission enforcement

### User Space
- **Drivers**: Keyboard, console, timer drivers
- **Utilities**: Shell, system monitor, init process
- **Applications**: User programs and tools

## Building

```bash
make all      # Build complete system
make clean    # Clean build artifacts
make test     # Run tests
make run      # Run in QEMU
```

## Requirements

- GCC cross-compiler (i686-elf)
- NASM assembler
- QEMU emulator
- Make build system

## Project Structure

```
MiniSecureOS/
├── boot/           # Bootloader code
├── kernel/         # Kernel source
├── hal/            # Hardware abstraction layer
├── drivers/        # User-space drivers
├── userspace/      # User programs
├── include/        # Header files
├── build/          # Build artifacts
└── docs/           # Documentation
```

## License

Educational use only.
