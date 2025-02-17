# DexisCore
Dexis is my own OS with custom shell `dsh` (DexisShell)

# Building
clone to the repository:
```
git clone https://github.com/TheShlyukov/DexisCore/
cd DexisCore
```
now you have 3 ways to build project:

`make` -- make only `dexiscore.bin` and quit.

`make iso` -- make `dexiscore.bin`, `dexis-x86.iso` and quit.

`make run` -- make `dexiscore.bin`, `dexis-x86.iso` and run `.iso` with QEMU

***NOTE: Install `build-essential`, `gcc-multilib`, `nasm`, `grub-pc-bin`, `xorriso` and `qemu-system-i386` before building project:***

```
sudo apt install build-essential gcc-multilib nasm grub-pc-bin xorriso qemu-system-i386
```

# Booting .iso from releases (or after building to use without Makefile)
Download iso from relaeses https://github.com/TheShlyukov/DexisCore/releases

...or copy built `.iso` to a dirrectory:
```
cp dexis-x86.iso /path/to/your/dirrectory/
```

Move to the dirrectory, where you saved `.iso` (example):
```
cd ~/Downloads
```

Now boot `.iso` using QEMU:
```
qemu-system-i386 -cdrom dexis-x86.iso
```

You can also boot with `-serial stdio` to debug with terminal output:
```
qemu-system-i386 -cdrom dexis-x86.iso -serial stdio
```
