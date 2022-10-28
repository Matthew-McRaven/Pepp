import { Elf } from './top_level';
import { Section } from './section';
import StringCache from './string_cache';

export interface StringAccessor {
    new(elf:Elf, sec:Section, cache:StringCache): StringAccessor
    getString(index: bigint): string|undefined
    // If name is not present in the string table section, add the string and return its string index.
    // If it does exist, just return the string index.
    addString(name: string): bigint
}
