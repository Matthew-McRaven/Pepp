# Rebuilds line_info.elf: line_info.c as a freestanding riscv64 executable with DWARF 5 line tables, stripped to only
# its line number sections. Linking resolves DW_LNE_set_address, so the ELF needs no relocations.
# Make predefines CC, so ?= would never apply. All can still be overridden: make CC=... OBJCOPY=... OBJDUMP=...
ifeq ($(origin CC),default)
CC := riscv64-elf-gcc
endif
OBJCOPY ?= riscv64-elf-objcopy
OBJDUMP ?= riscv64-elf-objdump

line_info.elf: line_info.c
	$(CC) -O0 -g -gdwarf-5 -ffreestanding -nostdlib -static -Wl,-e,main -Wl,--build-id=none \
	  -fdebug-prefix-map=$(CURDIR)=. -o $@.full $<
	$(OBJCOPY) --only-section=.debug_line --only-section=.debug_line_str --strip-all \
	  --keep-section=.debug_line --keep-section=.debug_line_str $@.full $@
	rm -f $@.full

# Prints the line number program to stdout: raw header and opcodes (-Wl), then decoded rows (-WL).
.PHONY: dump
dump: line_info.elf
	$(OBJDUMP) -Wl -WL $<
