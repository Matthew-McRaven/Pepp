# a.o and b.o link into an executable whose only relocation is a data word. exec.ld places their sections and leaves
# .mine an orphan.
# Override PREFIX to select a different binutils
PREFIX ?= riscv64-elf-
AS := $(PREFIX)as
ASFLAGS := -march=rv32i -mabi=ilp32

all: a.o b.o

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

clean:

.PHONY: all clean
