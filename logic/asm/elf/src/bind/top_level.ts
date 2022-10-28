import { Section } from './section';
import { Segment } from './segment';
import StringCache from './string_cache';

export interface Elf {
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

    /*
     * Helpers not present in elfio, but make library usage easier in TS.
     */
    addSection(name:string): Section
    getSection(index:bigint): Section | undefined

    addSegment(): Segment
    getSegment(index:bigint): Segment | undefined

    getStringCache(): StringCache
}

export type saveElfToFile = (elf:Elf, path:string)=>boolean;
export type loadElfFromFile= (path:string)=>Elf|undefined;
export type saveElfToBuffer = (elf:Elf)=>Uint8Array;
export type loadElfFromBuffer= (buffer:Uint8Array)=>Elf|undefined;
