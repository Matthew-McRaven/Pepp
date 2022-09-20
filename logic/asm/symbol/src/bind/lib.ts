import bindings from '@pep10/bindings';

import path from 'path';

import { fileURLToPath } from 'url';

interface ISymbolNative {
    name(): string;

    value(): number

    type(): string

    size(): string

    relocatable(): boolean

    symbolIndex(): number

    isConst(): boolean

    setConst(value: number): undefined

    isAddr(): boolean

    setAddr(base: number, offset: number, type: 'object' | 'code'): undefined

    isSymPtr(): boolean

    setSymPtr(value: ISymbolNative): undefined

    isEmpty(): boolean

    setEmpty(): undefined

    isDeleted(): boolean

    setDeleted(): undefined
}

// Must keep in sync with visit.hpp enum.
export enum TraversalPolicy {
    children = 0,
    siblings = 1,
    wholeTree = 2,
}

export interface ITableNative {
    parent(): IBranchTableNative | undefined

    rootTable(): IBranchTableNative | undefined

    exists(name: string, policy: TraversalPolicy): boolean | undefined

    enumerateSymbols(policy: TraversalPolicy): Array<ISymbolNative>

    listing(policy: TraversalPolicy): string

    // If two tables point to the same underlying data source, then the table indices will be equal.
    tableIndex(): number
}

export interface ILeafTableNative extends ITableNative {
    get(name: string): ISymbolNative | null

    reference(name: string): ISymbolNative | null

    define(name: string): ISymbolNative | null

    markGlobal(name: string): null
}

export interface IBranchTableNative extends ITableNative {
    addLeaf(): ILeafTableNative | null

    addBranch(): IBranchTableNative | null
}

export interface IBranchTableNativeCtor {
    new(): IBranchTableNative
}

const filename = fileURLToPath(import.meta.url);
const dirname = path.dirname(filename);

const addon16 = bindings({
  bindings: 'bind-symbol16.node',
  userDefinedTries: [[`${dirname}`, 'bindings'], [`${dirname}/../../dist`, 'bindings']],
});

export const LeafTable = {
  u16: addon16.LeafTable as new() => ILeafTableNative,
};

export const BranchTable = {
  u16: addon16.BranchTable as IBranchTableNativeCtor,
};
