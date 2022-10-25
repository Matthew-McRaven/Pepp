import { Writer } from '../../src/bind';

describe('ELF32 Writer', () => {
  it('Constructs', () => {
    expect(() => new Writer(32)).not.toThrow();
  });
  it('can set metadata', () => {
    expect(() => {
      const wr = new Writer(32);
      wr.writeEMachine(1n);
      wr.writeEntryPoint(1n);
      wr.writeEType(1n);
      wr.writeOSABI(1n);
    }).not.toThrow();
  });
  it('can cache string', () => {
    const wr = new Writer(32);
    const hello1 = wr.writeString('xtab', 'hello');
    const world7 = wr.writeString('xtab', 'world');
    expect(hello1.toString()).toEqual('1');
    expect(world7.toString()).toEqual('7');
    expect(wr.writeString('xtab', 'hello').toString()).toEqual(hello1.toString());
    expect(wr.writeString('xtab', 'world').toString()).toEqual(world7.toString());
    expect(wr.writeString('xtab2', 'world').toString()).toEqual('1');
  });
  it('can write section bytes without crashing', () => {
    const wr = new Writer(32);
    const bytes = Uint8Array.from([0x01, 0x02, 0x04, 0x08, 0x10]);
    wr.writeSectionBytes('.text', {} as any, bytes);
    wr.writeSectionBytes('test', {} as any, new TextEncoder().encode('hello cruel world'));
  });
});
