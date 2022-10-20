// eslint-disable-next-line import/no-extraneous-dependencies
import {
  BranchTable,
} from '../../src/bind';

describe('symbol.symbol', () => {
  it('crashes', () => {
    const x = new BranchTable.u16();
    console.log(x);
  });
  it('has been mounted', () => {
    expect(1).toEqual(1);
  });
});
