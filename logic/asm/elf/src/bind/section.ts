/* eslint-disable camelcase */
export const sh_type = {
  SHT_NULL: 0n,
  SHT_PROGBITGS: 1n,
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
  SHF_WRITE: 0x1,
  SHF_ALLOC: 0x2,
  SHF_EXECINSTR: 0x4,
  SHF_MERGE: 0x10,
  SHF_STRINGS: 0x20,
  SHF_INFO_LINK: 0x40,
  SHF_LINK_ORDER: 0x80,
  SHF_OS_NONCONFORMING: 0x100,
  SHF_GROUP: 0x200,
  SHF_TLS: 0x400,
  SHF_COMPRESSED: 0x800,
  SHF_GNU_RETAIN: 0x200000,
  SHF_GNU_MBIND: 0x01000000,
  SHF_MASKOS: 0x0FF00000,
  SHF_MIPS_GPREL: 0x10000000,
  SHF_ORDERED: 0x40000000,
  SHF_EXCLUDE: 0x80000000,
  SHF_MASKPROC: 0xF0000000,
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
    setData(bytes:Uint8Array): void
    appendData(bytes:Uint8Array): void
}
