; MiniSecureOS Stage 2 Bootloader - 16-bit assembly version
; Sets up protected mode and jumps to kernel

[BITS 16]
[ORG 0x0000]  ; Stage1 loads at 0x1000:0000, so ORG is offset within segment

stage2_main:
    ; Print message using BIOS
    mov si, stage2_msg
    call print_string
    
    ; Enable A20 line using fast method
    in al, 0x92
    or al, 0x02
    out 0x92, al
    
    mov si, a20_msg
    call print_string
    
    ; Load GDT
    lgdt [gdt_ptr]
    
    mov si, gdt_msg
    call print_string
    
    ; Enable protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    ; Far jump to protected mode
    jmp CODE_SEL:protected_mode_entry

[BITS 32]
protected_mode_entry:
    ; Setup segment registers
    mov ax, DATA_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Setup stack
    mov esp, 0x90000
    
    ; Print success message in protected mode
    mov ebx, protected_msg
    call print_string_vga
    
    ; Load kernel from disk (simplified - just jump to kernel for now)
    ; TODO: Add actual kernel loading
    
    ; Jump to kernel at 1MB
    jmp 0x100000
    
    ; Should never reach here
    cli
    hlt
    jmp $

; Print string (BIOS - 16-bit)
print_string:
    lodsb
    cmp al, 0
    je print_done
    mov ah, 0x0E
    int 0x10
    jmp print_string
print_done:
    ret

; Print string (VGA - 32-bit)
print_string_vga:
    mov edi, 0xB8000 + 160 * 3  ; Line 3
    mov ah, 0x0F  ; White on black
    
.print_loop:
    mov al, [ebx]
    cmp al, 0
    je .print_done
    stosw
    inc ebx
    jmp .print_loop
.print_done:
    ret

; GDT
gdt:
    dd 0  ; Null descriptor
    dd 0
    
    ; Code segment (32-bit)
    dw 0xFFFF     ; Limit
    dw 0x0000     ; Base
    db 0x00       ; Base
    db 0x9A       ; Access
    db 0xCF       ; Granularity
    db 0x00       ; Base
    
    ; Data segment (32-bit)
    dw 0xFFFF     ; Limit
    dw 0x0000     ; Base
    db 0x00       ; Base
    db 0x92       ; Access
    db 0xCF       ; Granularity
    db 0x00       ; Base

gdt_ptr:
    dw gdt_ptr - gdt - 1
    dd gdt

CODE_SEL equ 0x08
DATA_SEL equ 0x10

; Messages
stage2_msg db 'MiniSecureOS Stage 2 Bootloader', 13, 10, 0
a20_msg db 'A20 line enabled', 13, 10, 0
gdt_msg db 'GDT loaded, entering protected mode...', 13, 10, 0
protected_msg db 'Protected mode entered successfully!', 0

; Pad to fill sector
times 512-($-$$) db 0
