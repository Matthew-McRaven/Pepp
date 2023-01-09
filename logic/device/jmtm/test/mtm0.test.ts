// import toBuffer from 'typedarray-to-buffer';
import { TargetOperation, Clock, Device } from '@pepnext/device-interface';
import { JLLTB as LLTB } from '@pepnext/sim-jlltb';
import { MTM0 } from '../src/mtm0';
import { MTMRegister } from '../src/mtm_socket';
// import { JConditionalBreakpointInterposer } from '../../sim/sim/bp_interposer/src';

type Operation = TargetOperation.Operation
const { TickError } = Clock;
type DevicePOD = Device.DevicePOD

describe('McRaven Test Machine v0', () => {
  const pod:DevicePOD = {
    baseName: 'test',
    fullName: '/cluster0/test0',
    compatible: 'mtm0',
    deviceID: 0,
  };
  const op: Operation = {
    data: true,
    effectful: true,
    speculative: false,
  };
  it('Can be constructed from a pod', () => {
    expect(() => new MTM0(pod, '/dummy', 1)).not.toThrow();
  });
  it('Can be ticked', () => {
    const mtm0 = new MTM0(pod, '/dummy', 1);
    let read = mtm0.register.read(BigInt(MTMRegister.PC), 2, op);
    expect(read.completed).toEqual(true);
    expect(read.completed ? read.data : []).toEqual(new Uint8Array([0x0, 0x0]));
    const tick = mtm0.tick(1);
    expect(tick.pause).toEqual(false);
    expect(tick.sync).toEqual(false);
    expect(tick.error).toEqual(TickError.Success);
    expect(tick.delay).toEqual({ kind: 'clock', period: 1 });
    read = mtm0.register.read(BigInt(MTMRegister.PC), 2, op);
    expect(read.completed).toEqual(true);

    expect(read.completed ? read.data : []).toEqual(new Uint8Array([0x0, 0x1]));
  });
  it('Logs deltas for registers in the trace buffer', () => {
    const mtm0 = new MTM0(pod, '/dummy', 1);
    const tb = new LLTB();
    tb.traceDevice(mtm0.deviceID(), true);
    tb.traceDevice(mtm0.register.deviceID(), true);
    mtm0.setTraceBuffer(tb);
    mtm0.register.setTraceBuffer(tb);
    tb.tick(1);
    mtm0.tick(1);
    tb.tick(2);
    mtm0.tick(2);
    const inc = {
      device: 0,
      payload: undefined,
      type: {
        dataKind: 'inc-cycle-count',
        deviceKind: 'mtmv0',
        isDelta: true,

      },
    };
    expect([...tb.staged()]).toEqual([
      { ...inc, tick: 1 }, {
        device: 1,
        payload: {
          address: 0n,
          oldValue: new Uint8Array([0, 0]),
          newValue: new Uint8Array([0, 1]),
        },
        type: {
          dataKind: 'flat',
          deviceKind: 'memory',
          isDelta: true,
        },
        tick: 1,
      },
      { ...inc, tick: 2 },
      {
        device: 1,
        payload: {
          address: 0n,
          oldValue: new Uint8Array([0, 1]),
          newValue: new Uint8Array([0, 2]),
        },
        type: {
          dataKind: 'flat',
          deviceKind: 'memory',
          isDelta: true,
        },
        tick: 2,
      }]);
  });
  /* it('Can have PC breakpoints set', () => {
    const mtm0 = new MTM0(pod, '/dummy', 1);
    const bp = new JConditionalBreakpointInterposer();
    const cond = () => {
      const result = mtm0.register.read(BigInt(MTMRegister.PC), 2, op);
      if (!result.completed) return false;
      return toBuffer(result.data).readInt16BE() === 0x01;
    };
    bp.addBreakpoint(BigInt(MTMRegister.PC), cond);
    mtm0.register.setInterposer(bp);
    let tick = mtm0.tick(1);
    expect(tick.pause).toEqual(true);
    expect(tick.error).toEqual(tickError.Breakpoint);
    tick = mtm0.tick(2);
    expect(tick.pause).toEqual(false);
    expect(tick.error).toEqual(tickError.Success);
  }); */
  it('Increments its instruction/cycle counts', () => {
    const mtm0 = new MTM0(pod, '/dummy', 1);
    expect(mtm0.cycleCount()).toEqual(0);
    expect(mtm0.instructionCount()).toEqual(0);
    mtm0.tick(1);
    expect(mtm0.cycleCount()).toEqual(1);
    expect(mtm0.instructionCount()).toEqual(1);
  });
  it('Participates in undo', () => {
    const mtm0 = new MTM0(pod, '/dummy', 1);
    const tb = new LLTB();
    mtm0.setTraceBuffer(tb);
    tb.traceDevice(mtm0.deviceID(), true);
    tb.tick(1);
    mtm0.tick(1);
    expect(mtm0.cycleCount()).toEqual(1);
    expect(mtm0.instructionCount()).toEqual(1);
    const discarded = tb.discard(0);
    // eslint-disable-next-line no-restricted-syntax
    for (const trace of discarded) {
      mtm0.undo(trace);
    }
    expect(mtm0.cycleCount()).toEqual(0);
    expect(mtm0.instructionCount()).toEqual(0);
  });
  it('Resets correctly', () => {
    const mtm0 = new MTM0(pod, '/dummy', 1);
    mtm0.tick(1);
    expect(mtm0.cycleCount()).toEqual(1);
    expect(mtm0.instructionCount()).toEqual(1);
    mtm0.reset();
    expect(mtm0.cycleCount()).toEqual(0);
    expect(mtm0.instructionCount()).toEqual(0);
    const read = mtm0.register.read(BigInt(MTMRegister.PC), 2, op);
    expect(read.completed).toEqual(true);
    expect(read.completed ? read.data : []).toEqual(new Uint8Array([0x0, 0x0]));
  });
});
