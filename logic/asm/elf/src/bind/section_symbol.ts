/* eslint-disable camelcase,no-bitwise */
// import { Elf } from './top_level';
// import { Section } from './section';

export const st_bind = {
  STB_LOCAL: 0n,
  STB_GLOBAL: 1n,
  STB_WEAK: 2n,
  STB_LOPROC: 13n,
  STB_HIPROC: 15n,
} as const;

export const st_type = {
  STT_NOTYPE: 0n,
  STT_OBJECT: 1n,
  STT_FUNC: 2n,
  STT_SECTION: 3n,
  STT_FILE: 4n,
  STT_COMMON: 5n,
  STT_TLS: 6n,
  STT_LOOS: 10n,
  STT_HIOS: 12n,
  STT_LOPROC: 13n,
  STT_HIPROC: 15n,
} as const;

export const st_visibility = {
  STV_DEFAULT: 0n,
  STV_INTERNAL: 1n,
  STV_HIDDEN: 2n,
  STV_PROTECTED: 3n,
} as const;

export interface Symbol {
    name: string | bigint | undefined
    value: bigint
    size: bigint
    /** @see {@link st_info,st_bind} */
    binding: bigint
    /** @see {@link st_info, st_type} */
    type: bigint
    /** @see {@link st_visibility} */
    other: bigint
    shndx: bigint
}

export interface SymbolAccessor {
    /* new(elf:Elf, strSection:Section, symSection:Section): SymbolAccessor */
    getIndex(): bigint

    getSymbolCount(): bigint
    getSymbol(index:bigint|string): Symbol | undefined
    addSymbol(symbol:Symbol): bigint
    // update sh_info on the symtab.
    updateInfo(): void
}
