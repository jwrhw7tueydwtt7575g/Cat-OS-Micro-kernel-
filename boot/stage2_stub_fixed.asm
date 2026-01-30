; MiniSecureOS Stage 2 Entry Stub - COMPLETE FIXED VERSION
; Properly switches to protected mode and calls C code

[BITS 16]
; Stage2 loads at 0x1000:0000 = 0x10000 physical

; Constants
C_CODE_OFFSET equ 0x200  ; C code starts 512 bytes after stub

stage2_main:
    ; Print message using BIOS to confirm we're here
    mov si, stage2_msg
    call print_string
    
    ; Enable A20 line using fast method
    in al, 0x92
    or al, 0x02
    out 0x92, al
    
    ; Write 'A' after A20 enable:
    mov byte [0xB8000], 'A'
    mov byte [0xB8001], 0x0F
    
    mov si, a20_msg
    call print_string
    
    ; Load GDT with proper physical address
    lgdt [gdt_ptr]
    
    ; Write 'G' after GDT load:
    mov byte [0xB8002], 'G'
    mov byte [0xB8003], 0x0F
    
    mov si, gdt_msg
    call print_string
    
    ; Enable protected mode
    cli                          ; Disable interrupts
    mov eax, cr0
    or al, 1                     ; Set PE bit
    mov cr0, eax
    
    ; Write 'M' before far jump:
    mov byte [0xB8006], 'M'
    mov byte [0xB8007], 0x0F
    
    ; CRITICAL: Use absolute far jump to protected mode entry
    ; Calculate: stage2 at 0x10000, pm_start is at offset ~0x50
    jmp dword 0x08:0x10050      ; Absolute address to pm_start

[BITS 32]
align 16
pm_start:
    ; Write 'P' after entering protected mode:
    mov byte [0xB8004], 'P'
    mov byte [0xB8005], 0x0F
    
    ; Set all segment registers to data selector (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Set up stack at safe location
    mov esp, 0x90000
    mov ebp, 0x90000
    
    ; VERY VISIBLE TEST - Fill screen with blue from protected mode assembly
    mov edi, 0xB8000
    mov eax, 0x10002020  ; Blue background, space
    mov ecx, 80 * 25
    cld
    rep stosd
    
    ; Call C code at absolute address
    ; stage2 at 0x10000 + 512 bytes stub = 0x10200
    call 0x10200
    
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

; GDT - properly aligned
align 8
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
    dw gdt_ptr - gdt - 1    ; Limit (size - 1)
    dd 0x10000 + (gdt - $$)  ; Base address - stage2 loads at 0x10000

CODE_SEL equ 0x08
DATA_SEL equ 0x10

; Messages
stage2_msg db 'MiniSecureOS Stage 2 Entry Stub', 13, 10, 0
a20_msg db 'A20 line enabled', 13, 10, 0
gdt_msg db 'GDT loaded, entering protected mode...', 13, 10, 0

; Pad to fill sector
times 512-($-$$) db 0
