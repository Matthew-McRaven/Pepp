import { asm } from '../../../src/lib';

describe('asm.peg for blank', () => {
  it('detects single blank line', () => {
    const root = asm.peg.parse('\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const unary = root.C[0].C[0];
    expect(unary.T).toBe('blank');
  });
  it('detects blank line with whitespace', () => {
    const root = asm.peg.parse(' \t\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const unary = root.C[0].C[0];
    expect(unary.T).toBe('blank');
  });
});
