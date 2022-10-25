/* eslint-disable camelcase,no-bitwise */
import bindings from '@pepnext/bindings';
import path from 'path';
import { fileURLToPath } from 'url';

/* eslint-disable camelcase,no-bitwise */
export interface SectionHeaderFlags32 {
  size: 32
  sh_type: bigint
  sh_flags: bigint
  sh_addr: bigint
  sh_link: bigint
  sh_info: bigint
  sh_addr_align: bigint
}

export interface ELFSymbol32 {
  size: 32
  st_name: string
  st_value: bigint
  st_size: bigint
  st_bind: bigint
  st_info: bigint
  st_other: bigint
  st_shndx: bigint
}

export interface ELFRel32 {
  size: 32
  r_offset: bigint
  r_info: bigint
}

export type ELFRelA32 = ELFRel32 | { r_addend: bigint }

export interface IWriter {
  writeEntryPoint(arg:bigint): void;
  writeEType(arg:bigint): void;
  writeEMachine(arg:bigint): void;
  writeOSABI(arg:bigint): void;
  // If name is not present in the string table section, add the string and return it's string index.
  // If it does exist, just return the string index.
  writeString(section:string, name:string): bigint

  // Return a UByte4 containing the section
  writeSectionBytes(name:string, flags:SectionHeaderFlags32, bytes: Uint8Array): bigint;
  writeSymbols(strtabSectionName:string, symtabSectionName: string, symbols:ELFSymbol32[]): void;
  writeRelocations(relocations:(ELFRel32|ELFRelA32)[]): void;

  dumpToFile(path:string): boolean;
}

export const st_info = (bind:string, type: string) => {
  let ret = 0n;
  switch (type.toLowerCase()) {
    case 'notype': break;
    case 'object': ret |= 0x1n; break;
    case 'func': ret |= 0x2n; break;
    case 'section': ret |= 0x3n; break;
    case 'file': ret |= 0x4n; break;
    case 'common': ret |= 0x5n; break;
    default: throw new Error('Unexpected binding.');
  }
  // Mask out any misplace bits, as type is really an nyble.
  ret &= 0xfn;
  switch (bind.toLowerCase()) {
    case 'local': break;
    case 'global': ret |= (0x1n << 4n); break;
    case 'weak': ret |= (0x2n << 4n); break;
    default: throw new Error('Unexpected binding.');
  }
  return ret;
};

const filename = fileURLToPath(import.meta.url);
const dirname = path.dirname(filename);

const addon = bindings({
  bindings: 'bind-elf.node',
  userDefinedTries: [[`${dirname}`, 'bindings'], [`${dirname}/../../dist`, 'bindings']],
});

export const Writer = addon.ELFWriter as new(bitness:32|64)=>IWriter;
