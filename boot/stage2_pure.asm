; MiniSecureOS Stage 2 Bootloader - Pure Assembly
; 16-bit real mode code that prints to VGA

[BITS 16]
[ORG 0x1000]

stage2_main:
    ; Setup segments
    mov ax, cs
    mov ds, ax
    mov es, ax
    
    ; Clear screen and set video mode
    mov ax, 0x0003
    int 0x10
    
    ; Print success message
    mov si, success_msg
    call print_string
    
    ; Print second message
    mov si, working_msg
    call print_string
    
    ; Halt system
    cli
.halt:
    hlt
    jmp .halt

print_string:
    lodsb
    cmp al, 0
    je .done
    mov ah, 0x0E
    int 0x10
    jmp print_string
.done:
    ret

success_msg: db '=== STAGE 2 WORKING! ===', 13, 10, 0
working_msg: db 'Stage 2: 16-bit mode OK!', 13, 10, 0

; Pad to 512 bytes
times 512-($-$$) db 0
