import { native } from '../../src/bind';

describe('ELF32 Writer', () => {
  it('Constructs', () => {
    expect(() => new native.Elf()).not.toThrow();
  });

  it('can count segments/sections', () => {
    const e = new native.Elf();
    e.init(32);
    e.getClass();
    e.getVersion();
    e.getOSABI();
    e.setOSABI(0n);
    e.getABIVersion();
    e.setABIVersion(0n);
    e.getType();
    e.setType(0n);
    e.getMachine();
    e.setMachine(0n);
    e.getFlags();
    e.setFlags(0n);
    e.getEntry();
    e.setEntry(0n);
    expect(e.sectionCount().toString() === '0n');
    expect(e.segmentCount().toString() === '0n');
    e.addSection('hello');
    expect(e.sectionCount().toString() === '1n');
    expect(e.segmentCount().toString() === '0n');
    e.addSection('world');

    expect(e.sectionCount().toString() === '2n');
    expect(e.segmentCount().toString() === '0n');

    e.addSegment();
    expect(e.sectionCount().toString() === '2n');
    expect(e.segmentCount().toString() === '1n');
  });
});
