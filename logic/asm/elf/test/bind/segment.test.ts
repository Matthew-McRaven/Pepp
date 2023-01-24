import { native } from '../../src/bind';

describe('ELF32 Writer', () => {
  it('Constructs', () => {
    expect(() => new native.Elf()).not.toThrow();
  });
  it('can set metadata', () => {
    const e = new native.Elf();
    e.init(32);
    const sec = e.addSection('hello');
    const seg = e.addSegment();
    seg.getIndex();
    seg.getType();
    seg.setType(0n);
    seg.getAlign();
    seg.setAlign(0n);
    seg.getVAddress();
    seg.setVAddress(0n);
    seg.getPAddress();
    seg.setPAddress(0n);
    seg.getMemorySize();
    seg.setMemorySize(10n);
    seg.getFileSize();
    seg.getFlags();
    seg.setFlags(7n);
    seg.addSectionIndex(0n);
    seg.addSection(sec);
    expect(seg.getSectionCount().toString()).toEqual('2');
    seg.recomputeFileSize();
  });
});
