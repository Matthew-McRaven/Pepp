import { native, StringCache } from '../../src/bind';

describe('ELF32 Writer', () => {
  it('Constructs', () => {
    expect(() => new native.Elf(32, new StringCache())).not.toThrow();
  });
  it('can set metadata', () => {
    expect(() => {
      const e = new native.Elf(32, new StringCache());
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

      e.addSection('hello');
      e.getSection(0n);

      e.addSegment();
      e.getSegment(0n);
    }).not.toThrow();
  });
});
