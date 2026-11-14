# main.o references needed, which lib.a's only member defines.
# Uses RISC-V binutils by default (e.g. Homebrew's riscv64-elf-binutils); override PREFIX for another one.
PREFIX ?= riscv64-elf-
AS := $(PREFIX)as
AR := $(PREFIX)ar
ASFLAGS := -march=rv32i -mabi=ilp32

all: main.o lib.a
	rm -f needed.o

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

lib.a: needed.o
	rm -f $@ && $(AR) rcsD $@ $^

clean:
	rm -f needed.o

.PHONY: all clean
