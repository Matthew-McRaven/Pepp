import { Interposer, TargetOperation } from '@pepnext/device-interface';

type Operation = TargetOperation.Operation
type InterposeResultType = Interposer.InterposeResult
const { InterposeResult } = Interposer;

type NonNativeInterposer = Interposer.NonNativeInterposer

// eslint-disable-next-line import/prefer-default-export
export class NonNativeValueBreakpointInterposer implements NonNativeInterposer {
  constructor() {
    this.#breakpoints = new Map< bigint, Array<(address:bigint)=>boolean>>();
  }

  addBreakpoint(address:bigint, condition: (address:bigint)=>boolean) {
    if (this.#breakpoints.has(address)) {
      this.#breakpoints.get(address)!.push(condition);
    } else {
      this.#breakpoints.set(address, [condition]);
    }
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
      const e = BigInt(it) + address; // effective
      if (this.#breakpoints.has(e) && this.#breakpoints.get(e)!
        .map((c) => c(e)).reduce((p, c) => p || c, false)) return InterposeResult.Breakpoint;
    }
    return InterposeResult.Success;
  }

  #breakpoints: Map< bigint, Array<(address:bigint)=>boolean>>;
}
