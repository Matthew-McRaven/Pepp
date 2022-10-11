import { asm } from '../../../src/lib';

describe('asm.peg for unary', () => {
  it('detects unary instructions at start of line', () => {
    const root = asm.peg.parse('hi\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const unary = root.C[0].C[0];
    // Check that the unary op is correct
    expect(unary.A.op).toBe('hi');
    expect(unary.type).toBe('unary');
  });
  it('detects unary instructions with comments', () => {
    const root = asm.peg.parse('hi;world\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const unary = root.C[0].C[0];
    // Check that the unary op is correct
    expect(unary.A.op).toBe('hi');
    expect(unary.type).toBe('unary');
    expect(unary.A.comment).toBe('world');
  });
  it('detects unary instructions with whitespace', () => {
    const root = asm.peg.parse('  hi\n\tworld\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const unary0 = root.C[0].C[0];
    // Check that the comment's value is correct
    expect(unary0.A.op).toBe('hi');
    expect(unary0.type).toBe('unary');
    // C[0] is default section, C[0].C[1] is second line of code
    const unary1 = root.C[0].C[1];
    // Check that the unary op is correct
    expect(unary1.A.op).toBe('world');
    expect(unary1.type).toBe('unary');
  });
});
