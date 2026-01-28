; MiniSecureOS Stage 2 Bootloader - Pure Assembly Test
; Simple 16-bit code to verify disk read works

[BITS 16]
[ORG 0x1000]

stage2_main:
    ; Setup segments
    mov ax, cs
    mov ds, ax
    mov es, ax
    
    ; Clear screen first
    mov ax, 0x0003  ; Clear screen, set 80x25 text mode
    int 0x10
    
    ; Set cursor position
    mov ah, 0x02
    mov bh, 0
    mov dx, 0x0000  ; Row 0, Col 0
    int 0x10
    
    ; Print success message
    mov si, success_msg
    call print_string
    
    ; Wait for keypress
    mov ah, 0x00
    int 0x16
    
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
    mov bh, 0
    int 0x10
    jmp print_string
.done:
    ret

success_msg: db 'Stage 2: DISK READ SUCCESS!', 13, 10, 0

; Pad to 512 bytes
times 512-($-$$) db 0
