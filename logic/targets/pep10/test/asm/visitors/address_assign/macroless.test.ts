import { asm } from '../../../../src/lib';

describe('asm.visit.extractSymbols', () => {
  it('processes simple programs, leaving 0\'s for symbols', () => {
    const tree = asm.peg.parseRoot('a:.WORD 5\nADDA x,d\nasra\nLDWA 1887,i');
    asm.visit.extractSymbols(tree);
    asm.visit.setTreeAddresses(tree);
    expect(asm.visit.treeToHex(tree)).toEqual(Uint8Array.from([0, 5, 161, 0, 0, 22, 64, 7, 95]));
  });
  it('correctly resolves symbol values', () => {
    const tree = asm.peg.parseRoot('a:.WORD 5\nx:ADDA x,d\n');
    asm.visit.extractSymbols(tree);
    asm.visit.setTreeAddresses(tree);
    expect(asm.visit.treeToHex(tree)).toEqual(Uint8Array.from([0, 5, 161, 0, 2]));
  });
});
