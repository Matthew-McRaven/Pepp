import { asm } from '../../../src/lib';

describe('asm.peg for macro', () => {
  it('detects 0-arity macro at the start of the line', () => {
    const root = asm.peg.parseRoot('@END\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const macro = root.C[0].C[0];
    expect(macro.T).toStrictEqual('macro');
    if (macro.T !== 'macro') throw new Error('impossible');
    // Check that the comment's value is correct
    expect(macro.A.macro).toBe('END');
    expect(macro.A.args.length).toBe(0);
  });
  it('detects 0-arity macro preceded by whitespace', () => {
    const root = asm.peg.parseRoot('\t @END\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const macro = root.C[0].C[0];
    expect(macro.T).toStrictEqual('macro');
    if (macro.T !== 'macro') throw new Error('impossible');
    // Check that the macro is correct
    expect(macro.A.macro).toBe('END');
    expect(macro.A.args.length).toBe(0);
  });
  it('detects 1-arity macro at the start of the line', () => {
    const root = asm.peg.parseRoot('@BLAGH 1\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const macro = root.C[0].C[0];
    expect(macro.T).toStrictEqual('macro');
    if (macro.T !== 'macro') throw new Error('impossible');
    // Check that the macro is correct
    expect(macro.A.macro).toBe('BLAGH');
    expect(macro.A.args.length).toBe(1);
    expect(macro.A.args[0].type).toStrictEqual('decimal');
    expect(macro.A.args[0].value.toString()).toStrictEqual('1');
  });
  it('detects >1-arity macro at the start of the line', () => {
    const root = asm.peg.parseRoot('@BLAGH 1,2,3\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const macro = root.C[0].C[0];
    expect(macro.T).toStrictEqual('macro');
    if (macro.T !== 'macro') throw new Error('impossible');
    // Check that the macro is correct
    expect(macro.A.macro).toBe('BLAGH');
    // Check that all args are correct.
    expect(macro.A.args.length).toBe(3);
    expect(macro.A.args[0].type).toStrictEqual('decimal');
    expect(macro.A.args[0].value.toString()).toStrictEqual('1');
    expect(macro.A.args[1].type).toStrictEqual('decimal');
    expect(macro.A.args[1].value.toString()).toStrictEqual('2');
    expect(macro.A.args[2].type).toStrictEqual('decimal');
    expect(macro.A.args[2].value.toString()).toStrictEqual('3');
  });
});
