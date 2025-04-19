# Makefile for building a simple kernel

# Assembler and linker
NASM = nasm
GCC = gcc
LD = ld

# Flags
CFLAGS = -m32 -ffreestanding -c
LDFLAGS = -T linker.ld

# Source files
BOOTLOADER_SRC = bootloader.asm
KERNEL_SRC = kernel.c

# Output files
BOOTLOADER_BIN = bootloader.bin
KERNEL_BIN = kernel.o
KERNEL_ELF = kernel.elf
KERNEL_BIN_FINAL = kernel.bin

all: $(BOOTLOADER_BIN) $(KERNEL_BIN_FINAL)

# Build bootloader
$(BOOTLOADER_BIN): $(BOOTLOADER_SRC)
	$(NASM) -f bin $(BOOTLOADER_SRC) -o $(BOOTLOADER_BIN)

# Build kernel
$(KERNEL_BIN): $(KERNEL_SRC)
	$(GCC) $(CFLAGS) $(KERNEL_SRC) -o $(KERNEL_BIN)

# Link kernel
$(KERNEL_ELF): $(KERNEL_BIN)
	$(LD) -m elf_i386 -T linker.ld -o $(KERNEL_ELF) $(KERNEL_BIN)

# Convert ELF to binary
$(KERNEL_BIN_FINAL): $(KERNEL_ELF)
	$(LD) -o $(KERNEL_BIN_FINAL) $(KERNEL_ELF) -Ttext 0x1000 --oformat binary

clean:
	rm -f $(BOOTLOADER_BIN) $(KERNEL_BIN) $(KERNEL_ELF) $(KERNEL_BIN_FINAL)
