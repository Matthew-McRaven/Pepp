import { Interposer, TargetOperation } from '@pepnext/device-interface';

type Operation = TargetOperation.Operation
type InterposeResultType = Interposer.InterposeResult
const { InterposeResult } = Interposer;
type NonNativeInterposer = Interposer.NonNativeInterposer

// eslint-disable-next-line import/prefer-default-export
export class NonNativeAddressBreakpointInterposer implements NonNativeInterposer {
  constructor() {
    this.#breakpoints = new Set<bigint>();
  }

  addBreakpoint(address:bigint) {
    this.#breakpoints.add(address);
  }

  removeBreakpoint(address:bigint) {
    this.#breakpoints.delete(address);
  }

  removeAll() {
    this.#breakpoints.clear();
  }

  tryRead(address: bigint, count: number, op: Operation): InterposeResultType {
    // eslint-disable-next-line no-void
    void op;
    for (let it = 0; it < count; it += 1) {
      if (this.#breakpoints.has(BigInt(it) + address)) return InterposeResult.Breakpoint;
    }
    return InterposeResult.Success;
  }

  tryWrite(address: bigint, data: Uint8Array, op: Operation): InterposeResultType {
    // eslint-disable-next-line no-void
    void op;
    for (let it = 0; it < data.length; it += 1) {
      if (this.#breakpoints.has(BigInt(it) + address)) return InterposeResult.Breakpoint;
    }
    return InterposeResult.Success;
  }

  #breakpoints: Set<bigint>;
}
