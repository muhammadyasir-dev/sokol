OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)

SECTIONS
{
    . = 0x1000; /* Load the kernel at this address */
    .text : { *(.text) }
    .bss : { *(.bss) }
}
