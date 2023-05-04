import { JZTB } from '@pepnext/sim-jztb';
import { Device, Target } from '@pepnext/device-interface';
import { JLLTB } from '@pepnext/sim-jlltb';
import { JMMI } from '../src/mmi';

type DevicePOD = Device.DevicePOD
const { BaseAccessError } = Target;

describe('JMMI', () => {
  const devicePOD: DevicePOD = {
    baseName: 'magic',
    fullName: '/magic',
    deviceID: 0,
    compatible: 'test',
  };
  const tb = new JZTB();
  const rw = { speculative: false, effectful: true, data: true } as const;
  const gs = { speculative: true, effectful: false, data: true } as const;
  it('handles device identification', () => {
    const mem = new JMMI(devicePOD, 0n, 0x10n, 0);
    expect(mem.baseName()).toEqual(devicePOD.baseName);
    expect(mem.fullName()).toEqual(devicePOD.fullName);
    expect(mem.deviceID()).toEqual(devicePOD.deviceID);
    expect(mem.compatible()).toEqual(devicePOD.compatible);
  });
  it('respects memory bounds', () => {
    const mem = new JMMI(devicePOD, 0n, 0x10n, 0);
    expect(mem.maxOffset() === 0x10n);
    expect(() => { mem.resize(0x20n); }).not.toThrow();
    expect(mem.maxOffset() === 0x20n);
  });
  it('does not throw for in-bounds reads', () => {
    const mem = new JMMI(devicePOD, 0n, 0x10n, 0);
    mem.setTraceBuffer(tb);
    for (let it = 0n; it <= mem.maxOffset(); it += 1n) {
      expect(mem.read(it, 1, gs).completed).toEqual(true);
      expect(mem.read(it, 1, rw).completed).toEqual(false);
    }
  });
  it('does not throw for in-bounds writes', () => {
    const mem = new JMMI(devicePOD, 0n, 0x10n, 0);
    mem.setTraceBuffer(tb);
    for (let it = 0n; it <= mem.maxOffset(); it += 1n) {
      const data = new Uint8Array([0xfe]);
      expect(() => mem.write(it, data, rw)).not.toThrow();
      expect(() => mem.write(it, data, gs)).not.toThrow();
      expect(mem.write(it, data, rw).completed).toEqual(true);
      const rr = mem.read(it, 1, gs);
      expect(rr.completed ? rr.data : 0).toEqual(new Uint8Array([0x00]));
      expect(mem.write(it, data, gs).completed).toEqual(true);
      expect(rr.completed ? rr.data : 0).toEqual(new Uint8Array([0x00]));
    }
  });
  it('fails out-of-bounds reads', () => {
    const mem = new JMMI(devicePOD, 0n, 0x10n, 0);
    mem.setTraceBuffer(tb);
    const read = mem.read(mem.maxOffset() + 1n, 1, rw);
    const get = mem.read(mem.maxOffset() + 1n, 1, gs);
    expect(read.completed).toEqual(false);
    expect(read.error = BaseAccessError.OOBAccess);
    expect(get.completed).toEqual(false);
    expect(get.error = BaseAccessError.OOBAccess);
  });
  it('fails out-of-bounds writes', () => {
    const mem = new JMMI(devicePOD, 0n, 0x10n, 0);
    mem.setTraceBuffer(tb);
    const data = new Uint8Array([0xfe]);
    const write = mem.write(mem.maxOffset() + 1n, data, rw);
    expect(write.completed).toEqual(false);
    expect(write.error = BaseAccessError.OOBAccess);
  });
  it('can clear all bytes', () => {
    const mem = new JMMI(devicePOD, 0n, 0x10n, 10);
    mem.setTraceBuffer(tb);
    const data1 = new Uint8Array([0xfe]);
    for (let it = 0n; it < mem.maxOffset(); it += 1n) {
      expect(mem.push(it, 0xfe)).toEqual(true);
      const read = mem.read(it, 1, rw);
      expect(read.completed).toEqual(true);
      expect(read.completed ? read.data : new Uint8Array([0])).toEqual(data1);
    }
    mem.clear(0xca);
    const data2 = new Uint8Array([0xca]);
    for (let it = 0n; it < mem.maxOffset(); it += 1n) {
      const read = mem.read(it, 1, rw);
      expect(read.completed).toEqual(true);
      expect(read.completed ? read.data : new Uint8Array([0x0])).toEqual(data2);
    }
  });
  it('performs multi-byte wrapped reads/writes', () => {
    const mem = new JMMI(devicePOD, 0n, 0x2n, 0);
    mem.setTraceBuffer(tb);
    const data = new Uint8Array([0xfe, 0xed]);
    mem.push(mem.maxOffset(), 0xfe);
    mem.push(0n, 0xed);
    const read = mem.read(mem.maxOffset(), 2, rw);
    expect(read.completed).toEqual(true);
    expect(read.completed ? read.data : new Uint8Array(0)).toEqual(data);
    // console.log(read.completed ? read.data : 'fail');
  });
  it('participates in undo', () => {
    const mem = new JMMI(devicePOD, 0n, 0x1n, 0x02);
    const lltb = new JLLTB();
    mem.setTraceBuffer(lltb);
    lltb.traceDevice(mem.deviceID(), true);
    lltb.tick(1);
    for (let it = mem.minOffset(); it < mem.maxOffset(); it += 1n) {
      mem.push(it, 0x25);
      mem.push(it, 0x26);
    }
    const read = (expected:number) => {
      for (let it = 0n; it < mem.maxOffset(); it += 1n) {
        const readResult = mem.read(it, 1, rw);
        expect(readResult.completed).toBe(true);
        expect(readResult.error).toEqual(BaseAccessError.Success);
        expect(readResult.completed ? readResult.data : []).toEqual(new Uint8Array([expected]));
      }
    };
    read(0x25);
    lltb.stage();
    lltb.tick(2);
    read(0x26);
    const discarded = lltb.discard();
    // eslint-disable-next-line no-restricted-syntax
    for (const trace of discarded) {
      expect(mem.undo(trace)).toEqual(true);
    }
    read(0x26);
  });
  it('returns an "out of MMI" error when reading beyond queued input', () => {
    const mem = new JMMI(devicePOD, 0n, 0x1n, 0x02);
    mem.setTraceBuffer(tb);
    mem.push(0n, 0x25);

    let read = mem.read(0n, 1, rw);
    expect(read.completed).toBe(true);
    expect((<Target.ReadSuccessResult>read).data).toEqual(new Uint8Array([0x25]));
    expect(read.error).toEqual(BaseAccessError.Success);

    read = mem.read(0n, 1, rw);
    expect(read.completed).toBe(false);
    expect(read.error).toEqual(BaseAccessError.NeedsMMI);
  });
  it('doesn\'t advance input on speculative reads', () => {
    const mem = new JMMI(devicePOD, 0n, 0x1n, 0x02);
    mem.setTraceBuffer(tb);
    mem.push(0n, 0x25);

    let read = mem.read(0n, 1, rw);
    expect(read.completed).toBe(true);
    expect((<Target.ReadSuccessResult>read).data).toEqual(new Uint8Array([0x25]));
    expect(read.error).toEqual(BaseAccessError.Success);

    // Speculative access does not advance input pointer.
    read = mem.read(0n, 1, gs);
    expect(read.completed).toBe(true);
    expect((<Target.ReadSuccessResult>read).data).toEqual(new Uint8Array([0x25]));
    expect(read.error).toEqual(BaseAccessError.Success);

    read = mem.read(0n, 1, rw);
    expect(read.completed).toBe(false);
    expect(read.error).toEqual(BaseAccessError.NeedsMMI);
  });
});
