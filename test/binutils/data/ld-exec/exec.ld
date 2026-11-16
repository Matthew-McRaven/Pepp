ENTRY(_start)
SECTIONS {
  .text 0x10000 : { *(.text) }
  . = 0x20000;
  .data : { *(.data) }
  .bss : { *(.bss) }
}
