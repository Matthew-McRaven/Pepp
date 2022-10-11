import { asm } from '../../../src/lib';

describe('asm.peg for section', () => {
  it('default+1section', () => {
    const root = asm.peg.parse('asra\n.SECTION data\n.END');
    // Check that there are two sections with one line each.
    expect(root.C.length).toBe(2);
    expect(root.C[0].C.length).toBe(1);
    expect(root.C[1].C.length).toBe(1);
  });
});
