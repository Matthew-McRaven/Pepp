/* eslint-disable camelcase */
import { Elf } from './top_level';
import { Section } from './section';

export interface Rel {
    offset: bigint
    sym: bigint
    type: bigint
}

export type RelA = Rel & { addend: bigint }

export interface RelAccessor {
    new(elf:Elf, sec:Section): RelAccessor;
    getIndex(): bigint;
    getEntryCount(): bigint;
    getRelEntry(index:bigint): Rel | undefined;
    addRelEntry(entry: Rel): void;
}

export interface RelAAccessor {
    new(elf:Elf, sec:Section): RelAAccessor;
    getIndex(): bigint;
    getEntryCount(): bigint;
    getRelAEntry(index:bigint): Rel | undefined;
    addRelAEntry(entry: RelA): void;
}
