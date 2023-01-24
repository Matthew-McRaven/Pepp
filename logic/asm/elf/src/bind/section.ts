/* eslint-disable camelcase */
export const sh_type = {
  SHT_NULL: 0n,
  SHT_PROGBITS: 1n,
  SHT_SYMTAB: 2n,
  SHT_STRTAB: 3n,
  SHT_RELA: 4n,
  SHT_HASH: 5n,
  SHT_DYNAMIC: 6n,
  SHT_NOTE: 7n,
  SHT_NOBITS: 8n,
  SHT_REL: 9n,
  SHT_SHLIB: 10n,
  SHT_DYNSYM: 11n,
  SHT_LOPROC: 0x70000000n,
  SHT_HIPROC: 0x7fffffffn,
  SHT_LOUSER: 0x80000000n,
  SHT_HIUSER: 0xffffffffn,
} as const;

export const sh_flags = {
  SHF_WRITE: 0x1n,
  SHF_ALLOC: 0x2n,
  SHF_EXECINSTR: 0x4n,
  SHF_MERGE: 0x10n,
  SHF_STRINGS: 0x20n,
  SHF_INFO_LINK: 0x40n,
  SHF_LINK_ORDER: 0x80n,
  SHF_OS_NONCONFORMING: 0x100n,
  SHF_GROUP: 0x200n,
  SHF_TLS: 0x400n,
  SHF_COMPRESSED: 0x800n,
  SHF_GNU_RETAIN: 0x200000n,
  SHF_GNU_MBIND: 0x01000000n,
  SHF_MASKOS: 0x0FF00000n,
  SHF_MIPS_GPREL: 0x10000000n,
  SHF_ORDERED: 0x40000000n,
  SHF_EXCLUDE: 0x80000000n,
  SHF_MASKPROC: 0xF0000000n,
} as const;

export interface SectionHeader {
    name: string
    /** @see {@link sh_type} */
    type: bigint
    /** @see {@link sh_flags} */
    flags: bigint
    addr: bigint
    /** Must be 0 or the index of another section */
    link: bigint
    /** Depends on sh_type, see TIS ELF spec Fig 1-12 */
    info: bigint
    /** Must be power of 2 */
    addr_align: bigint
}

export interface Section {
    getIndex(): bigint

    getName(): string
    setName(name:string): void

    /** @see {@link sh_type} */
    getType(): bigint
    /**
     * @arg type should be from sh_type enum
     * @see {@link sh_type}
     */
    setType(type:bigint): void

    setEntrySize(size:bigint):void

    /** @see {@link sh_flags} */
    getFlags(): bigint
    /**
     * @arg type should be from sh_type enum
     * @see {@link sh_flags}
     */
    setFlags(flags:bigint): void

    getInfo(): bigint
    setInfo(info:bigint): void

    getLink(): bigint
    setLink(link:bigint): void

    getAlign(): bigint
    setAlign(align:bigint): void

    getAddress(): bigint
    setAddress(address: bigint): void

    // Can't set, because it is computed from data
    getSize(): bigint
    // Force override size value.
    setSize(size:bigint):void;
    setData(bytes:Uint8Array): void
    appendData(bytes:Uint8Array): void
    getData(): Uint8Array
}

export const markSymtab = (section: Section, link:bigint, entrySize:bigint) => {
  section.setType(sh_type.SHT_SYMTAB);
  section.setAlign(0n);
  section.setLink(link);
  section.setAlign(0x2n);
  section.setEntrySize(entrySize);
};

export const markStrtab = (section: Section) => {
  section.setType(sh_type.SHT_STRTAB);
  section.setAlign(0n);
};
