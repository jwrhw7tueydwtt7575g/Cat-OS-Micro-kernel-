/* Driver Linker Script */

ENTRY(main)

SECTIONS
{
    . = 0x00800000;  /* Load at 8MB */
    
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
