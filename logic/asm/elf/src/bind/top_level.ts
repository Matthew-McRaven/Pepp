import { Section } from './section';
import { Segment } from './segment';

export const e_type = {
  ET_NONE: 0n,
  ET_REL: 1n,
  ET_EXEC: 2n,
  ET_DYN: 3n,
  ET_CORE: 4n,
};

export interface Elf {
    init(bitWidth:32 | 64): boolean
    validate(): string|true

    getClass(): 32 | 64

    getVersion(): bigint

    getOSABI():bigint
    setOSABI(abi:bigint): void;
    getABIVersion():bigint
    setABIVersion(abi:bigint): void;

    getType(): bigint
    setType(type:bigint): void;

    getMachine(): bigint
    setMachine(machine:bigint): void

    getFlags(): bigint
    setFlags(flags:bigint): void

    getEntry(): bigint
    setEntry(address:bigint): void;

    getDefaultEntrySize(sectionType: bigint): bigint;

    /*
     * Helpers not present in elfio, but make library usage easier in TS.
     */
    addSection(name:string): Section
    getSection(index:bigint): Section | undefined

    addSegment(): Segment
    getSegment(index:bigint): Segment | undefined
}

export type saveElfToFile = (elf:Elf, path:string)=>boolean;
export type loadElfFromFile= (path:string)=>Elf|undefined;
export type saveElfToBuffer = (elf:Elf)=>Uint8Array;
export type loadElfFromBuffer= (buffer:Uint8Array)=>Elf|undefined;
