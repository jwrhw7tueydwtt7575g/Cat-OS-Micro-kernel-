/* User Program Linker Script */

ENTRY(_start)

SECTIONS
{
    . = 0x00400000;  /* Load at 4MB */
    
    .text :
    {
        KEEP(*(.text.entry))
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
