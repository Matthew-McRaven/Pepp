import { ILeafTableNative, TraversalPolicy } from '@pepnext/logic-symbol';
import { asm } from '../../../src/lib';

describe('asm.visit.extractSymbols', () => {
  it('processes program stress test', () => {
    const tree = asm.peg.parseRoot('a:.WORD 5\nADDA x,d\n n:.equate n');
    asm.visit.extractSymbols(tree);
    const symbols = (tree.A.symtab as ILeafTableNative).enumerateSymbols(TraversalPolicy.children);
    expect(symbols.length).toEqual(3);
  });
});
