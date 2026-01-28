; MiniSecureOS Stage 2 Bootloader Entry Point
; 16-bit entry point that transitions to 32-bit protected mode

[BITS 16]
[ORG 0x1000]

stage2_entry:
    ; Save boot drive
    mov [boot_drive], dl
    
    ; Setup segments
    mov ax, cs
    mov ds, ax
    mov es, ax
    
    ; Print entry message
    mov si, entry_msg
    call print_string
    
    ; Call 32-bit stage2_main
    call stage2_main
    
    ; Halt if we return
    cli
    hlt
    jmp $

; Include 32-bit code
[BITS 32]

; External C function
extern stage2_main

; 16-bit print function
print_string:
    lodsb
    cmp al, 0
    je print_done
    mov ah, 0x0E
    int 0x10
    jmp print_string
print_done:
    ret

; Data section
boot_drive: db 0
entry_msg: db 'Stage 2 entry point reached', 13, 10, 0

; Pad to 512 bytes
times 512-($-$$) db 0
