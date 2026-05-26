ENTRY(start)

SECTIONS
{

    . = 1M;

    .boot :
    {
        KEEP(*(.multiboot_header))
    }

    .text :
    {
        *(.text)
    }

    .heap ALIGN(0x1000):
    {
        __heap_start = .;          /* This defines the start of the heap */
        . = . + 0x100000;          /* Reserve 1MB for heap (adjust as needed) */
        __heap_end = .;
    }
        
}

