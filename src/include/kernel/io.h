#ifndef KERNEL_IO_H
#define KERNEL_IO_H

#include <stdint.h>

// Reading a byte from a port
static inline uint8_t inb(uint16_t port) {
    uint8_t data;
    __asm__ volatile ("inb %1, %0" : "=a"(data) : "Nd"(port));
    return data;
}

// Writing a byte to a port
static inline void outb(uint16_t port, uint8_t data) {
    __asm__ volatile ("outb %0, %1" : : "a"(data), "Nd"(port));
}

// Forbidden to read from a port
static inline void cli(void) {
    __asm__ volatile ("cli");
}

// Allow interrupts
static inline void sti(void) {
    __asm__ volatile ("sti");
}

// Wait for I/O operations
static inline void io_wait(void) {
    __asm__ volatile ("outb %%al, $0x80" : : "a"(0));
}

#endif // KERNEL_IO_H

// Halt proccesor requested
static inline void hlt(void) {
    __asm__ volatile ("hlt");
}
