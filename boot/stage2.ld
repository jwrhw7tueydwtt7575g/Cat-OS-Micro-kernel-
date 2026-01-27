/* Stage 2 Bootloader Linker Script */

ENTRY(stage2_main)

SECTIONS
{
    . = 0x1000;  /* Load at 0x1000 */
    
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
