; MiniSecureOS Stage 2 Entry Stub - WORKING VERSION
; This stub transitions from 16-bit real mode to 32-bit protected mode
; Then calls the C code that was concatenated after this binary

[BITS 16]
[ORG 0]

stage2_main:
    ; Setup segments - we're loaded at 0x1000:0x0000
    mov ax, 0x1000
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x9000
    
    ; Print startup message
    mov si, stage2_msg
    call print_string
    
    ; Enable A20 via fast method
    in al, 0x92
    or al, 2
    out 0x92, al
    
    mov si, a20_msg
    call print_string
    
    ; Load GDT
    lgdt [gdt_ptr]
    
    mov si, gdt_msg
    call print_string
    
    ; Enter protected mode
    cli
    mov eax, cr0
    or al, 1
    mov cr0, eax
    
    ; Far jump to clear pipeline and load CS
    ; Use absolute address: 0x10000 + offset to protected_mode_entry
    jmp 0x08:0x10050    ; Adjust 0x10050 based on actual offset

[BITS 32]
protected_mode_entry:
    ; Setup segment registers for protected mode
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs: ax
    mov ss, ax
    
    ; Setup stack
    mov esp, 0x90000
    mov ebp, esp
    
    ; Call C code at offset 512 bytes (0x200) from stage2 start
    ; Absolute address: 0x10000 + 0x200 = 0x10200
    call 0x10200
    
    ; Should never return, but halt if it does
.hang:
    hlt
    jmp .hang

[BITS 16]
print_string:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print_string
.done:
    ret

; Data
stage2_msg db 'MiniSecureOS Stage 2 Entry Stub', 13, 10, 0
a20_msg db 'A20 line enabled', 13, 10, 0
gdt_msg db 'GDT loaded, entering protected mode...', 13, 10, 0

; GDT
align 8
gdt:
    dq 0                    ; Null descriptor
    
    ; Code segment
    dw 0xFFFF               ; Limit low
    dw 0x0000               ; Base low
    db 0x00                 ; Base mid
    db 0x9A                 ; Access: present, ring 0, code, readable
    db 0xCF                 ; Flags: 4KB granularity, 32-bit
    db 0x00                 ; Base high
    
    ; Data segment
    dw 0xFFFF               ; Limit low
    dw 0x0000               ; Base low
    db 0x00                 ; Base mid
    db 0x92                 ; Access: present, ring 0, data, writable
    db 0xCF                 ; Flags: 4KB granularity, 32-bit
    db 0x00                 ; Base high

align 4
gdt_ptr:
    dw gdt_ptr - gdt - 1    ; Limit
    dd 0x10000 + gdt        ; Base (physical address)

; Pad to 512 bytes
times 512-($-$$) db 0
