import { Operation } from './target_operation';

export enum InterposeResult {
    Success=0,
    Breakpoint
}
export interface NonNativeInterposer {
    tryRead:(address:bigint, count:number, op:Operation|{get:boolean})=>InterposeResult
    tryWrite:(address:bigint, data:Uint8Array, op:Operation|{set:boolean})=>InterposeResult
}

export const NativeInterposer = {};
