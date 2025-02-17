#include <kernel/vga.h>
#include <kernel/io.h>
#include <kernel/dsh.h>

static void serial_write(const char* str) {
    while (*str) {
        outb(0x3F8, *str++);  // COM1 port
    }
}

void kmain(void) {
    terminal_initialize(); // Initialize terminal
    terminal_write("*DexisCore v0.1*\n");
    terminal_write("Architecture: x86 (32bit)\n");
    serial_write("\nKernel loaded and running\n");
    dsh_run(); // Run the dsh shell
    while (1) {} // Loop forever
}