import { asm } from '../../../src/lib';

describe('asm.peg for unary', () => {
  it('detects unary instructions at start of line', () => {
    const root = asm.peg.parseRoot('hi\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const unary = root.C[0].C[0];
    expect(unary.T).toStrictEqual('unary');
    if (unary.T !== 'unary') throw new Error('impossible');
    // Check that the unary op is correct
    expect(unary.A.op).toBe('hi');
  });
  it('detects unary instructions with comments', () => {
    const root = asm.peg.parseRoot('hi;world\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const unary = root.C[0].C[0];
    expect(unary.T).toStrictEqual('unary');
    if (unary.T !== 'unary') throw new Error('impossible');
    // Check that the unary op is correct
    expect(unary.A.op).toBe('hi');
    expect(unary.A.comment).toBe('world');
  });
  it('detects unary instructions with whitespace', () => {
    const root = asm.peg.parseRoot('  hi\n\tworld\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const unary0 = root.C[0].C[0];
    expect(unary0.T).toStrictEqual('unary');
    if (unary0.T !== 'unary') throw new Error('impossible');
    // Check that the comment's value is correct
    expect(unary0.A.op).toBe('hi');
    // C[0] is default section, C[0].C[1] is second line of code
    const unary1 = root.C[0].C[1];
    expect(unary1.T).toStrictEqual('unary');
    if (unary1.T !== 'unary') throw new Error('impossible');
    // Check that the unary op is correct
    expect(unary1.A.op).toBe('world');
  });
});
