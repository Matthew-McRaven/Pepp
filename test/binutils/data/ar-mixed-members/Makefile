# Regenerates golden test outputs
# Uses RISC-V binutils by default (e.g. Homebrew's riscv64-elf-binutils); override PREFIX for another one.
PREFIX ?= riscv64-elf-
AS := $(PREFIX)as
AR := $(PREFIX)ar
ASFLAGS := -march=rv32i -mabi=ilp32
# Member order matters: the tests index members by position.
MEMBERS := a.o odd.txt a_member_with_a_long_name.o
INTERMEDIATES := $(MEMBERS)

all: regular.a thin.a
	rm -f $(INTERMEDIATES)

a.o: a.s
	$(AS) $(ASFLAGS) $< -o $@

# A name of 16 or more characters goes into the // table.
a_member_with_a_long_name.o: b.s
	$(AS) $(ASFLAGS) $< -o $@

# An odd size, so the next header needs a pad byte. Not an ELF file.
odd.txt:
	printf 'abc' > $@

# D: deterministic headers (mtime, uid, gid 0; mode 644). s: write the / symbol index. T: thin.
regular.a: $(MEMBERS)
	rm -f $@ && $(AR) rcsD $@ $^

thin.a: $(MEMBERS)
	rm -f $@ && $(AR) rcsDT $@ $^

clean:
	rm -f $(INTERMEDIATES)

.PHONY: all clean
