import * as T from '@pepnext/logic-pdt';
import lodash from 'lodash';
import {
  Target, Trace, Interposer, TraceTypes, Device, TargetOperation,
} from '@pepnext/device-interface';

const { isMatch } = lodash;
// eslint-disable-next-line import/prefer-default-export
export class Dense implements Target.NonNativeTarget, Device.NonNativeDevice, Trace.Traceable {
  constructor(device: Device.DevicePOD, minOffset:bigint, maxOffset: bigint, defaultValue: number) {
    this.#device = device;
    this.#minOffset = minOffset;
    this.#maxOffset = maxOffset;
    this.#storage = new Uint8Array(Number(maxOffset - minOffset) + 1);
    this.#fillValue = defaultValue;
    this.#storage.fill(this.#fillValue);
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

  read(address: bigint, count: number, op: TargetOperation.Operation): Target.ReadResult {
    if (address < this.minOffset() || address > this.maxOffset()) {
      return {
        completed: false, advance: false, pause: true, sync: false, error: Target.BaseAccessError.OOBAccess,
      };
    }

    // eslint-disable-next-line no-void
    void op;
    const out = new Uint8Array(count);
    const modulus = BigInt(this.#storage.length);
    for (let it = 0; it < count; it += 1) {
      const effective = (address + BigInt(it) - this.minOffset()) % modulus;
      out[it] = this.#storage[Number(effective)];
    }

    let pause = false;
    let error = Target.BaseAccessError.Success;
    if (op.effectful && this.#interposer) {
      switch (this.#interposer.tryRead(address, count, op)) {
        case Interposer.InterposeResult.Breakpoint:
          pause = true;
          error = Target.BaseAccessError.Breakpoint;
          break;
        default:
          break;
      }
    }

    return {
      data: out, completed: true, advance: true, pause, sync: false, error,
    };
  }

  #pushWriteTrace(address: bigint, data:Uint8Array, op: TargetOperation.Operation): Trace.TraceBufferStatus {
    const old = this.read(address, data.length, { ...op, effectful: false });
    if (!old.completed) return { success: false, overflow: false };
    const trace: TraceTypes.Basic.Basic = {
      device: this.#device.deviceID,
      payload: {
        address,
        newValue: data,
        oldValue: old.data,
      } as TraceTypes.Basic.BasicData,
      type: TraceTypes.Basic.BasicType,
    };
    if (this.#traceBuffer) return this.#traceBuffer.push(trace);
    return { success: true, overflow: false };
  }

  setInterposer(interposer:Interposer.NonNativeInterposer|null) {
    this.#interposer = interposer;
  }

  setTraceBuffer(tb:Trace.TraceBuffer) {
    this.#traceBuffer = tb;
  }

  write(address: bigint, data: Uint8Array, op: TargetOperation.Operation): Target.WriteResult {
    if (address < this.minOffset() || address > this.maxOffset()) {
      return {
        completed: false, advance: false, pause: true, sync: false, error: Target.BaseAccessError.OOBAccess,
      };
    }

    const modulus = BigInt(this.#storage.length);
    const ret = this.#pushWriteTrace(address, data, op);
    if (!ret.success && this.#traceBuffer) { this.#traceBuffer.pop(); } else {
      for (let it = 0; it < data.length; it += 1) {
        const effective = (address + BigInt(it) - this.minOffset()) % modulus;
        this.#storage[Number(effective)] = data[it];
      }
      if (this.#traceBuffer) this.#traceBuffer.stage();
    }

    let pause = false;
    let error = Target.BaseAccessError.Success;
    if (op.effectful && this.#interposer) {
      switch (this.#interposer.tryWrite(address, data, op)) {
        case Interposer.InterposeResult.Breakpoint:
          pause = true;
          error = Target.BaseAccessError.Breakpoint;
          break;
        default:
          break;
      }
    }

    return {
      completed: true, advance: true, pause, sync: ret.overflow, error,
    };
  }

  resize(newMaxOffset:number, newMinOffset?:number) {
    this.#maxOffset = BigInt(newMaxOffset);
    this.#minOffset = newMinOffset ? BigInt(newMinOffset) : this.#minOffset;
    this.#storage = new Uint8Array(Number(this.#maxOffset - this.#minOffset) + 1);
    this.#storage.fill(this.#fillValue);
  }

  clear(value:number) {
    this.#fillValue = value;
    this.#storage.fill(this.#fillValue);
  }

  do(trace: TraceTypes.Trace<any>): boolean {
    if (trace.device !== this.deviceID()) return false;
    if (isMatch(trace.type, TraceTypes.Basic.BasicType)) {
      const { payload } = trace as TraceTypes.Trace<TraceTypes.Basic.BasicData>;
      const modulus = BigInt(this.#storage.length);
      for (let it = 0; it < payload.newValue.length; it += 1) {
        const effective = (payload.address + BigInt(it)) % modulus;
        this.#storage[Number(effective)] = payload.newValue[it];
      }
      return true;
    }
    return false;
  }

  undo(trace: TraceTypes.Trace<any>): boolean {
    if (trace.device !== this.deviceID()) return false;
    if (isMatch(trace.type, TraceTypes.Basic.BasicType)) {
      const { payload } = trace as TraceTypes.Trace<TraceTypes.Basic.BasicData>;
      const modulus = BigInt(this.#storage.length);
      for (let it = 0; it < payload.oldValue.length; it += 1) {
        const effective = (payload.address + BigInt(it)) % modulus;
        this.#storage[Number(effective)] = payload.oldValue[it];
      }
      return true;
    }
    return false;
  }

  #minOffset: bigint;

  #maxOffset: bigint;

  #device: Device.DevicePOD;

  #storage: Uint8Array;

  #fillValue: number;

  #traceBuffer: null|Trace.TraceBuffer = null;

  #interposer: null|Interposer.NonNativeInterposer = null;
}

export const getRegistration = () => {
  const create = (node:T.Device.Memory.Type, path:string, id:()=>number) => {
    const pod:Device.DevicePOD = {
      baseName: node.name,
      fullName: `${path}/${node.name}`,
      compatible: node.compatible,
      deviceID: id(),
    };
    // eslint-disable-next-line radix
    return new Dense(pod, BigInt(parseInt(node.minOffset, undefined)), BigInt(parseInt(node.maxOffset, undefined)), 0);
  };
  return { match: { ...T.Device.Memory.Match, storage: 'dense' }, construct: create };
};
