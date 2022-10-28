import { Section } from './section';
import { Elf } from './top_level';

export interface Note {
    type: bigint
    name: string
    /** If you need these bytes outside of the lifetime of the Elf file, clone them. */
    desc: Uint8Array
}

export interface NoteAccessor {
    new(elf: Elf, sec:Section): NoteAccessor
    getIndex(): bigint
    getNoteCount(): bigint;
    getNote(index:bigint): Note | undefined
    addNote(note:Note): void

}
