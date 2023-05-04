import { asm } from '../../../../src/lib';

describe('asm.visit.extractSymbols', () => {
  it('processes program stress test', () => {
    const tree = asm.peg.parseRoot('a:.WORD 5\nADDA x,d\nasra\nLDWA 1887,i');
    asm.visit.extractSymbols(tree);
    // console.log(asm.visit.treeToHex(tree));
    expect(asm.visit.treeToHex(tree)).toEqual(Uint8Array.from([0, 5, 161, 0, 0, 22, 64, 7, 95]));
  });
});
