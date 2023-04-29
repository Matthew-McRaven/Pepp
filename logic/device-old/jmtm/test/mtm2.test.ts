import { TargetOperation, Clock, Device } from '@pepnext/device-interface';
import { JDense } from '@pepnext/device-jdense';
import { MTM2 } from '../src/mtm2';
import { MTMRegister } from '../src/mtm_socket';

type Operation = TargetOperation.Operation
const { TickError } = Clock;
type DevicePOD = Device.DevicePOD
// eslint-disable-next-line @typescript-eslint/ban-ts-comment
// @ts-ignore
// eslint-disable-next-line no-extend-native

describe('McRaven Test Machine v2', () => {
  const pod:DevicePOD = {
    baseName: 'test',
    fullName: '/cluster0/test0',
    compatible: 'mtm2',
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
    expect(() => new MTM2(pod, '/dummy', () => 1)).not.toThrow();
  });
  it('Can be ticked', () => {
    let count = 1;
    const id = () => {
      const temp = count;
      count += 1;
      return temp;
    };
    const mtm = new MTM2(pod, '/dummy', id);
    let read = mtm.register.read(BigInt(MTMRegister.PC), 2, op);
    mtm.setTarget(new JDense(ramPOD, 0n, 0xFFFFn, 0));
    expect(read.completed).toEqual(true);
    expect(read.completed ? read.data : []).toEqual(new Uint8Array([0x0, 0x0]));
    mtm.cin.push(0n, [0x10, 0x20, 0x30]);
    const tick = mtm.tick(1);
    expect(tick.pause).toEqual(false);
    expect(tick.sync).toEqual(false);
    expect(tick.error).toEqual(TickError.Success);
    expect(tick.delay).toEqual({ kind: 'clock', period: 1 });
    read = mtm.register.read(BigInt(MTMRegister.PC), 2, op);
    expect(read.completed).toEqual(true);
    expect(mtm.tick(2).error).toEqual(TickError.Success);
    expect(mtm.tick(3).error).toEqual(TickError.Success);
    expect([...mtm.cout.bytes(0n)]).toEqual([0x10, 0x20, 0x30]);
    expect(read.completed ? read.data : []).toEqual(new Uint8Array([0x0, 0x1]));
  });
});
