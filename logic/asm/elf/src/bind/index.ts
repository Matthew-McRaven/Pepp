/* eslint-disable camelcase,no-bitwise */
import bindings from '@pepnext/bindings';
import path from 'path';
import { fileURLToPath } from 'url';

// Local imports
import {
  Elf, saveElfToFile, saveElfToBuffer, loadElfFromFile, loadElfFromBuffer, e_type,
} from './top_level';
import {
  sh_type, sh_flags, SectionHeader, Section, markStrtab, markSymtab,
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
export {
  Elf, saveElfToFile, saveElfToBuffer, loadElfFromFile, loadElfFromBuffer, e_type,
};
export {
  sh_type, sh_flags, SectionHeader, Section, markStrtab, markSymtab,
};
export type { Note, NoteAccessor };
export type {
  Rel, RelA, RelAccessor, RelAAccessor,
};
export type { StringAccessor };
export {
  st_type, st_bind, st_visibility, Symbol, SymbolAccessor, writeSymbols,
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
  StringAccessor: addon.StringAccessor as new(elf:Elf, section:Section)=>StringAccessor,
  SymbolAccessor: addon.SymbolAccessor as new(elf:Elf, strSec:Section, symSec: Section)=>SymbolAccessor,
  Elf: addon.Elf as new()=>Elf,
  saveElfToFile: addon.saveElfToFile as saveElfToFile,
  loadElfFromFile: addon.loadElfFomFile as loadElfFromFile,
  saveElfToBuffer: addon.saveElfToBuffer as saveElfToBuffer,
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

export class CachedStringAccessor implements StringAccessor {
  constructor(elf:Elf, strSec:Section) {
    this.#stringCache = new StringCache();
    this.#stringSection = strSec;
    this.#nativeAccessor = new native.StringAccessor(elf, strSec);
  }

  #stringSection: Section

  #nativeAccessor : StringAccessor

  #stringCache: StringCache

  addString(name: string): bigint {
    const has = this.#stringCache.has(this.#stringSection.getName(), name);
    if (has !== undefined) return has;
    const added = this.#nativeAccessor.addString(name);
    this.#stringCache.insert(this.#stringSection.getName(), name, added);
    return added;
  }

  getString(index: bigint): string | undefined {
    return this.#nativeAccessor.getString(index);
  }
}

export class CachedSymbolAccessor implements SymbolAccessor {
  constructor(elf:Elf, strSec:Section, symSec: Section) {
    this.#stringCache = new CachedStringAccessor(elf, strSec);
    this.#nativeAccessor = new native.SymbolAccessor(elf, strSec, symSec);
  }

  #nativeAccessor : SymbolAccessor

  #stringCache: CachedStringAccessor

  addSymbol(symbol: Symbol): bigint {
    const cachedName = typeof symbol.name === 'string' ? this.#stringCache.addString(symbol.name) : symbol.name;
    return this.#nativeAccessor.addSymbol({ ...symbol, name: cachedName });
  }

  getIndex(): bigint {
    return this.#nativeAccessor.getIndex();
  }

  getSymbol(index: bigint | string): Symbol | undefined {
    return this.#nativeAccessor.getSymbol(index);
  }

  getSymbolCount(): bigint {
    return this.#nativeAccessor.getSymbolCount();
  }

  updateInfo() {
    return this.#nativeAccessor.updateInfo();
  }
}

export const updateSegementMemorySize = (elf:Elf, segment:Segment) => {
  let bytes = 0n;
  for (let index = 0n; index < segment.getSectionCount(); index += 1n) {
    const secIndex = segment.getSectionIndexAt(index);
    const section = elf.getSection(secIndex);

    bytes += section !== undefined ? section.getSize() : 0n;
  }
  segment.setMemorySize(bytes);
  return bytes;
};
