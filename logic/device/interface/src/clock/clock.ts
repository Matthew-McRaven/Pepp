import { Target } from '../target';

export enum tickError {
    Success =0,
    NoMMInput,
    Terminate,
    Breakpoint,
    TraceOverflow,
    TraceOverflowAfterCommit,
    NoTarget,
}

export interface TickResult {
    pause: boolean
    sync: boolean
    delay: {kind:'tick' | 'clock', period: number}
    error: tickError
}

export interface FrequencyChangeHook {
    handleFrequencyChange: (frequency:number)=>void
}

export interface Clock {
    frequency: () => number
    // After initial system setup, maxFrequency is not allowed to change.
    maxFrequency: () => number
    registerFrequencyChangeHook: (hook:FrequencyChangeHook) => number;
}

export interface Clocked {
    getClockName: ()=>string
    setClock: (clock:Clock)=>void;
    getClock: ()=>Clock
    tick: (current_tick:number)=>TickResult
    reset: () => void
}

export interface Initiator {
    setTarget: (target:Target)=>void;
}

export interface SplitInitiator {
    setITarget: (target:Target)=>void;
    setDTarget: (target:Target)=>void;
}

export const DisabledClock = {
  frequency: () => 0,
  maxFrequency: () => 0,
  registerFrequencyChangeHook: () => 0,
};
