import { Device, Target } from '@pepnext/device-interface';
// eslint-disable-next-line import/no-extraneous-dependencies
import { JZTB } from '@pepnext/sim-jztb';
// eslint-disable-next-line import/no-extraneous-dependencies
import { JLLTB } from '@pepnext/sim-jlltb';
import { JMMO } from '../src/mmo';

type DevicePOD = Device.DevicePOD
const { BaseAccessError } = Target;
describe('JDense', () => {
  const devicePOD: DevicePOD = {
    baseName: 'magic',
    fullName: '/magic',
    deviceID: 0,
    compatible: 'test',
  };
  const tb = new JZTB();
  const rw = { speculative: false, effectful: true, data: true } as const;
  const gs = { speculative: false, effectful: false, data: true } as const;
  it('handles device identification', () => {
    const mem = new JMMO(devicePOD, 0n, 0x10n, 0);
    expect(mem.baseName()).toEqual(devicePOD.baseName);
    expect(mem.fullName()).toEqual(devicePOD.fullName);
    expect(mem.deviceID()).toEqual(devicePOD.deviceID);
    expect(mem.compatible()).toEqual(devicePOD.compatible);
  });
  it('respects memory bounds', () => {
    const mem = new JMMO(devicePOD, 0n, 0x10n, 0);
    expect(mem.maxOffset() === 0x10n);
    expect(() => { mem.resize(0x20n); }).not.toThrow();
    expect(mem.maxOffset() === 0x20n);
  });
  it('does not throw for in-bounds reads', () => {
    const mem = new JMMO(devicePOD, 0n, 0x10n, 0);
    mem.setTraceBuffer(tb);
    for (let it = 0n; it <= mem.maxOffset(); it += 1n) {
      expect(() => mem.read(it, 1, rw)).not.toThrow();
      expect(() => mem.read(it, 1, gs)).not.toThrow();
      expect(mem.read(it, 1, rw).completed).toEqual(true);
      expect(mem.read(it, 1, gs).completed).toEqual(true);
    }
  });
  it('does not throw for in-bounds writes', () => {
    const mem = new JMMO(devicePOD, 0n, 0x10n, 0);
    mem.setTraceBuffer(tb);
    for (let it = 0n; it <= mem.maxOffset(); it += 1n) {
      const data = new Uint8Array([0xfe]);
      expect(() => mem.write(it, data, rw)).not.toThrow();
      expect(() => mem.write(it, data, gs)).not.toThrow();
      expect(mem.write(it, data, rw).completed).toEqual(true);
      expect(mem.write(it, data, gs).completed).toEqual(true);
    }
  });
  it('fails out-of-bounds reads', () => {
    const mem = new JMMO(devicePOD, 0n, 0x10n, 0);
    mem.setTraceBuffer(tb);
    const read = mem.read(mem.maxOffset() + 1n, 1, rw);
    const get = mem.read(mem.maxOffset() + 1n, 1, gs);
    expect(read.completed).toEqual(false);
    expect(read.error = BaseAccessError.OOBAccess);
    expect(get.completed).toEqual(false);
    expect(get.error = BaseAccessError.OOBAccess);
  });
  it('fails out-of-bounds writes', () => {
    const mem = new JMMO(devicePOD, 0n, 0x10n, 0);
    mem.setTraceBuffer(tb);
    const data = new Uint8Array([0xfe]);
    const write = mem.write(mem.maxOffset() + 1n, data, rw);
    expect(write.completed).toEqual(false);
    expect(write.error = BaseAccessError.OOBAccess);
  });
  it('persists all bytes', () => {
    const mem = new JMMO(devicePOD, 0n, 0x10n, 0);
    mem.setTraceBuffer(tb);
    const data = new Uint8Array([0xfe]);
    for (let it = 0n; it < mem.maxOffset(); it += 1n) {
      const write = mem.write(it, data, rw);
      expect(write.completed).toEqual(true);
      expect(write.error).toEqual(BaseAccessError.Success);
      const read = mem.read(it, 1, rw);
      expect(read.completed).toEqual(true);
      expect(read.completed ? read.data : new Uint8Array(0)).toEqual(data);
    }
  });
  it('can clear all bytes', () => {
    const mem = new JMMO(devicePOD, 0n, 0x10n, 0);
    mem.setTraceBuffer(tb);
    const data1 = new Uint8Array([0xfe]);
    for (let it = 0n; it < mem.maxOffset(); it += 1n) {
      const write = mem.write(it, data1, rw);
      expect(write.completed).toEqual(true);
      expect(write.error).toEqual(BaseAccessError.Success);
      const read = mem.read(it, 1, rw);
      expect(read.completed).toEqual(true);
      expect(read.completed ? read.data : new Uint8Array(0)).toEqual(data1);
    }
    mem.clear(0xca);
    const data2 = new Uint8Array([0xca]);
    for (let it = 0n; it < mem.maxOffset(); it += 1n) {
      const read = mem.read(it, 1, rw);
      expect(read.completed).toEqual(true);
      expect(read.completed ? read.data : new Uint8Array(0)).toEqual(data2);
    }
  });
  it('performs multi-byte wrapped reads/writes', () => {
    const mem = new JMMO(devicePOD, 0n, 0x2n, 0);
    mem.setTraceBuffer(tb);
    const data = new Uint8Array([0xfe, 0xed]);
    const write = mem.write(mem.maxOffset(), data, rw);
    expect(write.completed).toEqual(true);
    expect(write.error = BaseAccessError.OOBAccess);
    const read = mem.read(mem.maxOffset(), 2, rw);
    expect(read.completed).toEqual(true);
    expect(read.completed ? read.data : new Uint8Array(0)).toEqual(data);
    // console.log(read.completed ? read.data : 'fail');
  });
  it('participates in undo', () => {
    const mem = new JMMO(devicePOD, 0n, 0x10n, 0x02);
    const lltb = new JLLTB();
    mem.setTraceBuffer(lltb);
    lltb.traceDevice(mem.deviceID(), true);
    lltb.tick(1);
    const data = new Uint8Array(Number(mem.maxOffset() + 1n));
    data.fill(0x0d);
    const writeResult = mem.write(0n, data, rw);
    expect(writeResult.completed).toEqual(true);
    for (let it = 0n; it < mem.maxOffset(); it += 1n) {
      const readResult = mem.read(it, 1, rw);
      expect(readResult.completed).toBe(true);
      expect(readResult.error).toEqual(BaseAccessError.Success);
      expect(readResult.completed ? readResult.data : []).toEqual(new Uint8Array([0x0d]));
    }
    const discarded = lltb.discard();
    // eslint-disable-next-line no-restricted-syntax
    for (const trace of discarded) {
      mem.undo(trace);
    }
    for (let it = 0n; it < mem.maxOffset(); it += 1n) {
      const readResult = mem.read(it, 1, rw);
      expect(readResult.completed).toBe(true);
      expect(readResult.error).toEqual(BaseAccessError.Success);
      expect(readResult.completed ? readResult.data : []).toEqual(new Uint8Array([0x02]));
    }
  });
  it('allows reading bytes as a steam', () => {
    const mem = new JMMO(devicePOD, 0n, 0x10n, 0);
    mem.setTraceBuffer(tb);
    mem.write(0n, new Uint8Array([0x10]), rw);
    mem.write(0n, new Uint8Array([0x20]), rw);
    mem.write(0n, new Uint8Array([0x30]), rw);
    expect([...mem.bytes(0n)]).toEqual([0x10, 0x20, 0x30]);
  });
});
