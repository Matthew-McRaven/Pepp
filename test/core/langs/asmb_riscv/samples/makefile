# Toolchain
CROSS   = riscv64-unknown-elf-
AS      = $(CROSS)gcc
LD      = $(CROSS)ld
OBJDUMP = $(CROSS)objdump

# Flags
ARCH     = rv32imac
ABI      = ilp32
ASFLAGS  = -march=$(ARCH) -mabi=$(ABI) -nostdlib
LDFLAGS  = -melf32lriscv

# Sources -> ELFs (one ELF per .S file)
SRCS = $(wildcard *.S)
ELFS = $(SRCS:.S=.elf)

all: $(ELFS)

# Assemble + link in one step via gcc driver
%.elf: %.S
	$(AS) $(ASFLAGS) -o $@ $<

# Optional: disassemble to verify
%.dump: %.elf
	$(OBJDUMP) -d $< > $@.txt

clean:
	rm -f *.elf *.o *.dump *.dump.txt

.PHONY: all clean
