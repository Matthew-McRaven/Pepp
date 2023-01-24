import * as T from '@pepnext/logic-pdt';
import toBuffer from 'typedarray-to-buffer';
import lodash from 'lodash';
import { JDense } from '@pepnext/device-jdense';
import {
  Clock, Trace as TraceMod, TraceTypes, Device, TargetOperation, System, Target, Initiator,
} from '@pepnext/device-interface';
import { MTM, MTMRegister } from './mtm_socket';

type Trace<T> = TraceTypes.Trace<T>
type DevicePOD = Device.DevicePOD
const { BaseAccessError } = Target;
const { isMatch } = lodash;
const { TickError } = Clock;

const incCycleType: TraceTypes.TraceType = {
  isDelta: true,
  deviceKind: 'mtmv1',
  dataKind: 'inc-cycle-count',
};

// eslint-disable-next-line import/prefer-default-export
export class MTM1 implements MTM, Clock.Clocked, TraceMod.Traceable, Device.Device, Initiator {
  constructor(device: DevicePOD, clockName: string, childID: number) {
    this.#device = { ...device, clockName };
    const child: DevicePOD = {
      baseName: `${device.baseName}.regbank`,
      fullName: `${device.fullName}.regbank`,
      compatible: 'memory',
      deviceID: childID,

    };
    this.register = new JDense(child, 0n, BigInt(2 * (MTMRegister.Count) - 1), 0);
    this.#clock = Clock.DisabledClock;
    this.#incCycleTrace = {
      device: this.deviceID(),
      payload: undefined,
      type: incCycleType,
    };
  }

  setTarget(target: Target.Target) {
    this.#target = target;
  }

  baseName() {
    return this.#device.baseName;
  }

  fullName() {
    return this.#device.fullName;
  }

  deviceID() {
    return this.#device.deviceID;
  }

  compatible() {
    return this.#device.compatible;
  }

  cycleCount(): number {
    return this.instructionCount();
  }

  instructionCount(): number {
    return this.#instructionCount;
  }

  getClock(): Clock.Clock {
    return this.#clock;
  }

  setClock(clock: Clock.Clock): void {
    this.#clock = clock;
  }

  tick(currentTick: number): Clock.TickResult {
    // eslint-disable-next-line no-void
    void currentTick;
    return this.#impl();
  }

  reset() {
    this.register.clear(0);
    this.#instructionCount = 0;
  }

  #impl(): Clock.TickResult {
    const fail:Clock.TickResult = {
      pause: true,
      sync: true,
      delay: { kind: 'tick', period: Infinity },
      error: TickError.Terminate,
    };
    if (!this.#target) return { ...fail, error: TickError.NoTarget };

    let sync = false;
    if (this.#tb) {
      const incInstrCountResult = this.#tb.push(this.#incCycleTrace);
      if (!incInstrCountResult.success) {
        return {
          pause: false,
          sync: true,
          delay: { kind: 'tick', period: Infinity },
          error: TickError.TraceOverflow,
        };
      }
      sync = incInstrCountResult.overflow;
    }
    this.#instructionCount += 1;

    const op: TargetOperation.Operation = { effectful: true, data: true, speculative: false };

    const pcResult = this.register.read(BigInt(MTMRegister.PC), 2, op);
    if (pcResult.completed === false) return fail;
    const pc = pcResult.data;
    const buffer = toBuffer(pc);
    const value = buffer.readInt16BE();
    buffer.writeInt16BE(value + 1);
    const writebackResult = this.register.write(BigInt(MTMRegister.PC), pc, op);
    if (writebackResult.completed === false) return fail;
    const isResult = this.#target.read(BigInt(buffer.readInt16BE()), 2, op);
    if (isResult.completed === false) return fail;
    const writebackResult2 = this.register.write(BigInt(MTMRegister.A), isResult.data, op);
    if (this.#tb) this.#tb.stage();
    if (writebackResult2.completed === false) return fail;
    const error1 = (writebackResult.error === BaseAccessError.Breakpoint) ? TickError.Breakpoint : TickError.Success;
    const error2 = (writebackResult2.error === BaseAccessError.Breakpoint) ? TickError.Breakpoint : TickError.Success;
    return {
      sync: sync || writebackResult.sync || writebackResult2.sync,
      pause: writebackResult.pause || writebackResult2.pause || false,
      delay: { kind: 'clock', period: 1 },
      error: error1 === TickError.Success ? error2 : error1,
    };
  }

  setTraceBuffer(tb: TraceMod.TraceBuffer): void {
    this.#tb = tb;
  }

  do(trace: Trace<any>): boolean {
    if (trace.device !== this.deviceID()) return false;
    if (isMatch(trace.type, incCycleType)) {
      this.#instructionCount += 1;
      return true;
    }
    return false;
  }

  undo(trace: Trace<any>): boolean {
    if (trace.device !== this.deviceID()) return false;
    if (isMatch(trace.type, incCycleType)) {
      this.#instructionCount -= 1;
      return true;
    }
    return false;
  }

  #device: DevicePOD & {clockName:string};

  register: JDense;

  #instructionCount = 0;

  #clock: Clock.Clock;

  #tb: TraceMod.TraceBuffer | undefined;

  #incCycleTrace: Trace<undefined>;

  #target: Target.Target | undefined;

  getClockName(): string {
    return this.#device.clockName;
  }
}
export const getRegistration = () => {
  const create = (node:T.Socket.MTM.Type, path:string, id:()=>number) => {
    const pod:DevicePOD = {
      baseName: node.name,
      fullName: `${path}/${node.name}`,
      compatible: node.compatible,
      deviceID: id(),
    };
    return new MTM1(pod, node.clock, id()) as System.SystemClocked;
  };
  const processor = {
    model: 'mcraven,mtm-v1',
  };
  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  const link = (node:T.Socket.MTM.Type, path:string, system:System.System) => {
    const device = system.getDevice(`${path}/${node.name}`);
    if (!device) throw new Error('MTM1 does not exist');
    const target = system.getDevice(node.target);
    if (!target) throw new Error("Target doesn't exist");
    else if (!('read' in target)) throw new Error('Target is not of type target');
    (<MTM1>device).setTarget(target);
    return true;
  };
  return { match: { ...T.Socket.MTM.Match, processor }, construct: create, link };
};
