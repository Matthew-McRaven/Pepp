import { Device, Target, Trace } from '@pepnext/device-interface';

export const enum MTMRegister {
    PC = 0,
    SP = 2,
    A = 4,
    Count = 3
}

export interface MTM {

    instructionCount: () => number
    cycleCount: () => number

    register: Trace.Traceable & Device.Device & Target.Target

}
