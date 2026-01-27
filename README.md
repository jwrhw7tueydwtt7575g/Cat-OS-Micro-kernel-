# MiniSecureOS Architecture Documentation

## Overview

MiniSecureOS is a minimal, secure, microkernel-based operating system designed for educational purposes. The system emphasizes security, modularity, and clarity over performance optimization.

## System Architecture

### Core Principles

1. **Microkernel Design**: Minimal kernel with maximum user-space services
2. **Security-First**: Capability-based security model with complete isolation
3. **User-Space Drivers**: All hardware drivers run in user space
4. **Message-Based IPC**: All inter-process communication via kernel messages
5. **Educational Focus**: Code clarity and learning over performance

### Architecture Layers

```
┌─────────────────────────────────────────────────────────────┐
│                    User Applications                         │
├─────────────────────────────────────────────────────────────┤
│                 System Utilities (Shell, Monitor)           │
├─────────────────────────────────────────────────────────────┤
│                User-Space Drivers (Keyboard, Console, Timer) │
├─────────────────────────────────────────────────────────────┤
│                      Microkernel                             │
│  ┌─────────────┬─────────────┬─────────────┬─────────────┐    │
│  │ Scheduler   │ Memory Mgr  │ IPC Core    │ Syscalls    │    │
│  └─────────────┴─────────────┴─────────────┴─────────────┘    │
├─────────────────────────────────────────────────────────────┤
│                Hardware Abstraction Layer (HAL)              │
│  ┌─────────────┬─────────────┬─────────────┬─────────────┐    │
│  │ CPU Control │ I/O Ports   │ Timer       │ PIC         │    │
│  └─────────────┴─────────────┴─────────────┴─────────────┘    │
├─────────────────────────────────────────────────────────────┤
│                       Hardware                               │
│  ┌─────────────┬─────────────┬─────────────┬─────────────┐    │
│  │ CPU (i386)  │ Keyboard    │ VGA         │ Timer/PIT   │    │
│  └─────────────┴─────────────┴─────────────┴─────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

## Boot Sequence

### Stage 1 Bootloader (MBR - 512 bytes)
- Loaded by BIOS at 0x7C00
- Loads Stage 2 bootloader from disk
- Transfers control to Stage 2

### Stage 2 Bootloader (~4KB)
- Enables A20 line for access to memory above 1MB
- Sets up Global Descriptor Table (GDT)
- Transitions CPU to protected mode
- Loads kernel ELF executable
- Jumps to kernel entry point

### Kernel Initialization
- Initializes HAL (CPU, I/O, timer, PIC)
- Sets up memory management (paging)
- Initializes scheduler and process management
- Sets up IPC system
- Configures system call interface
- Starts init process (PID 1)

## Kernel Components

### Scheduler
- **Algorithm**: Round-robin with time quantum (10ms)
- **Process States**: CREATED, READY, RUNNING, BLOCKED, TERMINATED
- **Priority Levels**: 3 levels (HIGH, NORMAL, LOW)
- **Context Switching**: Hardware context save/restore

### Memory Manager
- **Paging**: 4KB pages with two-level page tables
- **Physical Memory**: Bitmap-based allocation
- **Virtual Memory**: Per-process address spaces
- **Memory Protection**: Page-level protection bits

### IPC Core
- **Message Types**: DATA, CONTROL, SIGNAL, RESPONSE, DRIVER
- **Communication**: Synchronous and asynchronous messaging
- **Message Queues**: Per-process message queues
- **Buffer Management**: Kernel-managed message buffers

### System Call Dispatcher
- **Interface**: INT 0x80 software interrupt
- **Calling Convention**: EAX (syscall number), EBX/ECX/EDX (parameters)
- **Validation**: Parameter checking and capability verification
- **Categories**: Process, Memory, IPC, Driver, System operations

### Capability Manager
- **Security Model**: Capability-based access control
- **Capability Types**: PROCESS, MEMORY, DRIVER, HARDWARE, SYSTEM, IPC
- **Permissions**: READ, WRITE, EXECUTE, CREATE, DELETE, TRANSFER
- **Enforcement**: All operations validated against capabilities

## User-Space Components

### Drivers
All drivers run as user-space processes with message-based communication:

#### Keyboard Driver (PID 2)
- Handles keyboard input via IRQ 1
- Converts scancodes to ASCII characters
- Provides character buffer to applications
- Manages special keys (shift, ctrl, alt)

#### Console Driver (PID 3)
- Manages VGA text mode display (80x25)
- Handles cursor positioning and screen scrolling
- Provides text output services
- Supports color and screen control

#### Timer Driver (PID 4)
- Provides timing services and sleep functionality
- Manages timer-based events and alarms
- Maintains system uptime
- Supports millisecond precision

### System Utilities

#### Init Process (PID 1)
- First user-space process
- Starts and monitors system services
- Handles orphaned processes
- Coordinates system shutdown

#### Shell Program
- Command-line interface
- Built-in commands (help, clear, ps, kill, mem, uptime, drivers, test)
- Process management and job control
- I/O redirection support

#### System Monitor
- Real-time system monitoring
- Process and memory statistics
- Driver status and performance metrics
- Interactive diagnostic interface

## Security Architecture

### Isolation Mechanisms
- **Process Isolation**: Separate address spaces, no shared memory by default
- **Driver Isolation**: User-space drivers with limited capabilities
- **Memory Protection**: Page-level protection and guard pages
- **Capability System**: Fine-grained permission control

### Threat Mitigation
- **Buffer Overflow Protection**: Stack canaries and bounds checking
- **Privilege Escalation Prevention**: Strict capability checking
- **Denial of Service Protection**: Resource limits and rate limiting
- **Crash Containment**: Driver crashes don't affect kernel

### Recovery Mechanisms
- **Process Failure Recovery**: Automatic restart of critical processes
- **Driver Recovery**: Automatic driver restart on failure
- **System Recovery**: Safe mode and emergency shell
- **Debug Support**: Crash dumps and error logging

## Memory Layout

### Physical Memory
```
0x00000000 - 0x000003FF : IVT and BIOS data
0x00000400 - 0x000004FF : BIOS Data Area (BDA)
0x00000500 - 0x000007FF : Stage 1 bootloader stack
0x00000800 - 0x00008FFF : Stage 2 bootloader
0x00009000 - 0x00009FFF : Boot information structure
0x00100000 - 0x001FFFFF : Kernel image (1MB)
0x00200000 - 0x002FFFFF : Kernel heap and stacks
0x00300000 - 0x00FFFFFF : User-space processes
```

### Virtual Memory
- **Kernel Space**: 0xC0000000 - 0xFFFFFFFF (1GB)
- **User Space**: 0x00000000 - 0xBFFFFFFF (3GB)
- **Page Size**: 4KB
- **Page Tables**: Two-level hierarchy

## Interrupt Handling

### Interrupt Vectors
- **0x00-0x1F**: CPU exceptions (divide by zero, page fault, etc.)
- **0x20-0x2F**: Hardware interrupts (timer, keyboard, etc.)
- **0x30-0x3F**: Software interrupts (system calls)
- **0x40-0xFF**: Reserved for future expansion

### Interrupt Flow
1. Hardware interrupt occurs
2. CPU pushes state and jumps to ISR
3. HAL handles minimal hardware-specific processing
4. Kernel routes to appropriate handler
5. Process execution resumes

## Development Guidelines

### Code Organization
- **Modular Design**: Clear separation between components
- **Minimal Dependencies**: Avoid circular dependencies
- **Consistent Naming**: Follow naming conventions
- **Documentation**: Comprehensive code documentation

### Security Considerations
- **Input Validation**: Validate all inputs and parameters
- **Least Privilege**: Grant minimal necessary permissions
- **Error Handling**: Graceful failure modes
- **Audit Logging**: Security-relevant operations logged

### Performance Considerations
- **Educational Focus**: Clarity over optimization
- **Reasonable Performance**: Acceptable for educational use
- **Resource Limits**: Manage memory and CPU usage
- **Scalability**: Design for future enhancements

## Future Enhancements

### Phase 2 
- Extended filesystem support (ext2, ext3)
- Basic networking stack (TCP/IP)
- Network drivers and services
- Enhanced security features

### Phase 3 
- GUI system support
- Multi-core processor support
- 64-bit architecture support
- Advanced performance optimization

### Long-term Goals
- Production-ready features
- Comprehensive testing suite
- Performance benchmarking
- Application ecosystem development
