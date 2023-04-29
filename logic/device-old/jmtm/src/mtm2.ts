import * as T from '@pepnext/logic-pdt';
import toBuffer from 'typedarray-to-buffer';
import lodash from 'lodash';
import { JDense } from '@pepnext/device-jdense';
import {
  Clock,
  Device,
  Initiator,
  System,
  Target,
  TargetOperation,
  Trace as TraceMod,
  TraceTypes,
} from '@pepnext/device-interface';
import { JMMI, JMMO } from '@pepnext/device-jpubsub';
import { MTM, MTMRegister } from './mtm_socket';

type Trace<T> = TraceTypes.Trace<T>
type DevicePOD = Device.DevicePOD
const { BaseAccessError } = Target;
const { isMatch } = lodash;
const { TickError } = Clock;

const incCycleType: TraceTypes.TraceType = {
  isDelta: true,
  deviceKind: 'mtmv2',
  dataKind: 'inc-cycle-count',
};

// eslint-disable-next-line import/prefer-default-export
export class MTM2 implements MTM, Clock.Clocked, TraceMod.Traceable, Device.Device, Initiator {
  constructor(device: DevicePOD, clockName: string, childIDGen: ()=>number) {
    this.#device = { ...device, clockName };
    const regbank: DevicePOD = {
      baseName: `${device.baseName}.regbank`,
      fullName: `${device.fullName}.regbank`,
      compatible: 'memory',
      deviceID: childIDGen(),

    };
    const cin: DevicePOD = {
      baseName: `${device.baseName}.cin`,
      fullName: `${device.fullName}.cin`,
      compatible: 'memory',
      deviceID: childIDGen(),
    };
    const cout: DevicePOD = {
      baseName: `${device.baseName}.cout`,
      fullName: `${device.fullName}.cout`,
      compatible: 'memory',
      deviceID: childIDGen(),
    };
    this.register = new JDense(regbank, 0n, BigInt(2 * (MTMRegister.Count) - 1), 0);
    this.cin = new JMMI(cin, 0n, 0n, 0x0);
    this.cout = new JMMO(cout, 0n, 0n, 0x0);
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

  static mapError(err:Target.BaseAccessError): Clock.TickError {
    switch (err) {
      case BaseAccessError.Success:
        return TickError.Success;
      case BaseAccessError.Unmapped:
        return TickError.Terminate;
      case BaseAccessError.OOBAccess:
        return TickError.Terminate;
      case BaseAccessError.NeedsMMI:
        return TickError.NoMMInput;
      case BaseAccessError.Breakpoint:
        return TickError.Breakpoint;
      case BaseAccessError.FullTraceBuffer:
        return TickError.TraceOverflow;
      default: return TickError.Terminate;
    }
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
    const cin = this.cin.read(0n, 1, op);
    if (!cin.completed) throw new Error("Can't handle");
    const cout = this.cout.write(0n, cin.data.slice(0, 1), op);
    if (this.#tb) this.#tb.stage();
    const results = [writebackResult, cin, cout];
    return {
      sync: results.reduce((prev, t) => (prev || t.sync), sync),
      pause: results.reduce((prev, t) => (prev || t.pause), false),
      delay: { kind: 'clock', period: 1 },
      error: results.reduce((prev, t) => (prev !== TickError.Success ? prev : MTM2.mapError(t.error)), TickError.Success),
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

  cin: JMMI;

  cout: JMMO;

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
    return new MTM2(pod, node.clock, id) as System.SystemClocked;
  };
  const processor = {
    model: 'mcraven,mtm-v2',
  };
  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  const link = (node:T.Socket.MTM.Type, path:string, system:System.System) => {
    const device = system.getDevice(`${path}/${node.name}`);
    if (!device) throw new Error('MTM2 does not exist');
    const target = system.getDevice(node.target);
    if (!target) throw new Error("Target doesn't exist");
    else if (!('read' in target)) throw new Error('Target is not of type target');
    (<MTM2>device).setTarget(target);
    return true;
  };
  return { match: { ...T.Socket.MTM.Match, processor }, construct: create, link };
};
