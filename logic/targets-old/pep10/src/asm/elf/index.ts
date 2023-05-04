/* eslint-disable camelcase */
import { DefintionState, ISymbolNative, TraversalPolicy } from '@pepnext/logic-symbol';
import {
  e_type, Elf, native, sh_type, st_bind, st_type, Symbol as ElfSymbol, writeSymbols,
} from '@pepnext/logic-elf';
import { Root, SectionGroup, TypedNode } from '../ast/nodes';
import { treeToHex } from '../visitors';

import { updateSymbolShndx } from './shndx_annotation';

export const parseType = (type:string) => {
  switch (type.toLowerCase()) {
    case 'notype': return st_type.STT_NOTYPE;
    case 'object': return st_type.STT_OBJECT;
    case 'func': return st_type.STT_FUNC;
    case 'section': return st_type.STT_SECTION;
    case 'file': return st_type.STT_FILE;
    case 'common': return st_type.STT_COMMON;
    default: throw new Error('Unrecognized type');
  }
};

export const parseBind = (bind:string) => {
  switch (bind.toLowerCase()) {
    case 'local': return st_bind.STB_LOCAL;
    case 'global': return st_bind.STB_GLOBAL;
    case 'weak': return st_bind.STB_WEAK;
    default: throw new Error('Unrecognized binding');
  }
};

// eslint-disable-next-line no-bitwise
export const st_info = (bind:bigint, type:bigint) => (bind & 0xfn) | ((type & 0x4n) << 4n);

export const createElf = (node:TypedNode):Elf => {
  const writer = new native.Elf();
  writer.init(32);
  const sectionGroups = node.C as SectionGroup[];

  // Write global info
  writer.setType(e_type.ET_REL);
  writer.setMachine(0x5041n /* PA */); // P 10 with the second digit in hex.
  writer.setOSABI(0n /* ELFOSABI_NONE */);

  // Write code / data sections.

  // Helper to update the st_shndx of each section
  const setShndx = (name:string, st_shndx:bigint) => {
    node.walk((child:TypedNode) => {
      if (child.T !== 'sectionGroup') return;
      if (child.A.name !== name) return;
      // eslint-disable-next-line no-param-reassign
      child.A.st_shndx = st_shndx;
    });
  };

  // TODO: Optionally randomize section orders
  sectionGroups.forEach((s) => {
    let section = writer.getSection(s.A.name);
    if (section === undefined) {
      section = writer.addSection(s.A.name);
      section.setType(sh_type.SHT_PROGBITS);
      section.setFlags(0n /* TODO */);
      section.setAddress(s.A.address || 0n);
      setShndx(s.A.name, section.getIndex());
    }
    section.appendData(treeToHex(s));
  });

  // Update shndx on symbol declarations
  updateSymbolShndx(node);

  // Write symbol tables
  const rootTab = (node as Root).A.symtab;
  const symbols = rootTab.enumerateSymbols(TraversalPolicy.wholeTree) as Array<ISymbolNative>;
  // The only symbols that matter for linking are globals and undefined
  const exportedSymbols = symbols.filter((s) => (s.definitionState() === DefintionState.undefined || s.binding() === 'global'));
  // Undefined symbols may show up in multiple tables, but we only want one entry in the ELF's symtab.
  const deduplicatedSymbols = new Map<string, ISymbolNative>();
  exportedSymbols.forEach((s) => {
    if (s.definitionState() === DefintionState.undefined && !deduplicatedSymbols.has(s.name())) deduplicatedSymbols.set(s.name(), s);
    else if (s.binding() === 'global') deduplicatedSymbols.set(s.name(), s);
  });
  // Create the ELF symbol format for the deduplicate symbols.
  const elfSymbols: ElfSymbol[] = [];
  deduplicatedSymbols.forEach((s) => {
    elfSymbols.push({
      name: s.name(),
      size: s.size() || 0n,
      binding: 1n,
      type: 1n,
      value: s.value() || 0n,
      shndx: s.sectionIndex(),
      other: 0n, // Only contains visibility, which we will leave at default for now
    });
  });
  if (elfSymbols.length > 0) {
    // Rely on writeSymbols to set strtab/symtab info/links/sizes.
    writer.addSection('.strtab');
    writer.addSection('.symtab');
    writeSymbols(writer, '.strtab', '.symtab', elfSymbols);
  }

  // Write relocation entries
  // Make sure to relocate any usages of pushed-down symbols.
  // TODO: Implement after writing visitor.

  // Write addr:line mapping
  // TODO: Implement after writing visitor.

  // Write MMIO fields as JSON, add to MMIO.
  // If none, do not write section at all.
  // TODO: Implement when OS is assembling successfully

  // Write debug info
  // TODO: Implement after adding debug info

  return writer;
};
