import { native } from '../../src/bind';

describe('ELF32 Writer', () => {
  it('Constructs', () => {
    expect(() => new native.Elf()).not.toThrow();
  });

  it('can create copied bytes', () => {
    const e = new native.Elf();
    e.init(32);

    const s = e.addSection('hello');
    expect(s.getData().length).toEqual(0);

    // Set
    const actual = new Uint8Array([0xfe, 0xed]);
    s.setData(actual);
    let data = s.getData();
    expect(data.length).toEqual(actual.length);
    expect(data).toEqual(actual);

    // Append
    s.appendData(new Uint8Array([0xda]));
    data = s.getData();
    expect(data.length).toEqual(actual.length + 1);
    expect(data).toEqual(new Uint8Array([...actual, 0xda]));
  });
});
