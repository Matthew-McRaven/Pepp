/* eslint-disable camelcase,no-bitwise */
import bindings from '@pepnext/bindings';
import path from 'path';
import { fileURLToPath } from 'url';

// Local imports
import type {
  Elf, saveElfToFile, saveElfToBuffer, loadElfFromFile, loadElfFromBuffer,
} from './top_level';
import {
  sh_type, sh_flags, SectionHeader, Section,
} from './section';
import type { Note, NoteAccessor } from './section_note';
import type {
  Rel, RelA, RelAccessor, RelAAccessor,
} from './section_relocation';
import { StringAccessor } from './section_string';
import {
  st_type, st_bind, st_visibility, Symbol, SymbolAccessor,
} from './section_symbol';
import {
  p_type, p_flags, Segment,
} from './segment';
import StringCache from './string_cache';

// Local exports
export type {
  Elf, saveElfToFile, saveElfToBuffer, loadElfFromFile, loadElfFromBuffer,
};
export {
  sh_type, sh_flags, SectionHeader, Section,
};
export type { Note, NoteAccessor };
export type {
  Rel, RelA, RelAccessor, RelAAccessor,
};
export type { StringAccessor };
export {
  st_type, st_bind, st_visibility, Symbol, SymbolAccessor,
};
export { p_type, p_flags, Segment };
export { StringCache };

const filename = fileURLToPath(import.meta.url);
const dirname = path.dirname(filename);

const addon = bindings({
  bindings: 'bind-elf.node',
  userDefinedTries: [[`${dirname}`, 'bindings'], [`${dirname}/../../dist`, 'bindings']],
});

/* Export native types */
export const native = {
  NoteAccessor: addon.NoteAccessor as new(elf:Elf, section:Section, cache:StringCache)=>NoteAccessor,
  RelAccessor: addon.RelAccessor as new(elf:Elf, section:Section)=>RelAccessor,
  RelAAccessor: addon.RelAAccessor as new(elf:Elf, section:Section)=>RelAAccessor,
  StringAccessor: addon.StringAccessor as new(elf:Elf, section:Section, cache:StringCache)=>StringAccessor,
  SymbolAccessor: addon.SymbolAccessor as new(elf:Elf, strSec:Section, cache:StringCache, symSec: Section)=>SymbolAccessor,
  Elf: addon.Elf as new(bitness:32|64, cache:StringCache)=>Elf,
};

/* Export addon-dependent helpers */
// While a section helper, it depends on C++ code, and therefore must be defined after the addon
export const addRelocations = (elf:Elf, relocations:(Rel|RelA)[]) => {
  const rel: Rel[] = relocations.filter((r) => !('addend' in r));
  const rela: RelA[] = relocations.filter((r) => ('addend' in r)) as RelA[];
  if (rel.length > 0) {
    const relSec = elf.addSection('.rel');
    // Writer handles setting all the section flags correctly.
    const relWriter = new addon.RelAccessor(elf, relSec) as RelAccessor;
    rel.forEach((r) => relWriter.addRelEntry(r));
  }
  if (rela.length > 0) {
    const relaSec = elf.addSection('.rela');
    // Writer handles setting all the section flags correctly.
    const relaWriter = new addon.RelAAccessor(elf, relaSec) as RelAAccessor;
    rela.forEach((r) => relaWriter.addRelAEntry(r));
  }
};
