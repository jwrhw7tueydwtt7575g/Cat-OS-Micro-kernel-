# MiniSecureOS Build System
# Cross-compiler and tools
CC = i686-linux-gnu-gcc
CFLAGS = -std=c99 -ffreestanding -fno-stack-protector -fno-pic -O2 -fcf-protection=none
CFLAGS += -Wall -Wextra -Werror -nostdlib -nostdinc -Iinclude
AS = nasm
ASFLAGS = -f elf32
LD = i686-linux-gnu-ld
LDFLAGS = -m elf_i386 -nostdlib

# Directories
BOOT_DIR = boot
KERNEL_DIR = kernel
HAL_DIR = hal
DRIVER_DIR = drivers
USER_DIR = userspace
INCLUDE_DIR = include
BUILD_DIR = build

# Files
BOOT_STAGE1 = $(BUILD_DIR)/stage1.bin
BOOT_STAGE2 = $(BUILD_DIR)/stage2.bin
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
DISK_IMAGE = $(BUILD_DIR)/disk.img

# User program object files
USER_OBJS = \
	$(BUILD_DIR)/userspace/userspace.o

# User programs
USER_PROGRAMS = \
	$(BUILD_DIR)/userspace/init.bin \
	$(BUILD_DIR)/userspace/shell.bin \
	$(BUILD_DIR)/userspace/monitor.bin

# Drivers
DRIVER_BINS = $(BUILD_DIR)/drivers/keyboard.bin $(BUILD_DIR)/drivers/console.bin $(BUILD_DIR)/drivers/timer.bin

# All targets
all: $(DISK_IMAGE)

# Bootloader
$(BOOT_STAGE1): $(BOOT_DIR)/stage1.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) -f bin -o $@ $<

$(BOOT_STAGE2): $(BOOT_DIR)/stage2_stub.bin $(BOOT_DIR)/stage2_c.bin
	cat $(BOOT_DIR)/stage2_stub.bin $(BOOT_DIR)/stage2_c.bin > $@
	@SIZE=$$(stat -f%z "$@" 2>/dev/null || stat -c%s "$@"); \
	if [ $$SIZE -gt 4096 ]; then \
		echo "Error: stage2.bin ($$SIZE bytes) exceeds 4KB limit"; \
		exit 1; \
	fi

$(BOOT_DIR)/stage2_stub.bin: $(BOOT_DIR)/stage2_stub.asm
	$(AS) -f bin -o $@ $<

$(BOOT_DIR)/stage2_c.bin: $(BOOT_DIR)/stage2_c.c $(BOOT_DIR)/stage2_c.ld
	$(CC) $(CFLAGS) -c -o $(BOOT_DIR)/stage2_c.o $(BOOT_DIR)/stage2_c.c
	$(LD) -m elf_i386 --oformat binary -T $(BOOT_DIR)/stage2_c.ld $(BOOT_DIR)/stage2_c.o -o $@

# Kernel
KERNEL_OBJS = \
	$(BUILD_DIR)/kernel/entry.o \
	$(BUILD_DIR)/kernel/main.o \
	$(BUILD_DIR)/kernel/scheduler.o \
	$(BUILD_DIR)/kernel/memory.o \
	$(BUILD_DIR)/kernel/ipc.o \
	$(BUILD_DIR)/kernel/syscall.o \
	$(BUILD_DIR)/kernel/capability.o \
	$(BUILD_DIR)/kernel/process.o \
	$(BUILD_DIR)/kernel/interrupt.o

HAL_OBJS = \
	$(BUILD_DIR)/hal/cpu.o \
	$(BUILD_DIR)/hal/io.o \
	$(BUILD_DIR)/hal/timer.o \
	$(BUILD_DIR)/hal/pic.o \
	$(BUILD_DIR)/hal/gdt.o

$(KERNEL_ELF): $(KERNEL_OBJS) $(HAL_OBJS) $(KERNEL_DIR)/kernel.ld
	$(LD) $(LDFLAGS) -T $(KERNEL_DIR)/kernel.ld -o $@ $(KERNEL_OBJS) $(HAL_OBJS)

$(BUILD_DIR)/kernel.bin: $(KERNEL_ELF)
	objcopy -O binary $(KERNEL_ELF) $@

# Kernel object files
$(BUILD_DIR)/kernel/%.o: $(KERNEL_DIR)/%.c
	@mkdir -p $(BUILD_DIR)/kernel
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/hal/%.o: $(HAL_DIR)/%.c
	@mkdir -p $(BUILD_DIR)/hal
	$(CC) $(CFLAGS) -c -o $@ $<

# Kernel assembly entry point
$(BUILD_DIR)/kernel/entry.o: $(KERNEL_DIR)/entry.asm
	@mkdir -p $(BUILD_DIR)/kernel
	nasm -f elf32 -o $@ $<

# User programs
$(BUILD_DIR)/userspace/%.o: $(USER_DIR)/%.c
	@mkdir -p $(BUILD_DIR)/userspace
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/userspace/init.bin: $(BUILD_DIR)/userspace/init.o $(BUILD_DIR)/userspace/userspace.o
	$(LD) $(LDFLAGS) --oformat binary -T $(USER_DIR)/user.ld -o $@ $^

$(BUILD_DIR)/userspace/shell.bin: $(BUILD_DIR)/userspace/shell.o $(BUILD_DIR)/userspace/userspace.o
	$(LD) $(LDFLAGS) --oformat binary -T $(USER_DIR)/user.ld -o $@ $(BUILD_DIR)/userspace/shell.o $(BUILD_DIR)/userspace/userspace.o

$(BUILD_DIR)/userspace/monitor.bin: $(BUILD_DIR)/userspace/monitor.o $(BUILD_DIR)/userspace/userspace.o
	$(LD) $(LDFLAGS) --oformat binary -T $(USER_DIR)/user.ld -o $@ $(BUILD_DIR)/userspace/monitor.o $(BUILD_DIR)/userspace/userspace.o

# Drivers
$(BUILD_DIR)/drivers/%.o: $(DRIVER_DIR)/%.c
	@mkdir -p $(BUILD_DIR)/drivers
	$(CC) $(CFLAGS) -c -o $@ $<


$(BUILD_DIR)/drivers/keyboard.bin: $(BUILD_DIR)/drivers/keyboard.o $(BUILD_DIR)/drivers/driver_manager.o $(BUILD_DIR)/userspace/userspace.o
	$(LD) $(LDFLAGS) -T $(DRIVER_DIR)/driver.ld -o $@ $^

$(BUILD_DIR)/drivers/console.bin: $(BUILD_DIR)/drivers/console.o $(BUILD_DIR)/drivers/driver_manager.o $(BUILD_DIR)/userspace/userspace.o
	$(LD) $(LDFLAGS) -T $(DRIVER_DIR)/driver.ld -o $@ $^

$(BUILD_DIR)/drivers/timer.bin: $(BUILD_DIR)/drivers/timer.o $(BUILD_DIR)/drivers/driver_manager.o $(BUILD_DIR)/userspace/userspace.o
	$(LD) $(LDFLAGS) -T $(DRIVER_DIR)/driver.ld -o $@ $^

$(BUILD_DIR)/drivers/driver_manager.o: $(DRIVER_DIR)/driver_manager.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Disk image
$(DISK_IMAGE): $(BOOT_STAGE1) $(BOOT_STAGE2) $(BUILD_DIR)/kernel.bin $(USER_PROGRAMS) $(DRIVER_BINS)
	@dd if=/dev/zero of=$@ bs=1024 count=1440 2>/dev/null
	@dd if=$(BOOT_STAGE1) of=$@ bs=512 count=1 conv=notrunc 2>/dev/null
	@dd if=$(BOOT_STAGE2) of=$@ bs=512 seek=1 conv=notrunc 2>/dev/null
	@dd if=$(BUILD_DIR)/kernel.bin of=$@ bs=512 seek=10 conv=notrunc 2>/dev/null
	@dd if=$(BUILD_DIR)/userspace/init.bin of=$@ bs=512 seek=74 conv=notrunc 2>/dev/null
	@dd if=$(BUILD_DIR)/drivers/keyboard.bin of=$@ bs=512 seek=138 conv=notrunc 2>/dev/null
	@dd if=$(BUILD_DIR)/drivers/console.bin of=$@ bs=512 seek=202 conv=notrunc 2>/dev/null
	@dd if=$(BUILD_DIR)/drivers/timer.bin of=$@ bs=512 seek=266 conv=notrunc 2>/dev/null
	@dd if=$(BUILD_DIR)/userspace/shell.bin of=$@ bs=512 seek=330 conv=notrunc 2>/dev/null
	@dd if=$(BUILD_DIR)/userspace/monitor.bin of=$@ bs=512 seek=394 conv=notrunc 2>/dev/null

# Run in QEMU
run: $(DISK_IMAGE)
	qemu-system-i386 -fda $< -nographic -serial mon:stdio -no-reboot

# Clean
clean:
	rm -rf $(BUILD_DIR)

# Test
test: $(DISK_IMAGE)
	@echo "Running basic tests..."
	@echo "Test suite not yet implemented"

.PHONY: all clean test run
