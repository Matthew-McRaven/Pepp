import { native } from '../../src/bind';

describe('ELF32 Writer', () => {
  it('Constructs', () => {
    expect(() => new native.Elf()).not.toThrow();
  });
  it('can set metadata', () => {
    const e = new native.Elf();
    e.init(32);
    const s = e.addSection('hello');
    s.getIndex();
    s.getName();
    s.setName('boo');
    s.getType();
    s.setType(0n);
    s.getFlags();
    s.setFlags(0n);
    s.getInfo();
    s.setInfo(0n);
    s.getAlign();
    s.setAlign(0n);
    s.getAddress();
    s.setAddress(0n);
    s.getSize();
    s.setData(Uint8Array.from([]));
    s.appendData(Uint8Array.from([]));
  });
});
