[BITS 32]
global _start
extern kernel_main

section .entry
_start:
    ; IMMEDIATE VGA DEBUG - Write 'A' to top-left corner (white on red)
    mov word [0xB8000], 0x4F41  ; 0x4F = white on red, 0x41 = 'A'
    
    ; Write 'S' next to it
    mov word [0xB8002], 0x4F53  ; 'S'
    
    ; Write 'M' next to that  
    mov word [0xB8004], 0x4F4D  ; 'M'
    
    ; 1. Initialize Serial Port (0x3F8) for debugging
    mov dx, 0x3FB   ; Line Control Register
    mov al, 0x80    ; Enable DLAB (Set Baud Rate Divisor)
    out dx, al
    
    mov dx, 0x3F8   ; Divisor LSB
    mov al, 0x03    ; 3 (Lo byte)
    out dx, al
    
    mov dx, 0x3F9   ; Divisor MSB
    mov al, 0x00    ; 0 (Hi byte)
    out dx, al
    
    mov dx, 0x3FB   ; Line Control Register
    mov al, 0x03    ; Disable DLAB, 8 bits, no parity, 1 stop bit
    out dx, al

    ; 2. Set up Stack at 0x90000
    mov esp, 0x90000
    mov ebp, esp
    
    ; Write 'C' to VGA before calling C code
    mov word [0xB8006], 0x4F43  ; 'C'

    ; 3. Call Kernel Main
    call kernel_main

    ; Write 'R' to VGA if we return (shouldn't happen)
    mov word [0xB8008], 0x4F52  ; 'R'

    ; 4. Hang if return from kernel_main
    cli
.hang:
    hlt
    jmp .hang
