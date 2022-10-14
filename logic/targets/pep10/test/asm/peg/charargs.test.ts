import { asm } from '../../../src/lib';

describe('asm.peg for char args', () => {
  it('respects escape sequences \\[bfrntv]', () => {
    for (const char of 'bfrntv') {
      const root = asm.peg.parse(`.COMMAND '\\${char}'\n`);
      // C[0] is default section, C[0].C[0] is first line of code
      const dot = root.C[0].C[0];
      expect(dot.T).toStrictEqual('pseudo');
      if (dot.T !== 'pseudo') throw new Error('impossible');
      // Check that the operand is correct
      expect(dot.A.args).toEqual([{ type: 'char', value: `\\${char}` }]);
    }
  });
});
