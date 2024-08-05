CODE_BASE   = 0x80000;
DATA_BASE   = 0x400000;
STACK_TOP   = 0x800000;
PT_BASE     = 0x800000;
 
SECTIONS
{
    /* Starts at 0x20000000 (assuming ends at 0x208FFFFF, 9MB). */
    . = CODE_BASE;
    __start = .;
    __text_start = .;
    .text :
    {
        KEEP(*(.vectors))
        KEEP(*(.text.boot))
        *(.text)
    }
    __text_end = .;
 
    . = ALIGN(4096); /* align to page size */
    __data_start = DATA_BASE;
    .data :
    {
        *(.data)
    }
    . = ALIGN(4096); /* align to page size */
    __data_end = .;
 
    __bss_start = .;
    .bss :
    {
        bss = .;
        *(.bss)
    }
    . = ALIGN(4096); /* align to page size */
    __bss_end = .;
    __end = .;

    . = STACK_TOP; /* stack memory */
    stack_top = .;

    . = PT_BASE; /* Page tables */
    .pt :
    {
        *(.pt)
    }
}
__bss_size = (__bss_end - __bss_start)>>3;
