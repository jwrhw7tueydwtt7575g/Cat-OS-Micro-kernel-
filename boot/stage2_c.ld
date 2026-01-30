/* Simple linker script for stage2 C code */

ENTRY(stage2_c_main)

SECTIONS
{
    . = 0x10200;  /* Load at offset 0x10200 (LOAD_ADDR 0x10000 + stub 512) */
    
    .text :
    {
        *(.text.entry)
        *(.text)
    }
    
    .data :
    {
        *(.data)
    }
    
    .bss :
    {
        *(.bss)
    }
}
