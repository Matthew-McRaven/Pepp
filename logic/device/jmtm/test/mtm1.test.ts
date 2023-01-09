// import toBuffer from 'typedarray-to-buffer';
import { TargetOperation, Clock, Device } from '@pepnext/device-interface';
import { JLLTB as LLTB } from '@pepnext/device-jlltb';
import { JDense } from '@pepnext/device-jdense';
import { MTM1 } from '../src/mtm1';
import { MTMRegister } from '../src/mtm_socket';
// import { JConditionalBreakpointInterposer } from '../../sim/sim/bp_interposer/src';

type Operation = TargetOperation.Operation
const { TickError } = Clock;
type DevicePOD = Device.DevicePOD
// eslint-disable-next-line @typescript-eslint/ban-ts-comment
// @ts-ignore
// eslint-disable-next-line no-extend-native

describe('McRaven Test Machine v1', () => {
  const pod:DevicePOD = {
    baseName: 'test',
    fullName: '/cluster0/test0',
    compatible: 'mtm0',
    deviceID: 0,
  };
  const ramPOD: DevicePOD = {
    baseName: 'magic',
    fullName: '/magic',
    deviceID: 3,
    compatible: 'test',
  };
  const op: Operation = {
    data: true,
    effectful: true,
    speculative: false,
  };
  it('Can be constructed from a pod', () => {
    expect(() => new MTM1(pod, '/dummy', 1)).not.toThrow();
  });
  it('Can be ticked', () => {
    const mtm1 = new MTM1(pod, '/dummy', 1);
    let read = mtm1.register.read(BigInt(MTMRegister.PC), 2, op);
    mtm1.setTarget(new JDense(ramPOD, 0n, 0xFFFFn, 0));
    expect(read.completed).toEqual(true);
    expect(read.completed ? read.data : []).toEqual(new Uint8Array([0x0, 0x0]));
    const tick = mtm1.tick(1);
    expect(tick.pause).toEqual(false);
    expect(tick.sync).toEqual(false);
    expect(tick.error).toEqual(TickError.Success);
    expect(tick.delay).toEqual({ kind: 'clock', period: 1 });
    read = mtm1.register.read(BigInt(MTMRegister.PC), 2, op);
    expect(read.completed).toEqual(true);

    expect(read.completed ? read.data : []).toEqual(new Uint8Array([0x0, 0x1]));
  });
  it('Logs deltas for registers in the trace buffer', () => {
    const mtm1 = new MTM1(pod, '/dummy', 1);
    const tb = new LLTB();
    const dense = new JDense(ramPOD, 0n, 0xFFFFn, 0);
    dense.write(0n, new Uint8Array([0x1, 0x2, 0x3, 0x4]), { effectful: true, speculative: false, data: true });
    mtm1.setTarget(dense);
    tb.traceDevice(mtm1.deviceID(), true);
    tb.traceDevice(mtm1.register.deviceID(), true);
    tb.traceDevice(dense.deviceID(), true);
    mtm1.setTraceBuffer(tb);
    mtm1.register.setTraceBuffer(tb);
    tb.tick(1);
    expect(mtm1.tick(1).error).toEqual(TickError.Success);
    tb.tick(2);
    expect(mtm1.tick(2).error).toEqual(TickError.Success);
    const inc = {
      device: 0,
      payload: undefined,
      type: {
        dataKind: 'inc-cycle-count',
        deviceKind: 'mtmv1',
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
      {
        device: 1,
        payload: {
          address: BigInt(MTMRegister.A),
          oldValue: new Uint8Array([0, 0]),
          newValue: new Uint8Array([2, 3]),
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
      },
      {
        device: 1,
        payload: {
          address: BigInt(MTMRegister.A),
          oldValue: new Uint8Array([2, 3]),
          newValue: new Uint8Array([3, 4]),
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
    const mtm1 = new MTM1(pod, '/dummy', 1);
    mtm1.setTarget(new JDense(ramPOD, 0n, 0xFFFFn, 0));
    const bp = new JConditionalBreakpointInterposer();
    const cond = () => {
      const result = mtm1.register.read(BigInt(MTMRegister.PC), 2, op);
      if (!result.completed) return false;
      return toBuffer(result.data).readInt16BE() === 0x01;
    };
    bp.addBreakpoint(BigInt(MTMRegister.PC), cond);
    mtm1.register.setInterposer(bp);
    let tick = mtm1.tick(1);
    expect(tick.pause).toEqual(true);
    expect(tick.error).toEqual(tickError.Breakpoint);
    tick = mtm1.tick(2);
    expect(tick.pause).toEqual(false);
    expect(tick.error).toEqual(tickError.Success);
  }); */
  it('Increments its instruction/cycle counts', () => {
    const mtm1 = new MTM1(pod, '/dummy', 1);
    mtm1.setTarget(new JDense(ramPOD, 0n, 0xFFFFn, 0));
    expect(mtm1.cycleCount()).toEqual(0);
    expect(mtm1.instructionCount()).toEqual(0);
    mtm1.tick(1);
    expect(mtm1.cycleCount()).toEqual(1);
    expect(mtm1.instructionCount()).toEqual(1);
  });
  it('Participates in undo', () => {
    const mtm1 = new MTM1(pod, '/dummy', 1);
    mtm1.setTarget(new JDense(ramPOD, 0n, 0xFFFFn, 0));
    const tb = new LLTB();
    mtm1.setTraceBuffer(tb);
    tb.traceDevice(mtm1.deviceID(), true);
    tb.tick(1);
    mtm1.tick(1);
    expect(mtm1.cycleCount()).toEqual(1);
    expect(mtm1.instructionCount()).toEqual(1);
    const discarded = tb.discard(0);
    // eslint-disable-next-line no-restricted-syntax
    for (const trace of discarded) {
      mtm1.undo(trace);
    }
    expect(mtm1.cycleCount()).toEqual(0);
    expect(mtm1.instructionCount()).toEqual(0);
  });
  it('Resets correctly', () => {
    const mtm1 = new MTM1(pod, '/dummy', 1);
    mtm1.setTarget(new JDense(ramPOD, 0n, 0xFFFFn, 0));
    mtm1.tick(1);
    expect(mtm1.cycleCount()).toEqual(1);
    expect(mtm1.instructionCount()).toEqual(1);
    mtm1.reset();
    expect(mtm1.cycleCount()).toEqual(0);
    expect(mtm1.instructionCount()).toEqual(0);
    const read = mtm1.register.read(BigInt(MTMRegister.PC), 2, op);
    expect(read.completed).toEqual(true);
    expect(read.completed ? read.data : []).toEqual(new Uint8Array([0x0, 0x0]));
  });
});
