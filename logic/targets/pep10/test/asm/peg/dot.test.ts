import { asm } from '../../../src/lib';

describe('asm.peg for pseudo', () => {
  it('detects 0-arity dot commands at the start of the line', () => {
    const root = asm.peg.parseRoot('.END\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const pseudo = root.C[0].C[0];
    expect(pseudo.T).toStrictEqual('pseudo');
    if (pseudo.T !== 'pseudo') throw new Error('impossible');
    // Check that the comment's value is correct
    expect(pseudo.A.directive).toBe('END');
    expect(pseudo.A.args.length).toBe(0);
  });
  it('detects 0-arity dot commands preceded by whitespace', () => {
    const root = asm.peg.parseRoot('\t .END\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const pseudo = root.C[0].C[0];
    expect(pseudo.T).toStrictEqual('pseudo');
    if (pseudo.T !== 'pseudo') throw new Error('impossible');
    // Check that the directive is correct
    expect(pseudo.A.directive).toBe('END');
    expect(pseudo.A.args.length).toBe(0);
  });
  it('detects 1-arity dot commands at the start of the line', () => {
    const root = asm.peg.parseRoot('.BLAGH 1\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const pseudo = root.C[0].C[0];
    expect(pseudo.T).toStrictEqual('pseudo');
    if (pseudo.T !== 'pseudo') throw new Error('impossible');
    // Check that the directive is correct
    expect(pseudo.A.directive).toBe('BLAGH');
    expect(pseudo.A.args.length).toBe(1);
    expect(pseudo.A.args[0].type).toStrictEqual('decimal');
    expect(pseudo.A.args[0].value.toString()).toStrictEqual('1');
  });
  it('detects >1-arity dot commands at the start of the line', () => {
    const root = asm.peg.parseRoot('.BLAGH 1,2,3\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const pseudo = root.C[0].C[0];
    expect(pseudo.T).toStrictEqual('pseudo');
    if (pseudo.T !== 'pseudo') throw new Error('impossible');
    // Check that the directive is correct
    expect(pseudo.A.directive).toBe('BLAGH');
    // Check that all args are correct.
    expect(pseudo.A.args.length).toBe(3);
    expect(pseudo.A.args[0].type).toStrictEqual('decimal');
    expect(pseudo.A.args[0].value.toString()).toStrictEqual('1');
    expect(pseudo.A.args[1].type).toStrictEqual('decimal');
    expect(pseudo.A.args[1].value.toString()).toStrictEqual('2');
    expect(pseudo.A.args[2].type).toStrictEqual('decimal');
    expect(pseudo.A.args[2].value.toString()).toStrictEqual('3');
  });
});
