# Makefile Settings
CC = gcc
ASM = nasm
CFLAGS = -std=c11 -m32 -ffreestanding -nostdlib -Wall -Wextra -I src/include -fno-pie
LDFLAGS = -m32 -T scripts/linker.ld -nostdlib -no-pie
ISO_DIR = iso
ISO_DIR = iso
ISO_FILE = dexis-x86.iso
BUILD_DIR = build

.PHONY: all clean run iso

all: $(BUILD_DIR)/dexiscore.bin

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/dexiscore.bin: $(BUILD_DIR)/boot.o $(BUILD_DIR)/main.o $(BUILD_DIR)/vga.o $(BUILD_DIR)/dsh.o | $(BUILD_DIR)
	$(CC) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/boot.o: src/boot/boot.asm | $(BUILD_DIR)
	$(ASM) -f elf32 $< -o $@

$(BUILD_DIR)/main.o: src/kernel/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/vga.o: src/kernel/vga.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/dsh.o: src/kernel/dsh.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

iso: $(BUILD_DIR)/dexiscore.bin
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(BUILD_DIR)/dexiscore.bin $(ISO_DIR)/boot/
	echo 'set timeout=0' > $(ISO_DIR)/boot/grub/grub.cfg
	echo 'menuentry "DexisCore" { multiboot2 /boot/dexiscore.bin; boot }' >> $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_FILE) $(ISO_DIR)

run: iso
	qemu-system-i386 -cdrom $(ISO_FILE) -serial stdio

clean:
	rm -rf $(BUILD_DIR) *.o $(ISO_DIR) $(ISO_FILE)