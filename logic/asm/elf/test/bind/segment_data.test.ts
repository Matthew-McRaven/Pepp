import { native, segmentToBytes } from '../../src/bind';

describe('ELF32 Writer', () => {
  it('Constructs', () => {
    expect(() => new native.Elf()).not.toThrow();
  });

  it('can create copied bytes', () => {
    const e = new native.Elf();
    e.init(32);

    const s1 = e.addSection('hello');
    s1.setData(new Uint8Array([0xde]));

    const s2 = e.addSection('hello');
    s2.setData(new Uint8Array([0xad]));

    const seg = e.addSegment();
    seg.addSection(s1);
    expect(segmentToBytes(e, seg)).toEqual(new Uint8Array([0xde]));
    seg.addSection(s2);
    expect(segmentToBytes(e, seg)).toEqual(new Uint8Array([0xde, 0xad]));
  });
});
