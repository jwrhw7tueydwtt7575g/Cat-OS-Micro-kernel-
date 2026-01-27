# MiniSecureOS Installation Guide

## Prerequisites

### Required Tools
- **GCC Cross-Compiler**: i686-elf-gcc for 32-bit x86 target
- **NASM**: Netwide Assembler for assembly code
- **QEMU**: Emulator for testing
- **Make**: Build system
- **Git**: Version control (optional)

### Ubuntu/Debian Installation
```bash
# Install basic tools
sudo apt update
sudo apt install build-essential nasm qemu-system-x86 make git

# Install cross-compiler (if not available)
# You may need to build from source or use pre-built binaries
```

### Building Cross-Compiler (if needed)
```bash
# Download binutils and GCC source
wget https://ftp.gnu.org/gnu/binutils/binutils-2.38.tar.gz
wget https://ftp.gnu.org/gnu/gcc/gcc-11.2.0/gcc-11.2.0.tar.gz

# Extract and build binutils
tar -xzf binutils-2.38.tar.gz
mkdir build-binutils
cd build-binutils
../binutils-2.38/configure --target=i686-elf --prefix=/opt/cross
make -j$(nproc)
sudo make install
cd ..

# Extract and build GCC
tar -xzf gcc-11.2.0.tar.gz
mkdir build-gcc
cd build-gcc
../gcc-11.2.0/configure --target=i686-elf --prefix=/opt/cross --without-headers --with-newlib --disable-nls --enable-languages=c,c++
make all-gcc -j$(nproc)
make all-target-libgcc -j$(nproc)
sudo make install-gcc
sudo make install-target-libgcc
cd ..

# Add to PATH
export PATH="/opt/cross/bin:$PATH"
echo 'export PATH="/opt/cross/bin:$PATH"' >> ~/.bashrc
```

## Building MiniSecureOS

### 1. Clone Repository
```bash
git clone <repository-url> MiniSecureOS
cd MiniSecureOS
```

### 2. Configure Build Environment
```bash
# Ensure cross-compiler is in PATH
which i686-elf-gcc
# Should output: /opt/cross/bin/i686-elf-gcc or similar

# Verify tools
i686-elf-gcc --version
nasm --version
make --version
```

### 3. Build System
```bash
# Clean any previous builds
make clean

# Build complete system
make all

# This will create:
# - build/disk.img: Bootable disk image
# - build/stage1.bin: Stage 1 bootloader
# - build/stage2.bin: Stage 2 bootloader
# - build/kernel.elf: Kernel executable
# - User programs and drivers
```

### 4. Run in Emulator
```bash
# Run with QEMU
make run

# Or manually:
qemu-system-i386 -fda build/disk.img -monitor stdio
```

## Project Structure

```
MiniSecureOS/
├── boot/               # Bootloader code
│   ├── stage1.asm      # MBR bootloader (512 bytes)
│   ├── stage2.c        # Protected mode setup
│   └── stage2.ld       # Linker script
├── kernel/             # Kernel source
│   ├── main.c          # Kernel entry point
│   ├── scheduler.c     # Process scheduler
│   ├── memory.c        # Memory manager
│   ├── ipc.c           # Inter-process communication
│   ├── syscall.c       # System call dispatcher
│   ├── capability.c    # Security manager
│   ├── process.c       # Process management
│   ├── interrupt.c     # Interrupt handling
│   └── kernel.ld       # Kernel linker script
├── hal/                # Hardware abstraction layer
│   ├── cpu.c           # CPU control
│   ├── io.c            # I/O port access
│   ├── timer.c         # Timer management
│   └── pic.c           # PIC controller
├── drivers/            # User-space drivers
│   ├── keyboard.c      # Keyboard driver
│   ├── console.c       # Console driver
│   ├── timer.c         # Timer driver
│   ├── driver_manager.c # Driver management
│   └── driver.ld       # Driver linker script
├── userspace/          # User programs
│   ├── init.c          # Init process (PID 1)
│   ├── shell.c         # Shell program
│   ├── monitor.c       # System monitor
│   └── user.ld         # User program linker script
├── include/            # Header files
│   ├── types.h         # Basic types
│   ├── kernel.h        # Kernel interfaces
│   ├── hal.h           # HAL interfaces
│   ├── driver.h        # Driver interfaces
│   └── userspace.h     # User-space APIs
├── tests/              # Test framework
│   └── test_framework.c
├── docs/               # Documentation
│   ├── ARCHITECTURE.md
│   └── API_REFERENCE.md
├── Makefile            # Build system
└── README.md           # Project overview
```

## Build Targets

### Available Make Targets
```bash
make all          # Build complete system
make clean        # Clean build artifacts
make run          # Build and run in QEMU
make test         # Run tests (when implemented)
```

### Build Outputs
- `build/disk.img`: Bootable floppy disk image (1.44MB)
- `build/stage1.bin`: MBR bootloader
- `build/stage2.bin`: Protected mode bootloader
- `build/kernel.elf`: Kernel executable
- `build/userspace/*.bin`: User programs
- `build/drivers/*.bin`: User-space drivers

## Running MiniSecureOS

### In QEMU
```bash
# Basic run
make run

# With debug options
qemu-system-i386 -fda build/disk.img -monitor stdio -d int,cpu

# With GDB debugging
qemu-system-i386 -fda build/disk.img -s -S
# In another terminal:
i686-elf-gdb build/kernel.elf
(gdb) target remote localhost:1234
(gdb) break kernel_main
(gdb) continue
```

### On Real Hardware
```bash
# Write to floppy disk
sudo dd if=build/disk.img of=/dev/fd0 bs=1024

# Or create USB boot device
sudo dd if=build/disk.img of=/dev/sdX bs=1024  # Replace sdX with device
```

## Troubleshooting

### Common Issues

#### Cross-Compiler Not Found
```bash
# Error: i686-elf-gcc: command not found
# Solution: Install cross-compiler or add to PATH
export PATH="/opt/cross/bin:$PATH"
```

#### Build Failures
```bash
# Clean and rebuild
make clean
make all

# Check for missing dependencies
i686-elf-gcc --version
nasm --version
```

#### QEMU Issues
```bash
# Install QEMU system emulator
sudo apt install qemu-system-x86

# Check QEMU version
qemu-system-i386 --version
```

#### Boot Issues
```bash
# Check disk image
ls -la build/disk.img
file build/disk.img

# Verify bootloader size
ls -la build/stage1.bin  # Should be 512 bytes
```

### Debug Tips

1. **Enable Debug Output**: Modify kernel_print to show more information
2. **Use QEMU Monitor**: Press Ctrl+A, C to access QEMU monitor
3. **Check Registers**: Use `info registers` in QEMU monitor
4. **Single Step**: Use `stepi` in QEMU monitor or GDB
5. **Memory Dump**: Use `x/10x 0x100000` to view kernel memory

## Development Workflow

### Making Changes
1. Edit source files
2. Run `make clean && make all`
3. Test with `make run`
4. Debug issues as needed
5. Repeat

### Adding New Features
1. Design interface in include/
2. Implement in appropriate module
3. Add tests if needed
4. Update documentation
5. Build and test

### Code Style
- Use 4-space indentation
- Follow existing naming conventions
- Add comments for complex functions
- Update documentation for new APIs

## Getting Help

### Resources
- **Documentation**: docs/ARCHITECTURE.md, docs/API_REFERENCE.md
- **Source Code**: Well-commented implementation
- **Tests**: tests/test_framework.c for usage examples

### Common Questions
- Q: How do I add a new driver?
  A: Create driver in drivers/, register with driver_manager.c
- Q: How do I add a system call?
  A: Add to syscall.c and userspace.h
- Q: How do I debug kernel issues?
  A: Use QEMU with debug options or GDB

## Contributing

### Submitting Changes
1. Fork repository
2. Create feature branch
3. Make changes with tests
4. Update documentation
5. Submit pull request

### Code Review
- Follow existing style
- Add appropriate comments
- Test thoroughly
- Document new features

## License

MiniSecureOS is provided for educational purposes. See LICENSE file for details.
