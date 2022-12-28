import {
  Clock, Clocked, Initiator, SplitInitiator, TickResult,
} from './clock/clock';
import { Device } from './device';
import { NonNativeTarget } from './target';
import { Traceable, TraceBuffer } from './trace';

export const enum TickMode {
    Increment, // Execute only the next tick, even if no clocked device is ticked.
    Jump, // Execute up to and including the next tick with a clocked device.
}
export interface Scheduler {
    next: (currentTick: number, mode:TickMode) => {devices:SystemClocked[], tick:number}
    schedule: (device:SystemClocked, firstTick: number) => void;
    reschedule: (deviceID: number, nextTick: number) => void;
}
export type SystemTarget = (NonNativeTarget & Device) | (NonNativeTarget & Device & Traceable)
export type SystemClock = (Clock & Device) | (Clock & Device & Traceable)
export type SystemInitiator = ((Initiator | SplitInitiator) & Device) | ((Initiator | SplitInitiator) & Device & Traceable)
export type SystemClocked = (Clocked & Device & Traceable) | (Clocked & Device & Traceable & SystemInitiator)
export interface System {
    getDevice: (target: string | number) => SystemTarget | SystemClocked | SystemClock | undefined
    addTarget:(target: SystemTarget)=> void
    addClocked:(clocked:SystemClocked)=> void
    clockeds: ()=>readonly SystemClocked[]
    addClock:(clock:SystemClock)=> void
    clocks: ()=>readonly SystemClock[]

    tick: (mode:TickMode)=>TickResult & {tick:number}
    currentTick: ()=>number
    overrideTickRate:(tickRate:number)=>boolean
    getTickRate: ()=>number
    // Return a function that generates monotonically increasing IDs
    nextID:()=>()=>number

    setTraceBuffer: (tb:TraceBuffer)=>void
}
