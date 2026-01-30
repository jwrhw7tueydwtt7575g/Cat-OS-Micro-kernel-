; MiniSecureOS Stage 2 Bootloader - CORRECTED
[BITS 16]
[ORG 0]

LOAD_ADDR equ 0x10000

start:
    ; Setup segments
    mov ax, 0x1000
    mov ds, ax
    
    ; Save boot drive (passed in DL from Stage 1)
    mov [boot_drive], dl
    
    mov es, ax
    mov ss, ax
    mov sp, 0x9000
    
    ; Print message
    mov si, msg1
    call print
    
    ; Load Kernel into memory buffer (Real Mode)
    ; We load it to 0x2000:0000 (physical 0x20000)
    
    mov si, msg_load_kernel
    call print
    
    mov ax, 0x2000      ; Segment 0x2000
    mov es, ax
    xor bx, bx
    
    ; Read 512 sectors using single-sector reads (Slow but robust for Floppy)
    mov cx, 512
    mov bp, 10          ; Start LBA 10
    
read_loop:
    push cx             ; Save loop count
    
    ; LBA to CHS
    mov ax, bp
    mov bl, 18
    div bl              ; AH = (LBA % 18), AL = (LBA / 18)
    
    mov cl, ah
    inc cl              ; Sector = (LBA % 18) + 1
    
    mov dh, al
    and dh, 1           ; Head = (LBA / 18) % 2
    
    mov ch, al
    shr ch, 1           ; Cylinder = (LBA / 18) / 2
    
    ; Read 1 sector
    mov ax, 0x0201      ; AH=02, AL=1
    mov dl, [boot_drive]
    xor bx, bx
    int 0x13
    jc disk_error
    
    ; Update Segment
    mov ax, es
    add ax, 32          ; Add 512 bytes (32 paragraphs)
    mov es, ax
    
    ; Next LBA
    inc bp
    
    pop cx              ; Restore loop count
    loop read_loop
    
    mov si, msg_kernel_loaded
    call print
    
    ; Enable A20
    in al, 0x92
    or al, 2
    out 0x92, al
    
    mov si, msg2
    call print
    
    ; Load GDT
    lgdt [gdt_ptr]
    
    mov si, msg3
    call print
    
    ; Enter protected mode
    cli
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far jump to protected-mode entry using trampoline selector 0x18
    ; Selector 0x18 has Base 0x10000, Limit 4GB, 32-bit.
    ; pm_entry is an offset relative to ORG 0 (start of file).
    ; Since we are at 0x10000 physical, 0x18:pm_entry will jump to 0x10000 + pm_entry.
    jmp 0x18:pm_entry

; CRITICAL: [BITS 32] must come BEFORE the label
[BITS 32]
pm_entry:
    ; We are now in 32-bit mode, but CS base is 0x10000 (Selector 0x18).
    ; We need to switch to Selector 0x08 (Base 0).
    ; We must jump to linear address (LOAD_ADDR + .pm_flat).
    jmp 0x08:(LOAD_ADDR + .pm_flat)
.pm_flat:
    ; Now CS is 0x08 (Base 0), EIP is correctly pointing to code.

    ; Debug: Output 'P' to serial
    mov dx, 0x3F8
    mov al, 'P'
    out dx, al

    ; Setup segments (now assembles as 32-bit)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ; Load SS and setup protected-mode stack
    mov ss, ax
    mov esp, 0x90000
    mov ebp, esp
    
    ; Write 'PM' to VGA
    mov edi, 0xB8000
    mov dword [edi], 0x0F500F4D
    
    ; Call C code
    ; Call the C entry point which is located at LOAD_ADDR + 0x200
    mov eax, LOAD_ADDR + 0x200
    call eax
    
    ; If C code returns, hang
    cli
    hlt

disk_error:
    mov si, msg_disk_error
    call print
    cli
    hlt
    
    ; Hang
    cli
.hang:
    hlt
    jmp .hang

[BITS 16]
print:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print
.done:
    ret

msg1 db 'Stage2', 13, 10, 0
msg2 db 'A20', 13, 10, 0
msg3 db 'PM', 13, 10, 0
msg_load_kernel db 'Load K', 13, 10, 0
msg_kernel_loaded db 'K OK', 13, 10, 0
msg_disk_error db 'DiskErr', 13, 10, 0
boot_drive db 0


align 8
gdt:
    dq 0                ; NULL descriptor
    dw 0xFFFF, 0x0000   ; 0x08 Code: limit low, base low (Flat 4GB)
    db 0x00, 0x9A, 0xCF, 0x00  ; Code: base mid, access, granularity, base high
    dw 0xFFFF, 0x0000   ; 0x10 Data: limit low, base low (Flat 4GB)
    db 0x00, 0x92, 0xCF, 0x00  ; Data: base mid, access, granularity, base high
    
    ; Trampoline descriptor 0x18
    dw 0xFFFF, 0x0000   ; Limit low, Base low (0x0000)
    db 0x01, 0x9A, 0xCF, 0x00  ; Base mid (0x01 = 0x10000 >> 16), Access, Granularity, Base high
    ; Base = 0x010000 = 0x10000. Limit = 0xFFFFF * 4KB = 4GB.

gdt_end:

gdt_ptr:
    dw gdt_end - gdt - 1
    dd LOAD_ADDR + gdt

; Minimal IDT (all zeros) and pointer
align 4
idt:
    times 8 db 0
idt_end:

idt_ptr:
    dw idt_end - idt - 1
    dd LOAD_ADDR + idt

times 512-($-$$) db 0