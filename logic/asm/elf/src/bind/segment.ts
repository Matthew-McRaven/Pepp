/* eslint-disable camelcase */
import { Section } from './section';

export const p_type = {
  PT_NULL: 0,
  PT_LOAD: 1n,
  PT_DYNAMIC: 2n,
  PT_INTERP: 3n,
  PT_NOTE: 4n,
  PT_SHLIB: 5n,
  PT_PHDR: 6n,
  PT_TLS: 7n,
  PT_LOOS: 0X60000000n,
  PT_HIOS: 0X6FFFFFFFn,
  PT_LOPROC: 0X70000000n,
  PT_HIPROC: 0X7FFFFFFFn,
} as const;

export const p_flags = {
  // Segment flags
  PF_X: 1n, // Execute
  PF_W: 2n, // Write
  PF_R: 4n, // Read
  PF_MASKOS: 0x0ff00000n, // Unspecified
  PF_MASKPROC: 0xf0000000n, // Unspecified
};
export interface Segment {
    getIndex(): bigint

    /** returns p_type */
    getType(): bigint
    /** @arg type should be a p_type */
    setType(type:bigint): void

    getAlign(): bigint
    setAlign(align:bigint):void

    getVAddress(): bigint
    setVAddress(address:bigint): void

    getPAddress(): bigint
    setPAddress(address:bigint): void

    getMemorySize(): bigint
    setMemorySize(size:bigint): void

    getFileSize(): bigint

    /** returns p_flags |'ed together */
    getFlags(): bigint
    /** @see {@link p_flags} */
    setFlags(flags:bigint): void

    getSectionCount(): bigint
    getSectionIndexAt(index: bigint): bigint
    addSection(name:Section): void
    addSectionIndex(shndx: bigint): boolean

    recomputeFileSize():void
}
