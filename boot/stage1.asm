; MiniSecureOS Stage 1 Bootloader
; MBR - 512 bytes, loaded by BIOS at 0x7C00

[BITS 16]
[ORG 0x7C00]

start:
    ; Clear segment registers
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00      ; Stack grows downward

    ; Save boot drive
    mov [boot_drive], dl

    ; Display boot message
    mov si, boot_msg
    call print_string

    ; Load Stage 2 bootloader
    mov ax, 0x1000      ; Load at 0x1000:0000
    mov es, ax
    xor bx, bx          ; Offset 0

    mov ah, 0x02        ; Read sectors
    mov al, 8           ; Read 8 sectors (4KB)
    mov ch, 0           ; Cylinder 0
    mov cl, 2           ; Sector 2 (after MBR)
    mov dh, 0           ; Head 0
    mov dl, [boot_drive]
    int 0x13            ; BIOS disk interrupt
    jc disk_error       ; Carry flag set = error

    ; Jump to Stage 2
    mov ax, 0x1000
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov sp, 0x9000      ; New stack

    jmp 0x1000:0x0000   ; Jump to Stage 2

disk_error:
    mov si, disk_error_msg
    call print_string
    jmp hang

print_string:
    lodsb               ; Load byte from SI into AL
    cmp al, 0
    je print_done
    mov ah, 0x0E        ; BIOS teletype output
    int 0x10
    jmp print_string
print_done:
    ret

hang:
    cli                 ; Clear interrupts
    hlt                 ; Halt CPU
    jmp hang

; Data section
boot_drive: db 0
boot_msg: db 'MiniSecureOS Stage 1 Bootloader', 13, 10, 0
disk_error_msg: db 'Disk error loading Stage 2', 13, 10, 0

; Padding to 512 bytes
times 510-($-$$) db 0
dw 0xAA55              ; MBR signature
