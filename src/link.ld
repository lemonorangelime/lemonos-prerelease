OUTPUT_FORMAT(elf32-i386)
ENTRY(start)
SECTIONS
        {
                . = 0x100000;
                __start__ = ALIGN(8);
                .text : {
                                . = ALIGN(8);
                                KEEP(*(.multiboot))
                                *(.text)
                        }
                .data :{ *(.data) }
                .bss : { *(.bss) }
        }
__end__ = ALIGN(8);
_kernel_start = __start__;
_kernel_size = __end__ - __start__;
_kernel_end = ALIGN(8);
