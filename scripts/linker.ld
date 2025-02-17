ENTRY(_start)

SECTIONS {
    . = 1M;               /* Kernel load address */

    .text : ALIGN(4K) {
        *(.multiboot)    /* Multiboot2 Section (Must be first) */
        *(.text*)
    }

    .rodata : ALIGN(4K) {
        *(.rodata*)
    }

    .data : ALIGN(4K) {
        *(.data*)
    }

    .bss : ALIGN(4K) {
        *(COMMON)
        *(.bss*)
    }

    /DISCARD/ : {
        *(.eh_frame)
        *(.note*)
    }
}
