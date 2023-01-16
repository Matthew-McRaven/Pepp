import lodash from 'lodash';
import {
  Device, Interposer, Target, TargetOperation, Trace, TraceTypes,
} from '@pepnext/device-interface';
import { InterposeResult } from '@pepnext/device-interface/dist/interposer';
import { Channel, Endpoint } from './channel';

export const MMITraceType: TraceTypes.TraceType = {
  isDelta: true,
  deviceKind: 'mmi',
  dataKind: 'none',
};

export interface MMITracePayload {
  index: number
  time: number
}

export type MMITrace = TraceTypes.Trace<MMITracePayload> & {type:typeof MMITraceType}

const { isMatch } = lodash;
export class JMMI implements Target.NonNativeTarget, Device.NonNativeDevice, Trace.Traceable {
  constructor(device:Device.DevicePOD, minOffset:bigint, maxOffset:bigint, defaultValue:number) {
    this.#device = device;
    this.#defaultValue = defaultValue;
    // Assigns min and max offsets for us.
    this.resize(maxOffset, minOffset);
  }

  baseName():string {
    return this.#device.baseName;
  }

  fullName(): string {
    return this.#device.fullName;
  }

  deviceID(): number {
    return this.#device.deviceID;
  }

  compatible(): string {
    return this.#device.compatible;
  }

  // eslint-disable-next-line class-methods-use-this
  minOffset(): bigint {
    return this.#minOffset;
  }

  maxOffset(): bigint {
    return this.#maxOffset;
  }

  push(address:bigint, value:number|number[]): boolean {
    if (address < this.minOffset() || address > this.maxOffset()) return false;
    const effective = Number(address - this.minOffset());
    const channel = this.#channels[effective];
    if (typeof value === 'number') channel.write.append(value);
    else value.forEach((v) => channel.write.append(v));
    return true;
  }

  read(address: bigint, count: number, op: TargetOperation.Operation): Target.ReadResult {
    if (address < this.minOffset() || address > this.maxOffset()) {
      return {
        completed: false, advance: false, pause: true, sync: false, error: Target.BaseAccessError.OOBAccess,
      };
    }

    const traces: MMITrace[] = [];
    const data = new Uint8Array(count);
    const modulus = BigInt(this.#channels.length);

    // Check if the memory access will trigger any kind of interposer action before doing memory access.
    // The interposer will become disabled until explicitly re-enabled, to prevent infinite breakpoint loops.
    if (op.effectful && this.#interposer) {
      switch (this.#interposer.tryRead(address, count, op)) {
        case InterposeResult.Breakpoint:
          return {
            completed: false, advance: false, pause: true, sync: false, error: Target.BaseAccessError.Breakpoint,
          };
        default:
          break;
      }
    }

    // If the op can't trigger IO (such as reading the memory pane), then use the last-known value of the device.
    // Do not trigger any trace generation.
    if (op.speculative) {
      for (let it = 0; it < count; it += 1) {
        const index = Number((address + BigInt(it) - this.minOffset()) % modulus);
        const channel = this.#channels[index];
        data[it] = channel.last;
      }
    } else {
      // Read out all data, and generate traces
      for (let it = 0; it < count; it += 1) {
        const index = Number((address + BigInt(it) - this.minOffset()) % modulus);
        const channel = this.#channels[index];
        const time = channel.read.currentTime();
        traces.push({ device: this.deviceID(), payload: { index, time }, type: MMITraceType });
        const next = channel.read.next();
        // If at the end of MMI, undo all reads so far, and then return that we need more MMI.
        if (next === null) {
          traces.forEach((trace) => this.undo(trace));
          return {
            completed: false, advance: false, pause: false, sync: false, error: Target.BaseAccessError.NeedsMMI,
          };
        }
        channel.last = next.value;
        data[it] = next.value;
      }
    }

    let sync = false;
    // Push each trace onto trace buffer. If it overflows but succeeds, then request sync.\
    // If it fails, back out
    if (this.#traceBuffer) {
      // TODO: Replace with atomic multi-enqueue / "space check"
      for (let it = 0; it < traces.length; it += 1) {
        const push = this.#traceBuffer.push(traces[it]);
        if (push.overflow) sync = true;
        if (!push.success) {
          traces.forEach((trace) => this.undo(trace));
          this.#traceBuffer.discard();
          return {
            completed: false, advance: false, pause: false, sync, error: Target.BaseAccessError.FullTraceBuffer,
          };
        }
      }
      this.#traceBuffer.stage();
    }
    return {
      completed: true, advance: true, pause: false, sync, data, error: Target.BaseAccessError.Success,
    };
  }

  // Writing to an MMI device is a no-op
  write(address: bigint, data:Uint8Array, op:TargetOperation.Operation): Target.WriteResult {
    if (address < this.minOffset() || address > this.maxOffset()) {
      return {
        completed: false, advance: false, pause: true, sync: false, error: Target.BaseAccessError.OOBAccess,
      };
    }
    if (op.effectful && this.#interposer) {
      switch (this.#interposer.tryWrite(address, data, op)) {
        case Interposer.InterposeResult.Breakpoint:
          return {
            completed: true, advance: true, pause: true, sync: false, error: Target.BaseAccessError.Breakpoint,
          };
        default:
          break;
      }
    }
    // eslint-disable-next-line no-void
    void data; void op;
    return {
      completed: true, advance: true, pause: false, sync: false, error: Target.BaseAccessError.Success,
    };
  }

  setTraceBuffer(tb:Trace.TraceBuffer) {
    this.#traceBuffer = tb;
  }

  setInterposer(interposer:Interposer.NonNativeInterposer) {
    this.#interposer = interposer;
  }

  resize(newMaxOffset:bigint, newMinOffset?:bigint) {
    this.#maxOffset = newMaxOffset;
    this.#minOffset = newMinOffset === undefined ? this.#minOffset : newMinOffset;
    const size = Number(this.#maxOffset - this.#minOffset) + 1;
    this.#channels = [];
    for (let it = 0; it < size; it += 1) {
      const c = new Channel(this.#defaultValue);
      this.#channels.push({
        c, read: c.endpoint(), write: c.endpoint(), last: this.#defaultValue,
      });
    }
  }

  clear(value:number) {
    this.#defaultValue = value;
    this.#channels.forEach((c) => c.c.clear(this.#defaultValue));
  }

  do(trace: TraceTypes.Trace<any>):boolean {
    if (trace.device !== this.deviceID()) return false;
    if (!isMatch(trace.type, MMITraceType)) return false;
    const { payload } = <MMITrace> trace;
    const channel = this.#channels[payload.index];
    channel.read.next();
    return true;
  }

  undo(trace: TraceTypes.Trace<any>):boolean {
    if (trace.device !== this.deviceID()) return false;
    if (!isMatch(trace.type, MMITraceType)) return false;
    const { payload } = <MMITrace> trace;
    const channel = this.#channels[payload.index];
    channel.read.unread();
    return true;
  }

  #device: Device.DevicePOD;

  #defaultValue: number;

  #minOffset: bigint;

  #maxOffset:bigint;

  #interposer:null|Interposer.NonNativeInterposer = null;

  #traceBuffer: null|Trace.TraceBuffer = null;

  #channels: Array<{c:Channel, read:Endpoint, write:Endpoint, last:number}>;
}
