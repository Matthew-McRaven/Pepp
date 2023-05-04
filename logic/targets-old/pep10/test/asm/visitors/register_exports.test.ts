import { DefintionState, TraversalPolicy } from '@pepnext/logic-symbol';
import { asm } from '../../../src/lib';

describe('asm.visit.registerExports', () => {
  it('sets a single symbol to global', () => {
    const tree = asm.peg.parseRoot('false:.EQUATE 0\n.EXPORT false');
    asm.visit.extractSymbols(tree);
    asm.visit.registerExports(tree);
    const symbols = tree.A.symtab.find('false', TraversalPolicy.wholeTree);
    expect(symbols.length).toEqual(1);
    expect(symbols[0].definitionState()).toEqual(DefintionState.single);
    expect(symbols[0].binding()).toEqual('global');
  });
});
