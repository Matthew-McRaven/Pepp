import { asm } from '../../../src/lib';

const program = `
    symbol: hElLo world, d       ;comment
\tinst "char"
         s: aSrA ;comment
    .ASCii "Hi s\\tan \\x00"
    @macrO        0xa,'ba' ;comment
    fish -25,go
    ;comment
`;
const formatted = `
symbol:  hello    world, d      ;comment
         inst     "char"
s:       asra                   ;comment
         .ASCII   "Hi s\\tan \\x00"
         @macrO   0xA, 'ba'     ;comment
         fish     -25, go
;comment
`;
describe('asm.visit.formatSource for all node types', () => {
  it('processes program stress test', () => {
    const tree = asm.peg.parseRoot(program);
    expect(asm.visit.formatTree(tree)).toEqual(formatted);
  });
});
