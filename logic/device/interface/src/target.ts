import { NonNativeInterposer } from './interposer';
import { Operation } from './target_operation';

export enum BaseAccessError {
    Success = 0,
    Unmapped, //  Attempted to read a physical address with no device present.
    OOBAccess, //  Attempted out-of-bound access on a storage device.
    NeedsMMI, //  Attempted to read MMI that had no buffered input.
    Breakpoint, // Memory access triggered a memory breakpoint
    FullTraceBuffer, // Attempted memory access would overflow the trace buffer. Flush the trace buffer and try again.
}
export interface BaseAccessResult {
    completed: boolean
    advance: boolean
    pause: boolean
    sync: boolean
    error: BaseAccessError
}

export interface ReadFailedResult extends BaseAccessResult {
    completed: false
}
export interface ReadSuccessResult extends BaseAccessResult {
    completed:true
    data: Uint8Array
}
export type ReadResult = ReadFailedResult | ReadSuccessResult
export type WriteResult = BaseAccessResult;

export interface Target {
    read:(address:bigint, count:number, op:Operation)=>ReadResult
    write:(address:bigint, data:Uint8Array, op:Operation)=>WriteResult
    minOffset:()=>bigint
    maxOffset:()=>bigint
    clear:(value:number)=>void
}
export interface NativeTarget extends Target{
    setInterposer:()=>null
}

export interface NonNativeTarget extends Target{

    setInterposer:(interposer:NonNativeInterposer)=>void
}
