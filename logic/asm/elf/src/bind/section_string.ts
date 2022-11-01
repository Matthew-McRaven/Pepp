export interface StringAccessor {
    /* new(elf:Elf, sec:Section): StringAccessor */
    getString(index: bigint): string|undefined
    addString(name: string): bigint
}
