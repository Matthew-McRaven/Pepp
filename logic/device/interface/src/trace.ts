import { Trace } from './trace/types';

export interface TraceBufferStatus {
    success: boolean
    overflow: boolean
}

export type TraceCommitHookFn = (traces: IterableIterator<Trace<any>|{tick:number}>) => void;
export interface TraceCommitHook {
    handle: TraceCommitHookFn;
}

export interface TraceBuffer {
    traceDevice: (device: number, enabled:boolean)=>void;
    tick: (currentTick: number) => void;

    push:(trace: Trace<any>)=>TraceBufferStatus
    pop:()=>void
    pending(): IterableIterator<Trace<any>>

    stage:()=>void
    discard:(until?:number)=>IterableIterator<Trace<any>|{tick:number}>
    staged(): IterableIterator<Trace<any>|{tick:number}>

    commit:(strategy:'preferred'|'all')=>void;
    registerCommitHook:(hook:TraceCommitHook)=>number;
    unregisterCommitHook:(hookID:number)=>TraceCommitHook|undefined;

}

export interface Traceable {
    setTraceBuffer:(tb:TraceBuffer)=>void
    do: (trace:Trace<any>)=>boolean
    undo: (trace:Trace<any>)=>boolean
}
