/* Kernel Linker Script */

ENTRY(kernel_main)

SECTIONS
{
    . = 0x100000;  /* Load at 1MB */
    
    .text :
    {
        *(.text)
        *(.rodata*)
        . = ALIGN(4096);
    }
    
    .data :
    {
        *(.data)
        . = ALIGN(4096);
    }
    
    .bss :
    {
        __bss_start = .;
        *(.bss)
        *(COMMON)
        __bss_end = .;
        . = ALIGN(4096);
    }
    
    . = ALIGN(4096);
    __end = .;
    
    /DISCARD/ :
    {
        *(.eh_frame)
        *(.comment)
    }
}
