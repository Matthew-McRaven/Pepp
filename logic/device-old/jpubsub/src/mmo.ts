import lodash from 'lodash';
import {
  Device, Interposer, Target, TargetOperation, Trace, TraceTypes,
} from '@pepnext/device-interface';
import { InterposeResult } from '@pepnext/device-interface/dist/interposer';
import { Channel, Endpoint } from './channel';
import { EndpointReadIterator } from './endpointiterator';

export const MMOTraceType: TraceTypes.TraceType = {
  isDelta: true,
  deviceKind: 'mmo',
  dataKind: 'none',
};

export interface MMOTracePayload {
  index: number
  time: number
  value:number
}

export type MMOTrace = TraceTypes.Trace<MMOTracePayload> & {type:typeof MMOTraceType}

const { isMatch } = lodash;
export class JMMO implements Target.NonNativeTarget, Device.NonNativeDevice, Trace.Traceable {
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

  #endpoint(address:bigint): Endpoint | null {
    if (address < this.minOffset() || address > this.maxOffset()) return null;
    const index = Number(address - this.minOffset());
    const channel = this.#channels[index];
    return channel.c.endpoint();
  }

  bytes(address:bigint): Iterable<number> {
    const endpoint = this.#endpoint(address);
    if (address < this.minOffset() || address > this.maxOffset() || endpoint === null) return [][Symbol.iterator]();
    endpoint.setToHead();
    return {
      [Symbol.iterator]: () => (new EndpointReadIterator(endpoint)),
    };
  }

  // Reading an MMO device returns the last-written value.
  read(address: bigint, count: number, op: TargetOperation.Operation): Target.ReadResult {
    if (address < this.minOffset() || address > this.maxOffset()) {
      return {
        completed: false, advance: false, pause: true, sync: false, error: Target.BaseAccessError.OOBAccess,
      };
    }

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

    for (let it = 0; it < count; it += 1) {
      const index = Number((address + BigInt(it) - this.minOffset()) % modulus);
      const channel = this.#channels[index];
      data[it] = channel.last;
    }
    return {
      completed: true, advance: true, pause: false, sync: false, data, error: Target.BaseAccessError.Success,
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

    const traces: MMOTrace[] = [];
    const modulus = BigInt(this.#channels.length);

    for (let it = 0; it < data.length; it += 1) {
      const index = Number((address + BigInt(it) - this.minOffset()) % modulus);
      const channel = this.#channels[index];
      const time = channel.write.currentTime();
      const value = data[it];
      traces.push({ device: this.deviceID(), payload: { index, time, value }, type: MMOTraceType });
      channel.write.append(value);
      channel.last = value;
    }
    let sync = false;
    // Atomically push all traces onto trace buffer. If this fails, we must back out the traces, because these actions
    // have already occured.
    if (this.#traceBuffer && traces.length > 0) {
      const result = this.#traceBuffer.push(traces);
      if (!result.success) {
        traces.forEach((trace) => this.undo(trace));
        return {
          completed: false, advance: false, pause: false, sync, error: Target.BaseAccessError.FullTraceBuffer,
        };
      }
      sync = result.overflow;
      this.#traceBuffer.stage();
    }
    return {
      completed: true, advance: true, pause: false, sync, error: Target.BaseAccessError.Success,
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
        c, write: c.endpoint(), last: this.#defaultValue,
      });
    }
  }

  clear(value:number) {
    this.#defaultValue = value;
    this.#channels.forEach((c) => {
      c.c.clear(this.#defaultValue);
      // eslint-disable-next-line no-param-reassign
      c.last = this.#defaultValue;
    });
  }

  do(trace: TraceTypes.Trace<any>):boolean {
    if (trace.device !== this.deviceID()) return false;
    if (!isMatch(trace.type, MMOTraceType)) return false;
    const { payload } = <MMOTrace> trace;
    const channel = this.#channels[payload.index];
    channel.write.append(payload.value);
    channel.last = channel.write.currentEvent().value;
    return true;
  }

  undo(trace: TraceTypes.Trace<any>):boolean {
    if (trace.device !== this.deviceID()) return false;
    if (!isMatch(trace.type, MMOTraceType)) return false;
    const { payload } = <MMOTrace> trace;
    const channel = this.#channels[payload.index];
    channel.write.unwrite();
    channel.last = channel.write.currentEvent().value;
    return true;
  }

  #device: Device.DevicePOD;

  #defaultValue: number;

  #minOffset: bigint;

  #maxOffset:bigint;

  #interposer:null|Interposer.NonNativeInterposer = null;

  #traceBuffer: null|Trace.TraceBuffer = null;

  #channels: Array<{c:Channel, write:Endpoint, last:number}>;
}
