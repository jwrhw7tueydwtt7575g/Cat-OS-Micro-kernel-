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

    ; Try disk read multiple times
    mov si, 3          ; Retry count
retry_read:
    mov ah, 0x02        ; Read sectors
    mov al, 8           ; Read 8 sectors (4KB)
    mov ch, 0           ; Cylinder 0
    mov cl, 2           ; Sector 2 (Stage 2 starts at LBA 1 -> Sector 2)
    mov dh, 0           ; Head 0
    mov dl, [boot_drive]
    int 0x13            ; BIOS disk interrupt
    jc disk_error_retry ; Carry flag set = error

    ; Check if we actually read data by looking for known pattern
    ; Since we're using flat binary, just check if we read anything
    cmp byte [es:0], 0    ; Check if first byte is zero (empty)
    jne disk_read_ok      ; Not zero, we have data

disk_error_retry:
    dec si
    jz disk_error       ; Out of retries
    mov ah, 0x00        ; Reset disk system
    int 0x13
    jmp retry_read

disk_read_ok:
    mov si, success_msg
    call print_string
    
    ; Verify we have data at 0x1000:0000
    mov ax, 0x1000
    mov ds, ax
    mov si, debug_msg
    call print_string
    
    ; Show first few bytes of loaded data
    mov al, [0]
    call print_hex
    mov al, ' '
    call print_char
    mov al, [1]
    call print_hex
    mov al, ' '
    call print_char
    mov al, [2]
    call print_hex
    mov al, ' '
    call print_char
    mov al, [3]
    call print_hex
    call print_newline


    ; Setup stack BEFORE jumping to Stage 2
    mov ax, 0x1000
    mov ss, ax
    mov sp, 0x9000      ; New stack

    ; Jump to Stage 2
    mov ax, 0x1000
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

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

print_hex:
    push ax
    push bx
    push cx
    push dx
    
    mov bl, al          ; Save original byte
    shr al, 4           ; Get high nibble
    call hex_digit
    mov al, bl          ; Restore byte
    and al, 0x0F        ; Get low nibble
    call hex_digit
    
    pop dx
    pop cx
    pop bx
    pop ax
    ret

hex_digit:
    add al, '0'
    cmp al, '9'
    jle hex_digit_done
    add al, 7           ; Convert to A-F
hex_digit_done:
    mov ah, 0x0E
    int 0x10
    ret

print_char:
    mov ah, 0x0E
    int 0x10
    ret

print_newline:
    push ax
    mov al, 13
    call print_char
    mov al, 10
    call print_char
    pop ax
    ret

hang:
    cli                 ; Clear interrupts
    hlt                 ; Halt CPU
    jmp hang

; Data section
boot_drive: db 0
boot_msg: db 'MiniSecureOS Stage 1 Bootloader', 13, 10, 0
disk_error_msg: db 'Disk error loading Stage 2', 13, 10, 0
success_msg: db 'Stage 2 loaded successfully', 13, 10, 0
debug_msg: db 'First 3 bytes: ', 0
final_msg: db 'About to jump to Stage 2...', 13, 10, 0
elf_signature: db 0x7F, 'E', 'L', 'F'  ; ELF magic number

; Padding to 512 bytes
times 510-($-$$) db 0
dw 0xAA55              ; MBR signature
