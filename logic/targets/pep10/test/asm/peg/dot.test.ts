import { asm } from '../../../src/lib';

describe('asm.peg for psuedo', () => {
  it('detects 0-arity dot commands at the start of the line', () => {
    const root = asm.peg.parse('.END\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const psuedo = root.C[0].C[0];
    // Check that the comment's value is correct
    expect(psuedo.A.directive).toBe('END');
    expect(psuedo.type).toBe('pseudo');
    expect(psuedo.A.args.length).toBe(0);
  });
  it('detects 0-arity dot commands preceded by whitespace', () => {
    const root = asm.peg.parse('\t .END\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const psuedo = root.C[0].C[0];
    // Check that the directive is correct
    expect(psuedo.A.directive).toBe('END');
    expect(psuedo.type).toBe('pseudo');
    expect(psuedo.A.args.length).toBe(0);
  });
  it('detects 1-arity dot commands at the start of the line', () => {
    const root = asm.peg.parse('.BLAGH 1\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const psuedo = root.C[0].C[0];
    // Check that the directive is correct
    expect(psuedo.A.directive).toBe('BLAGH');
    expect(psuedo.type).toBe('pseudo');
    expect(psuedo.A.args.length).toBe(1);
    expect(psuedo.A.args[0]).toBe(1);
  });
  it('detects >1-arity dot commands at the start of the line', () => {
    const root = asm.peg.parse('.BLAGH 1,2,3\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const psuedo = root.C[0].C[0];
    // Check that the directive is correct
    expect(psuedo.A.directive).toBe('BLAGH');
    expect(psuedo.type).toBe('pseudo');
    // Check that all args are correct.
    expect(psuedo.A.args.length).toBe(3);
    expect(psuedo.A.args[0]).toBe(1);
    expect(psuedo.A.args[1]).toBe(2);
    expect(psuedo.A.args[2]).toBe(3);
  });
});
