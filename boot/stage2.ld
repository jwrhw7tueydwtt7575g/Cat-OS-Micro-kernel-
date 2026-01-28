/* Stage 2 Bootloader Linker Script */

ENTRY(stage2_main)

SECTIONS
{
    . = 0x10000;  /* Load at 0x10000 (0x1000:0000 in real mode) */
    
    .text :
    {
        *(.text)
        *(.rodata*)
    }
    
    .data :
    {
        *(.data)
    }
    
    .bss :
    {
        *(.bss)
    }
    
    /DISCARD/ :
    {
        *(.eh_frame)
        *(.comment)
    }
}
