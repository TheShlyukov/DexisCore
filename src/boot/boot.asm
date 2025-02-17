; Multiboot2 header for GRUB
section .multiboot
header_start:
    dd 0xe85250d6          ; Magic number (Multiboot2)
    dd 0                   ; Arch: 0 = x86
    dd header_end - header_start ; Header length
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start)) ; Control sum
    ; Тег конца (END tag)
    dw 0                   ; Type = 0 (end)
    dw 0                   ; Flags = 0
    dd 8                   ; Size = 8
header_end:

; Entering 64-bit mode
section .text
global _start
_start:
    ; Sett up stack (Simplified)
    mov esp, stack_top

    ; Calling C code
    extern kmain
    call kmain

    ; If kmain returns — halt
halt:
    cli
    hlt

section .bss
align 16
stack_bottom:
    resb 16384            ; 16 КБ stack
stack_top:
