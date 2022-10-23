// eslint-disable-next-line import/no-extraneous-dependencies
import {
  LeafTable,
} from '../../src/bind';

// @ts-ignore
BigInt.prototype.toJSON = function () { return this.toString(); };

describe('symbol.symbol', () => {
  it('doesn\'t crash when using section indecies', () => {
    const table = new LeafTable.u16();
    const sym = table.reference('hello');
    if (sym === null) throw new Error('Unreachable code');
    expect(sym.sectionIndex().toString()).toEqual('0');
    sym.setSectionIndex(0x8088n);
    expect(sym.sectionIndex().toString()).toEqual(0x8088.toString());
  });
});
